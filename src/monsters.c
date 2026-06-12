// monsters.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/30 00:44:42 PDT



#include "common/string.h"
#include "common/files.c"
#include "monsters.h"


static Monster     *pvt_monsters = nullptr;
static LenStrArray *pvt_monster_names;

static int create_string_array(FILE *fptr, void **result_out);

// reads a text file where each line is a string. This function will skip line comments and blank lines as well as
// multiline comments. Line comments start with '//' or '#' and multiline comments are C-style /* */
// Leading and trailing whitespace is trimmed.
static int monster_read_string_file(const char * monster_filename) {
    int err = process_file( monster_filename, create_string_array, (void**) &pvt_monster_names);
    // if (err == 0) {
    //     printf("In main, LenStrArray from monsters.txt is:\n");
    //     for (int i = 0; i < lsa->size; ++i) {
    //         printf("(%zd):%s\n",lsa->array[i].len, lsa->array[i].s);
    //     }
    // }
    return err;
}

int monsters_init(const char * monster_filename) {
    int result = monster_read_string_file(monster_filename);
    if (result != 0) {
        return result;
    }
    pvt_monsters = calloc(1, sizeof(Monster) * monsters_num_monsters());
    if (!pvt_monsters) {
        free_LenStrArray(pvt_monster_names);
        pvt_monster_names = nullptr;
        return ENOMEM;
    }
    const size_t num_monsters = pvt_monster_names->size;
    for (int i = 0; i < num_monsters; ++i) {
        pvt_monsters[i] = (Monster){.name = pvt_monster_names->array[i].s, .id = i};
    }
    return 0;
}

// Frees resources used by this module
void monsters_destroy(void) {
    free_LenStrArray(pvt_monster_names);
    pvt_monster_names = nullptr;
    free(pvt_monsters);
    pvt_monsters = nullptr;
}


int monsters_num_monsters(void) {
    return (int)pvt_monster_names->size;
}

static Monster * pvt_monsters_find_monster(const monster_id id) {
    return &pvt_monsters[id];
}

// find_monster() will eventually use some better data structure, but we're using an internal array for now
// the pvt version is designed to return a non-const qualified Monster * so internal functions here can mutate it.
// the non-pvt version is intended for outside API use and should be const qualified. But for now, it's not because
// many methods are mutating the monsters. As we implement more service methods, we can eventually add const here
Monster * monsters_find_monster(const monster_id id) {
    if (id < 0 || id > pvt_monster_names->size - 1 ) {
        // Oh, I miss you Java! This would be a good place to throw an exception.
        // todo (rob) this would be a good place for returning a ResultError struct,
        // containing an error code (0 for no error) and the result of the function if no error
        fprintf(stderr, "constraint violated: 0 < monster_id < %zd, monster_id = %d\n", pvt_monster_names->size, id);
        return nullptr;
    }

    return &pvt_monsters[id];
}

// overwrites the state of the monster object in storage for the argument's id member.
void monsters_update_monster(const Monster *m) {
    const int num_monsters = monsters_num_monsters();
    if (!m || m->id < 0 || m->id > num_monsters - 1 ) {
        return;
    }
    pvt_monsters[m->id] = *m;
}

void monsters_clear_all(void) {
    const int num_monsters = monsters_num_monsters();
    for (int i = 0; i < num_monsters; ++i) {
        pvt_monsters[i] = (Monster){};
    }
}


bool monsters_monster_is_in_room( const char *monster_name, const Room *r ) {
    if (!r || !monster_name || r->monster == 0 ) return false;
    const monster_id id = r->monster;
    const char *room_monster_name = monsters_name_for_id(id);
    if (! room_monster_name) return false;
    return string_starts_with_ignore_case(monster_name, room_monster_name);
}

void monsters_names_repr(void) {
    const int num_monsters = monsters_num_monsters();
    printf("MONSTER_NAMES[%d] {\n", num_monsters);
    for (int i = 0; i < num_monsters; ++i) {
        printf("'%s',\n", pvt_monster_names->array[i].s);
    }
    printf("};\n");
}

void monsters_repr(const monster_id id) {
    Monster m = pvt_monsters[id];
    printf("(Monster){}");
}

const char * monsters_name_for_id(const monster_id id) {
    const size_t num_monsters = pvt_monster_names->size;
    if (id < 0 || id > num_monsters - 1) return "null";

    return pvt_monster_names->array[id].s;
}





// reads the text file from the argument stream pointer and extracts each line into an array element in LenStrArray
// returns the result in the out ptr, a *LenStrArray
static int create_string_array(FILE *fptr, void **result_out) {

    size_t results_capacity = 100;
    size_t result_counter = 0;
    LenStr *results = malloc(sizeof(LenStr) * results_capacity);
    if (!results) return ENOMEM;
    // we insert the null monster name in the first position
    results[result_counter++] = (LenStr){.s = strdup("NULL"), .len=strlen("NULL") };

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

