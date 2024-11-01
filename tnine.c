#define MAX_VALUE_LENGTH 100
#define MAX_CONTACT_COUNT 42

#include <stdio.h> // puts, printf, fgets
#include <string.h> // strlen, strcpy, strstr
#include <ctype.h> // isalpha, islower
#include <stdbool.h> // bool, true, false

typedef struct contact {
    char name[MAX_VALUE_LENGTH + 1];
    char phone_number[MAX_VALUE_LENGTH + 1];
} contact;

typedef enum parse_err {
    ERR_CONTACT_LIMIT = -1,
    ERR_TOOLONG = -2,
    ERR_NONUMBER = -3
} parse_err;

typedef enum program_err {
    OK = 0,
    ARGUMENT_ERROR = 1,
    PARSE_ERROR = 2
} program_err;

/*
Prints an error represented by the parse_err enum to stderr
*/
void print_parse_err(parse_err error) {
    static const char *error_strings[] = {"Contact limit reached", "Input line is too long", "Contact is missing a number"};
    int index = (error / -1) - 1;
    fprintf(stderr, "Error: %s", error_strings[index]);
}

/*
Replaces the trailing newline in string with a null byte (if there is one)
*/
void remove_newline(char* string) {
    if(string[strlen(string)-1] == '\n') {
        string[strlen(string)-1] = '\0';
    }
}

/*
Copies string to buffer and replaces the leading plus character with a zero (if there is one)

string must be null-terminated and buffer must be as large as string!
*/
void replace_plus(const char* string, char* buffer) {
    strcpy(buffer, string);
    char* plus = strchr(buffer, '+');
    if(plus != NULL) {
        *plus = '0';
    }
}

/*
Writes the "T9 keypad" representation of string to buffer
Non-alphabetical characters are replaced with X (except for '+')
Numerical characters are copied over

Example:
"V. Havel" -> "8XX42835"

Buffer must be as large as the input string!
*/ 
void str_to_tnine(const char* string, char* buffer) {
    for (size_t i = 0; i < strlen(string); i++) {
        char c = string[i];
        if(c == '+') {
            buffer[i] = '0';
            continue;
        }
        if(isdigit(c)) {
            buffer[i] = c;
            continue; // Copy numerical characters
        }
        if(!isalpha(c)) {
            buffer[i] = 'X';
            continue; // Skip non-alphabetical characters
        }
        
        if(islower(c)) c = toupper(c);

        if(c < 'P') {
            /* This works for the first two rows
             each button in order represents three letters and we start at 2 */
            c = ((c - 'A') / 3) + 2; 
        } else if(c < 'T') {
            c = 7;
        } else if(c < 'W') {
            c = 8;
        } else if(c <= 'Z') {
            c = 9;
        }
        buffer[i] = c + '0'; // Convert the number to the character 
    }
    buffer[strlen(string)] = '\0';
}

bool contains_in_order(const char* string, const char* sequence) {
    unsigned int seq_count = 0;
    for(size_t i = 0; i < strlen(string); i++) {
        if(string[i] == sequence[seq_count]) seq_count++;
    }
    return seq_count == strlen(sequence);
}

/*
 Reads up to (max) contacts into the supplied array from stdin until EOF and returns the final count
 Returns a value from the parse_err enum on error
*/
int parse_contacts(contact contacts[], int max) {
    int contact_count = 0;
    char linebuffer[MAX_VALUE_LENGTH + 2]; // +2 to account for the newline and null terminator
    for(int i = 0; fgets(linebuffer, MAX_VALUE_LENGTH + 1, stdin) != NULL; i++) {
        if(i >= max) return ERR_CONTACT_LIMIT;

        // If the string read doesn't contain a newline, we went over the 100 char limit
        if(strchr(linebuffer, '\n') == NULL) return ERR_TOOLONG;

        remove_newline(linebuffer);
        strncpy(contacts[i].name, linebuffer, MAX_VALUE_LENGTH);
        if(fgets(linebuffer, MAX_VALUE_LENGTH + 1, stdin) == NULL) { // Contact with no number, can't continue
            return ERR_NONUMBER;
        }

        remove_newline(linebuffer);
        strncpy(contacts[i].phone_number, linebuffer, MAX_VALUE_LENGTH);
        contact_count++;
    }
    return contact_count;
}

void print_all_contacts(const contact contacts[], int count) {
    for(int i = 0; i < count; i++) {
        printf("%s, %s\n", contacts[i].name, contacts[i].phone_number);
    }
}

bool contact_matches(const contact contact, char* query, bool nonstrict) {
    char str_buffer[MAX_VALUE_LENGTH + 1];
    if(nonstrict) {
        replace_plus(contact.phone_number, str_buffer);
        if(contains_in_order(str_buffer, query)) return true;

        str_to_tnine(contact.name, str_buffer);
        if(contains_in_order(str_buffer, query)) return true;
    } else {
        replace_plus(contact.phone_number, str_buffer);
        if(strstr(str_buffer, query) != NULL) return true;

        str_to_tnine(contact.name, str_buffer);
        if(strstr(str_buffer, query) != NULL) return true;
    }
    return false;
}

void search_contacts(const contact contacts[], int count, char* query, bool nonstrict) {
    bool is_found = false;
    for(int i = 0; i < count; i++) {
        if(contact_matches(contacts[i], query, nonstrict)) {
            printf("%s, %s\n", contacts[i].name, contacts[i].phone_number);
            is_found = true;
        }
    }
    if(!is_found) {
        puts("Not found");
    }
}

bool validate_query(const char* query) {
    for(size_t i = 0; i < strlen(query); i++) {
        if(!isdigit(query[i])) return false;
    }
    return true;
}

int main(int argc, char** argv) {
    int contact_count = 0;
    contact contacts[MAX_CONTACT_COUNT];
    char query[MAX_VALUE_LENGTH + 1];
    bool nonstrict_search = false;

    if(argc > 3 || (argc == 3 && strcmp(argv[1], "-s") != 0)) {
        fputs("Invalid arguments, exiting.\nUsage: ./tnine [-s] [SEARCH QUERY]", stderr);
        return ARGUMENT_ERROR;
    }

    if(argc > 1) {
        if(argc == 3) { // We have already verified that argv[1] is -s
            strcpy(query, argv[2]);
            nonstrict_search = true;
        } else {
            strcpy(query, argv[1]);
        }
        if(!validate_query(query)) {
            fputs("Invalid query (must be a number)", stderr);
            return ARGUMENT_ERROR;
        }
    }

    contact_count = parse_contacts(contacts, MAX_CONTACT_COUNT);

    if(contact_count < 0) {
        print_parse_err(contact_count);
        return PARSE_ERROR;
    }

    if(argc == 1) {
        print_all_contacts(contacts, contact_count);
    } else {
        search_contacts(contacts, contact_count, query, nonstrict_search);
    }

    return OK;
}