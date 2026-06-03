// files.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/02 22:18:43 PDT

// Jambooty
// Created by Rob Ross on 6/2/26.
//

#include <ctype.h>
#include "files.h"
#include "string.h"


int get_next_line_chunk(FILE *fptr, int len, char buffer[static len]) {
    if (!fptr) {
        printf("get_next_line_chunk() called with nullptr.\n");
        return -1;
    }
    string s = fgets(buffer, len, fptr);
    if (s == nullptr) {
        if (ferror(fptr)) {
            fprintf(stderr, "\nError reading file during get_next_line_chunk.\n");
            return errno;
        }
        return EOF;
    }

    return 0;
}



LenStr parsed_strings[100] = {}; // fixed buffer for example code. in prod code this would be a dynamic list.
ValuePos parsed_pos[100] = {};

int str_count = 0;
int parsed_pos_count = 0;

void display_parsed_strings( const int len, LenStr ls[static len] ) {
    for (int i = 0; i < len; ++i) {
        printf("len=%zd, str:'%s'\n", ls[i].len, ls[i].s);
    }
}

void display_value_positions( const int len, ValuePos vp[static len]) {
    for (int i = 0; i < len; ++i) {
        printf("str='%s', start:%d, end:%d, start[l:%d c:%d], end[l:%d c:%d]\n",
            vp[i].value.s,
            vp[i].start_char_pos, vp[i].end_char_pos ,
            vp[i].line_num_start, vp[i].col_start,
            vp[i].line_num_end, vp[i].col_end);
    }
}

void free_parsed_strings( const int len, LenStr ls[static len] ) {
    for (int i = 0; i < len; ++i) {
        free((void*)ls[i].s);
    }
}

// adds the character to the buffer argument, growing the buffer via realloc if capacity is reached.
// Returns 0 on success. On success, len, capacity, and buffer are updated if they changed.
// on error, no changes are made to input arguments.
int add_to_expandable_buffer(const char c, size_t *len, size_t *capacity, char **buffer) {
    size_t l = *len;
    size_t cap = *capacity;
    if (l + 1 >= cap) {
        cap *= 2;
        char *temp = realloc(*buffer, cap);
        if (!temp) {
            return ENOMEM;
        }
        *capacity = cap;
        *buffer = temp;
    }
    (*buffer)[l++] = c;
    *len = l;
    return 0;
}


int find_json_strings(FILE *fptr) {
    // iterate through the entire file one buffer chunk at a time.
    // if not state string_start, iterate buffer looking for double-quotes.
    //  if found, set state string_start, save start pos.
    //  if not found, get next file chunk, continue loop
    // if state is string_start, continue reading chunk until end of chunk or end of file or until '"" found
    char buffer[1024];
    bool in_string = false;

    // Dynamic buffer to accumulate the string
    size_t val_capacity = 128;
    size_t val_len = 0;
    char *val_buffer = malloc(val_capacity);


    if (!val_buffer) return ENOMEM;
    int line_count = 1;
    int col_count = 1;
    int char_count = 1;


    while ( get_next_line_chunk(fptr, sizeof(buffer), buffer) == 0  ) {
        int buffer_index = 0;
        while (buffer[buffer_index] != '\0') {
            char c = buffer[buffer_index++];
            char_count++;
            col_count++;
            if (c == '\n') {
                line_count++;
                col_count = 1;
            }

            if (!in_string) {
                if (c == QUOTATION_MARK) {
                    in_string = true;
                    val_len = 0; // Reset accumulator for new string
                    parsed_pos[parsed_pos_count++] =
                        (ValuePos){ .start_char_pos = char_count, .line_num_start = line_count, .col_start = col_count };
                }
            } else {
                if (c == QUOTATION_MARK) {
                    in_string = false;
                    val_buffer[val_len] = '\0';
                    printf("Found String: [%s]\n", val_buffer);
                    string s = strdup(val_buffer);
                    if (!s) {
                        free(val_buffer);
                        free_parsed_strings(str_count, parsed_strings);
                        return ENOMEM;
                    }
                    LenStr ls = {.len=val_len, .s = s};
                    parsed_strings[str_count++] = ls;
                    parsed_pos[parsed_pos_count - 1].end_char_pos = char_count;
                    parsed_pos[parsed_pos_count - 1].line_num_end = line_count;
                    parsed_pos[parsed_pos_count - 1].col_end = col_count;
                    parsed_pos[parsed_pos_count - 1].value = ls;


                } else {
                    // Check if we need to grow the accumulator
                    if (val_len + 1 >= val_capacity) {
                        val_capacity *= 2;
                        char *temp = realloc(val_buffer, val_capacity);
                        if (!temp) {
                            free(val_buffer);
                            free_parsed_strings(str_count, parsed_strings);
                            return ENOMEM;
                        }
                        val_buffer = temp;
                    }

                    // Handle the "MUST escape" rule
                    if (c < 32) {
                        fprintf(stderr, "Error: Unescaped control character (ASCII %d) in string.\n", c);
                        // Technically we should return an error here per JSON spec
                        c = ' ';
                    }
                    val_buffer[val_len++] = c;
                }
            }
        }
    }

    if (in_string) {
        fprintf(stderr, "Error: reached EOF before string terminated.\n");
    }

    free(val_buffer);

    display_parsed_strings( str_count, parsed_strings);
    display_value_positions(parsed_pos_count, parsed_pos);

    free_parsed_strings(str_count, parsed_strings);
    return 0;
}


static void free_LenStr(size_t size, LenStr array[static size]) {
    for (int i = 0; i < size; ++i) {
        free((void*)array[i].s);
    }
}

void free_LenStrArray(LenStrArray *lsa) {
    if (!lsa) return;
    free_LenStr(lsa->size, lsa->array);
    free(lsa);
}

// reads the text file from the argument stream pointer and extracts each line into an array element in LenStrArray
// returns the result in the out ptr, a *LenStrArray
int create_string_array(FILE *fptr, void **result_out) {

    size_t results_capacity = 100;
    size_t result_counter = 0;
    LenStr *results = malloc(sizeof(LenStr) * results_capacity);
    if (!results) return ENOMEM;

    char buffer[1024] = {};
    constexpr size_t buffer_len = sizeof(buffer);
    bool in_block_comment = false;

    // Dynamic buffer to accumulate the string
    size_t val_capacity = 128;
    size_t val_len = 0;
    char *val_buffer = malloc(val_capacity);
    if (!val_buffer) {
        free(results);
        return ENOMEM;
    }

    while ( get_next_line_chunk(fptr, buffer_len, buffer) == 0 ) {
        // strip leading and trailing spaces
        string_trim(buffer);
        int buffer_index = 0;
        val_len = 0;
        while (buffer[buffer_index] != '\0') {
            char c = buffer[buffer_index++];
            bool one_more_char = buffer[buffer_index] != '\0';
            
            if (in_block_comment) {
                if (c == '*' && one_more_char && buffer[buffer_index] == '/') {
                    // end of block comment
                    in_block_comment = false;
                    break; // match original behavior: skip rest of line
                }
                // Skip all characters while in block comment
                continue;
            }

            // Check for comment starts anywhere on the line
            if (c == '#') break;
            if (c == '/' && one_more_char) {
                if (buffer[buffer_index] == '/') break; // line comment
                if (buffer[buffer_index] == '*') {
                    in_block_comment = true;
                    break; // match original behavior: skip rest of line
                }
            }

            if ( add_to_expandable_buffer(c, &val_len, &val_capacity, &val_buffer) != 0 ) {
                free(val_buffer);
                free_LenStr(result_counter, results);
                free(results);
                return ENOMEM;
            }
        }
        
        if ( val_len ) {
            // save this string if it's not empty
            val_buffer[val_len] = '\0';
            // Trim trailing spaces that might remain before a comment
            string_trim(val_buffer);
            size_t trimmed_len = strlen(val_buffer);
            if (trimmed_len == 0) continue;

            if (result_counter >= results_capacity) {
                results_capacity *= 2;
                LenStr *temp = realloc(results, sizeof(LenStr) * results_capacity);
                if (!temp) {
                    free_LenStr(result_counter, results);
                    free(results);
                    free(val_buffer);
                    return ENOMEM;
                }
                results = temp;
            }

            char *s = strdup(val_buffer);
            if (!s) {
                free_LenStr(result_counter, results);
                free(results);
                free(val_buffer);
                return ENOMEM;
            }
            results[result_counter++] = (LenStr){.len=trimmed_len, .s = s};
        }
    }
    free(val_buffer);

    LenStrArray *lsa = malloc(sizeof(LenStrArray) + sizeof(LenStr) * result_counter);
    if (!lsa) {
        free_LenStr(result_counter, results);
        free(results);
        return ENOMEM;
    }

    lsa->size = result_counter;

    for (int i = 0; i < (int)result_counter; ++i) {
        lsa->array[i] = results[i];
    }
    free(results);
    (*result_out) = lsa;

    return 0;
}

int echo_file( FILE *fptr) {
    printf("In echo file!\n");

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fptr) != nullptr ) {
        printf("%s",buffer);
    }

    if (ferror(fptr)) {
        fprintf(stderr, "\nError reading file during echo_file.\n");
        return errno;
    }

    putchar('\n');
    return 0;
}


int process_file(string file_name, file_process_action function, void **result_ptr) {
    FILE *fptr = nullptr;

    fptr = fopen(file_name, "r");
    if (!fptr) {
        printf("fopen failed for %s, error:%d, ", file_name, errno);
        perror(" ");
        return errno;
    }

    printf("file opened: %s\n", file_name);
    int result = function(fptr, result_ptr);

    if (fclose(fptr) != 0) {
        printf("fclose failed for %s, error:%d, ", file_name, errno);
        perror(" ");
        return errno;
    }

    printf("file closed: %s\n", file_name);
    return result;
}

// make:
// clang -g -std=c23 -fsanitize=address -fsanitize=leak files.c string.c -o files_test.out

int main(void) {


    printf("\n\n");
    LenStrArray *lsa;
    int err = process_file("../chateau_gaillard/monsters.txt", create_string_array, (void**) &lsa);
    if (err == 0) {
        printf("In main, LenStrArray from monsters.txt is:\n");
        for (int i = 0; i < lsa->size; ++i) {
            printf("(%zd):%s\n",lsa->array[i].len, lsa->array[i].s);
        }
    }
    printf("\n");
    free_LenStrArray(lsa);
    return 0;
}