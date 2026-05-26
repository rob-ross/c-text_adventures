// console_utils.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 20:01:31 PDT


#include "console_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/_types/_useconds_t.h>
#include <ctype.h>


uint32_t GLOBAL_char_sleep_duration = _15ms;
bool   GLOBAL_silent_mode = true;


//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------

void cls() {
    // \033[2J clears the screen, \033[H moves the cursor to the top-left corner
    printf("\033[2J\033[H");
    fflush(stdout);
}

/** API for ML/AI to suppress text output */
void set_silent_mode(const bool silent) {
    GLOBAL_silent_mode = silent;
}

// sets the current sleep duration (in microseconds) for future calls to char_sleep().
// returns the previous sleep duration value
uint32_t set_char_sleep(const uint32_t microseconds) {
    const uint32_t temp = GLOBAL_char_sleep_duration;
    GLOBAL_char_sleep_duration = microseconds;
    return temp;
}

// pass -1 to sleep for GLOBALS.char_sleep_duration  (see set_char_sleep(),
// or pass a duration >0 in microseconds
// ReSharper disable once CppDFAConstantParameter
void char_sleep(const int32_t microseconds) {
    useconds_t sleep_time;
    // ReSharper disable once CppDFAConstantConditions
    if (microseconds >= 0) {
        sleep_time = microseconds;
    } else {
        sleep_time = GLOBAL_char_sleep_duration;
    }
    if (sleep_time > 0) {
        // usleep() takes argument in microseconds
        usleep(sleep_time); // todo is this portable? What about windows?
    }
}

//display string without adding newline
void display(char const* msg ) {
    if (GLOBAL_silent_mode) return;

    fflush(stdout);
    for (char const *next = msg; *next; ++next) {
        putchar(*next);
        fflush(stdout);
        char_sleep(-1);
    }
}

//displays the string and adds newline to end.
void display_line(char const* msg ) {
    if (GLOBAL_silent_mode) return;
    display(msg);
    putchar('\n');
    fflush(stdout);
    char_sleep(-1);
}

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

// determines how much of the string to print on one line then handles the rest of the string
// recursively. It works but didn't seem to really simplify the method as I thought it would.
// the previous version used an outer loop until all characters were processed, but it became very messy.
void display_paginated_recursive(char const* msg, const int num_columns) {
    const int str_len = strlen(msg);
    if ( ! str_len ) return;  //base case

    int potential_char_count = min(num_columns, str_len);

    int potential_line_end = potential_char_count;


    char const * next_start_char = msg;
    // we only want to break on spaces, newline, or punctuation.
    // check for newlines, which indicate a new paragraph. todo (rob) should we have a formal mark-up for Paragraph?
    // for newlines, we want to break on each from left to right, so we scan from the left
    bool has_newline = false;
    bool initial_spaces = true; //set to false when we see the first not-space character at start of line
    int line_start = 0;
    for (int c = 0; c < potential_line_end; ++c ) {
        if (msg[c] == '\n') {
            //We break on this newline
            potential_line_end = c;
            next_start_char = msg +  potential_line_end + 1;
            has_newline = true;
            break;
        }
        if ( isblank(msg[c]) && initial_spaces) {
            line_start++;
        } else {
            initial_spaces = false;
        }
    }

    //  potential_line_end is the count of the number of characters, like a length of an array.
    // a line takes on index values 0 to potential_line_end - 1.

    if (! has_newline) {
        if ( isalnum(msg[potential_line_end]) ||
            ( ispunct(msg[potential_line_end]) &&  msg[potential_line_end] != '-' ) ) {
            // if next char after the end of this line is a blank, the last word can fit on the current line
            bool line_ends_before_space = potential_line_end < (str_len) && isblank(msg[potential_line_end ]);

            if (!line_ends_before_space) {
                // last character is alphanumeric or punctuation, so walk back until we find white space
                while ( --potential_line_end > 0 &&
                    ( isalnum(msg[potential_line_end]) ||
                        ( ispunct(msg[potential_line_end]) &&  msg[potential_line_end] != '-' ) ) );

                // it's bad design when you don't know why a line like this works. but for now, it works with this and
                // doesn't work without it. todo (rob) trace this algorithm and prove it correct
                // line end var is exclusive so we must add one here.
                // Even though we didn't have to add it in the previous loop?? because reasons?
                potential_line_end++;  // we decremented one too many times

                if (potential_line_end <= 1 ) {
                    // degenerate case, entire line is one large string don't break it
                    potential_line_end = potential_char_count;
                } else {
                    potential_char_count = potential_line_end;
                }
            }

        }
        next_start_char = msg + potential_line_end;
    }




    int line_end = potential_line_end;

    for (int c = line_start; c < line_end; ++c ) {
        if (msg[c] != '\n' ) {
            putchar(msg[c]);
            char_sleep(-1);
            fflush(stdout);
        }
    }
    putchar('\n');
    if (has_newline) putchar('\n');  // explicit newline means start a new paragraph

    display_paginated_recursive(next_start_char, num_columns);

}

// paginate msg text to column size
void display_paginated(char const* msg, const int num_columns) {
    display_paginated_recursive(msg, num_columns);

    char * foo = "While out walking one day, you come across a small, ramshackle shed in the woods";
}


void display_paginated_(char const* msg, const int num_columns) {
    // todo (rob) not tested with Unicode, so this will probably not work for Unicode
    int str_len = strlen(msg);
    if (str_len <= num_columns) {
        display_line(msg);
        return;
    }
    // paginate to column size
    char const * char_start = msg;
    int chars_used = 0;
    int line_length = num_columns;
    while (chars_used < str_len) {
        if (chars_used + line_length >= str_len) {
            // last line
            display_line(char_start);
            return;
        }
        char const * end_target = char_start + line_length;
        while ( isalnum(*end_target)) {
            // walk back the line while the last character would be alpha numeric
            end_target--;
            line_length--;
        }
        if (line_length == 0) {
            // the last "word" takes up the entire num_columns width, so we have to break it
            line_length = num_columns;
            end_target = char_start + line_length;
        }
        // print the line
        fflush(stdout);
        for (char const *next = char_start; next <= end_target; ++next) {
            putchar(*next);
            fflush(stdout);
            char_sleep(-1);
        }
        char_start = end_target + 1;
        chars_used += line_length;
    }
}


//// ------------------------------------------------------------
////
////    INPUT
////
//// ------------------------------------------------------------


static bool stdin_has_data(void) {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    struct pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;
    return poll(&fds, 1, 0) > 0;
#endif
}

void flush_input(void) {
    while (stdin_has_data()) {
        int c = getchar();
        if (c == '\n' || c == EOF) break;
    }
}

// Allocates a new CharBuffer based on user input and returns a pointer to
// the newly allocated CharBuffer. Caller is responsible for freeing.
CharBuffer *get_char_buffer(char const *prompt) {
    display(prompt);
    char temp_buffer[1024];
    size_t len = 0;

    if (fgets(temp_buffer, sizeof(temp_buffer), stdin)) {
        len = strlen(temp_buffer);
        if (len > 0) {
            if (temp_buffer[len - 1] == '\n') {
                // Normal case: entire line read, remove newline
                temp_buffer[len - 1] = '\0';
                len--; // Decrement length to reflect newline removal
            } else {
                // Truncation case: buffer was too small, leftovers remain in stdin
                flush_input();
            }
        }
    } else {
        // Handle fgets failure/EOF
        temp_buffer[0] = '\0';
        len = 0;
    }

    CharBuffer *cb = malloc(sizeof(CharBuffer) + len + 1);
    if (cb) {
        cb->length = len;
        memcpy((void *)cb->buffer, temp_buffer, len + 1);
    }
    return cb;
}

static struct StringBuffer {char buffer[1024];} get_str(char const *  prompt) {
    struct StringBuffer sb = {};
    display(prompt);

    if (fgets(sb.buffer, sizeof(sb.buffer), stdin)) {
        size_t len = strlen(sb.buffer);
        if (len > 0) {
            if (sb.buffer[len - 1] == '\n') {
                // Normal case: entire line read, remove newline
                sb.buffer[len - 1] = '\0';
            } else {
                // Truncation case: buffer was too small, leftovers remain in stdin
                flush_input();
            }
        }
    }
    return sb;
}

int get_int(char const * const prompt, const int min, const int max) {
    for (;;) {
        struct StringBuffer sb = get_str(prompt);

        // If the user just hit enter, sb.buffer[0] will be '\0'
        if (sb.buffer[0] == '\0') {
            display_line("INPUT CANNOT BE EMPTY. PLEASE ENTER A NUMBER.");
            continue;
        }

        char *endptr;
        long val = strtol(sb.buffer, &endptr, 10);

        // If endptr is the same as the buffer, no numbers were found at the start
        if (endptr == sb.buffer) {
            display_line("INVALID INPUT. PLEASE ENTER A VALID INTEGER.");
            continue;
        }

        if (val < min || val > max) {
            printf("OUT OF RANGE. PLEASE ENTER A NUMBER BETWEEN %d AND %d.\n", min, max);
            continue;
        }

        return (int)val;
    }
}


static void display_command_err(char const * msg, char const command) {
    if (!msg) {
        msg = "INVALID COMMAND: ";
    }
    display(msg);
    printf("'%c'\n", command);
}

// return the first letter of the user's input converted to uppercase.
// input char must be in the `valid_chars` string to be accepted or user is re-prompted until it is
// err_msg may be null
char get_command_char(char const * const prompt, char const * const valid_chars, char const * const err_msg) {
    char first_letter;
    bool is_invalid_command;
    do {
        const struct StringBuffer sb = get_str(prompt);
        first_letter = (char)toupper(sb.buffer[0]);
        is_invalid_command = ! strchr(valid_chars, first_letter);
        if (is_invalid_command) {
            display_command_err(err_msg, first_letter);
        }
    } while (is_invalid_command);

    return first_letter;
}
