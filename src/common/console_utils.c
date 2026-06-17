// console_utils.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 20:01:31 PDT




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <poll.h>
#endif

#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
#endif


#include <sys/_types/_useconds_t.h>
#include <ctype.h>
#include <stdarg.h> // Added for variadic functions

#include "console_utils.h"

// todo (rob) this is messy. We want a GLOBALS object to share among all TU for truly global values.
// but to avoid cyclic dependencies this has to be re-declared here since we can't have console_utils
// importing the main TU. We'll probably have to change this again back to a GLOBALS object scoped to just
// this TU with getter/setter methods for users of this TU.
struct GlobalState {
    const char * player_name;
    bool         silent_mode;
    uint32_t     char_sleep_duration;
    uint32_t     char_sleep_visited_duration;
    uint32_t     debug_normal_sleep;
    uint32_t     debug_visited_sleep;
    bool         debug_mode;
};
extern struct GlobalState GLOBALS;




//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------

void cls() {
    // \033[2J clears the screen, \033[H moves the cursor to the top-left corner
    // printf("\033[2J\033[H");


#ifdef _WIN32
    // For legacy Windows CMD, this clears both screen and scrollback
    system("cls");
#else
    // For Linux, macOS, and modern Windows Terminal
    /*
     *  \033[3J: Tells the terminal emulator to delete all saved lines in the scrollback buffer.
     *  \033[2J: Clears the entire visible screen.
     *  \033[H: Moves the cursor back to row 1, column 1 (top-left).
     */
    printf("\033[3J\033[2J\033[H");
    fflush(stdout);
#endif
}

/** API for ML/AI to suppress text output */
void set_silent_mode(const bool silent) {
    GLOBALS.silent_mode = silent;
}

// sets the current sleep duration (in microseconds) for future calls to char_sleep().
// returns the previous sleep duration value
uint32_t set_char_sleep(const uint32_t microseconds) {
    const uint32_t temp = GLOBALS.char_sleep_duration;
    GLOBALS.char_sleep_duration = microseconds;
    return temp;
}

// pass -1 to sleep for GLOBALS.char_sleep_duration (see set_char_sleep(),
// or pass a duration >0 in microseconds
// ReSharper disable once CppDFAConstantParameter
void char_sleep(const int32_t microseconds) {
    useconds_t sleep_time;
    // ReSharper disable once CppDFAConstantConditions
    if (microseconds >= 0) {
        sleep_time = microseconds;
    } else {
        sleep_time = GLOBALS.char_sleep_duration;
    }
    if (sleep_time > 0) {
        // usleep() takes argument in microseconds
        // usleep(sleep_time); // previously we used usleep() here.
#ifdef _WIN32
        Sleep(sleep_time / 1000);
#else
        struct timespec ts = {
            .tv_sec = sleep_time / 1'000'000,  // seconds, type time_t
            .tv_nsec = (sleep_time % 1'000'000) * 1000 // nanoseconds, range: 0 - 999,999,999 inclusive
        };
        // The second parameter is `rem`, "remaining." If nanosleep() is interrupted before sleeping the full duration,
        // it will write how much time is remaining to sleep in this struct timespec. You can use it to call
        // nanosleep() again with the remaining time as the first argument.
        nanosleep(&ts, nullptr);
#endif
    }
}

//display string without adding newline
void display(char const* msg ) {
    if ( GLOBALS.silent_mode ) return;
    if (!msg) { msg = "(null)"; }

    fflush(stdout);
    for (char const *next = msg; *next; ++next) {
        putchar(*next);
        fflush(stdout);
        char_sleep(-1);
    }
}

//displays the string and adds newline to end.
void display_line(char const* msg ) {
    if (GLOBALS.silent_mode) return;
    display(msg);
    putchar('\n');
    fflush(stdout);
    char_sleep(-1);
}

// Internal helper function for vdisplay and vdisplay_line
// This function performs the formatting and character-by-character display without adding a newline.
static void _vdisplay_internal(const char * restrict format, va_list args_orig) { // NOLINT(*-reserved-identifier)
    if (GLOBALS.silent_mode) return;

    // Determine the size needed for the formatted string
    va_list args_copy_for_size;
    va_copy(args_copy_for_size, args_orig); // Make a copy for the first vsnprintf call
    int size = vsnprintf(nullptr, 0, format, args_copy_for_size);
    va_end(args_copy_for_size); // End the copy

    if (size < 0) {
        // Handle error, e.g., print a generic error message or log it
        display_line("Error formatting message.");
        return;
    }

    // Allocate buffer for the formatted string + null terminator
    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        // Handle memory allocation error
        display_line("Memory allocation error for message.");
        return;
    }

    // Format into buffer using the original va_list
    vsnprintf(buffer, size + 1, format, args_orig);

    display(buffer); // Use existing display function for char-by-char output

    free(buffer);
}


// supports format string and variadic args (without adding a newline)
void displayf(const char * restrict format, ...) {
    if (GLOBALS.silent_mode) return;

    va_list args;
    va_start(args, format);
    _vdisplay_internal(format, args);
    va_end(args);
}

// calls vdisplay() and appends a newline.
void display_linef(const char * restrict format, ...) {
    if (GLOBALS.silent_mode) return;

    va_list args;
    va_start(args, format);
    _vdisplay_internal(format, args); // Use the internal helper
    va_end(args);

    putchar('\n'); // Add the newline here
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
    const int str_len = (int)strlen(msg);
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
    // if (has_newline) putchar('\n');  // explicit newline means start a new paragraph

    display_paginated_recursive(next_start_char, num_columns);

}

// paginate msg text to column size
void display_paginated(char const* msg, const int num_columns) {
    display_paginated_recursive(msg, num_columns);
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