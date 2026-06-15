// monsters.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/30 00:44:42 PDT


#include "roblib/json_parser/json_parser.h"
#include "common/string.h"
#include "common/files.c"
#include "monsters.h"

#include <assert.h>


static MonsterPrototypeArray *monster_prototypes_array = nullptr;  // this acts like a singleton in this monster library





static int create_string_array(FILE *fptr, void **result_out);
static int load_monster_json_file(FILE *fptr, void **monster_array_out);

// reads a text file where each line is a string. This function will skip line comments and blank lines as well as
// multiline comments. Line comments start with '//' or '#' and multiline comments are C-style /* */
// Leading and trailing whitespace is trimmed.
static int monster_read_string_file(const char * monster_filename) {

    LenStrArray *pvt_monster_names;
    int err = process_file( monster_filename, create_string_array, (void**) &pvt_monster_names);
    // if (err == 0) {
    //     printf("In main, LenStrArray from monsters.txt is:\n");
    //     for (int i = 0; i < lsa->size; ++i) {
    //         printf("(%zd):%s\n",lsa->array[i].len, lsa->array[i].s);
    //     }
    // }
    return err;
}

static int monster_read_json_file(const char * monster_json_filename) {
    // `load_monster_json_file` is the worker method here; `process_file` just ensures the file is closed properly
    int err = process_file( monster_json_filename, load_monster_json_file, (void**) &monster_prototypes_array);
    return err;
}

int monsters_init(const char * monster_filename) {
    int result = monster_read_json_file(monster_filename);
    if (result != 0) {
        // show error message here?
        return result;
    }
    return 0;
}

// Frees resources used by this module
void monsters_destroy(void) {
    const uint32_t num_monsters = monster_prototypes_array->len;

    // monster names were copied from json parser arena via strdup, so we must free them
    for (int i = 0; i < num_monsters ; ++i) {
        free((void*)monster_prototypes_array->monsters[i].name);
    }

    free(monster_prototypes_array);
    monster_prototypes_array = nullptr;
}


int monsters_num_monsters(void) {
    return (int)monster_prototypes_array->len;
}

static MonsterPrototype * pvt_monsters_find_monster(const monster_id id) {
    return &monster_prototypes_array->monsters[id];
}

// find_monster() will eventually use some better data structure, but we're using an internal array for now
// the pvt version is designed to return a non-const qualified Monster * so internal functions here can mutate it.
// the non-pvt version is intended for outside API use and should be const qualified. But for now, it's not because
// many methods are mutating the monsters. As we implement more service methods, we can eventually add const here
MonsterPrototype * monsters_find_monster(const monster_id id) {
    if (id < 0 || id > monster_prototypes_array->len - 1 ) {
        // Oh, I miss you Java! This would be a good place to throw an exception.
        // todo (rob) this would be a good place for returning a ResultError struct,
        // containing an error code (0 for no error) and the result of the function if no error
        fprintf(stderr, "constraint violated: 0 < monster_id < %d, monster_id = %d\n", monster_prototypes_array->len, id);
        return nullptr;
    }

    return &monster_prototypes_array->monsters[id];
}

// overwrites the state of the monster object in storage for the argument's id member.
void monsters_update_monster(const MonsterPrototype *m) {
    const int num_monsters = monsters_num_monsters();
    if (!m || m->id < 0 || m->id > num_monsters - 1 ) {
        return;
    }
    monster_prototypes_array->monsters[m->id] = *m;
}

void monsters_clear_all(void) {
    const int num_monsters = monsters_num_monsters();
    // for (int i = 0; i < num_monsters; ++i) {
    //     pvt_monsters[i] = (Monster){};
    // }
}


bool monsters_monster_is_in_room( const char *monster_name, const Room *r ) {
    if (!r || !monster_name || r->monster == 0 ) return false;
    const monster_id id = r->monster;
    const char *room_monster_name = monsters_name_for_id(id);
    if (! room_monster_name) return false;
    return string_starts_with_ignore_case(monster_name, room_monster_name);
}

static void monsters_stats_repr(const CharStats stats) {
    printf("(CharStats){ .strength=%d, .charisma=%d, .dexterity=%d, .intelligence=%d, .wisdom=%d, .constitution=%d }",
            stats.strength, stats.charisma, stats.dexterity, stats.intelligence, stats.wisdom, stats.constitution);
}

void monsters_repr(const monster_id id) {
    MonsterPrototype *m = pvt_monsters_find_monster(id);
    printf("(Monster){ .id=%d, .ferocity_factor=%d, ", m->id, m->ferocity_factor);
    monsters_stats_repr(m->stats);
    printf(", .name='%s' } \n", m->name);
}

void monsters_all_repr() {
    for (int i = 0; i < monster_prototypes_array->len; ++i) {
        monsters_repr(i);
    }
}

const char * monsters_name_for_id(const monster_id id) {
    const size_t num_monsters = monster_prototypes_array->len;
    if (id < 0 || id > num_monsters - 1) return "null";
    return monster_prototypes_array->monsters[id].name;
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

// parses the json file from the argument stream pointer and extracts monster objects into a Monster struct instance
// returns the result in the out ptr, a *Monster
static int load_monster_json_file(FILE *fptr, void **monster_array_out) {
    if (!fptr) return EINVAL;

    // 1. Determine file size
    fseek(fptr, 0, SEEK_END);
    long length = ftell(fptr);
    if (length < 0) return EIO;
    rewind(fptr);

    // 2. Allocate memory
    char *json_text_buffer = malloc(length + 1);
    if (!json_text_buffer) return ENOMEM;

    // 3. Read the file into the buffer
    size_t read_size = fread(json_text_buffer, 1, length, fptr);
    if (read_size != (size_t)length) {
        free(json_text_buffer);
        return EIO;
    }

    // 4. Null-terminate the string
    json_text_buffer[length] = '\0';

    // TODO: Pass 'buffer' to your JSON parser here
    Error error = jsonp_init();
    if (error.err) {
        err_print(error);
        return error.reported_err;
    }
    JsonError err = {.json = json_text_buffer};
    printf("\nParsing json string '%s': \n", json_text_buffer);
    JsonValue *jval = json_parse(json_text_buffer, &err);
    if (!jval) {
        printf("ERROR : line:%d col:%d start:%d end:%d  %s\n",
            err.line, err.column, err.parse_start, err.parse_end -1, err.message);
    }
    else {
        json_value_str(jval);
        printf("\n");
    }

    assert(jval->type == JSON_ARRAY);
    const size_t num_monsters = jval->u.array.count;  // we add one for the null monster

    //todo mem migrate to long-term RO arena
    MonsterPrototypeArray *ma = (MonsterPrototypeArray *)calloc(1, sizeof(MonsterPrototypeArray) + sizeof(MonsterPrototype) * (num_monsters + 1 ));
    if (!ma) {
        jsonp_destroy();
        free(json_text_buffer);
        return ENOMEM;
    }
    // add the null monster object
    ma->monsters[0] = (MonsterPrototype){.name = strdup("(null)"), .id = 0};

    // populate our Monster[] with entries from the parsed json file
    ma->len = num_monsters + 1;
    for (int i = 0; i < num_monsters; ++i) {
        // we don't own the jason parser arena, so we have to make a copy of this string
        JsonValue *map = jval->u.array.elements[i];
        assert(map->type == JSON_OBJECT);

        JsonObjectEntry *id_joe = jsonp_entry_for_key(map, "id");
        JsonObjectEntry *name_joe = jsonp_entry_for_key(map, "name");
        JsonObjectEntry *ff_joe = jsonp_entry_for_key(map, "ff");


        //optional field
        const int ff = ff_joe ? (int)ff_joe->value->u.n_long : 0;

        char const *dup_str = strdup(name_joe->value->u.string);
        ma->monsters[i+1] = (MonsterPrototype){
            .name = dup_str,
            .id = (int)id_joe->value->u.n_long,
            .ferocity_factor = ff
        };
    }

    *monster_array_out = ma;

    jsonp_destroy();
    free(json_text_buffer);
    return 0;
}
