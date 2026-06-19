// monster_loader.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/19 01:29:52 PDT

//
// Created by Rob Ross on 6/19/26.
//

#include "monster_loader.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "files.h"
#include "monsters.h"
#include "../roblib/json_parser/json_parser.h"


// -----------------------------------------------------------------
//      Forward References
// -----------------------------------------------------------------
static int load_monster_json_file(FILE *fptr, void **monster_array_out);


int monster_read_json_file(const char * monster_json_filename, MonsterPrototypeArray **mpa_out) {
    // `load_monster_json_file` is the worker method here; `process_file` just ensures the file is closed properly
    int err = process_file( monster_json_filename, load_monster_json_file, (void**) mpa_out);
    return err;
}



/*
 *  When we are parsing a JSON file for reading in information about a type of entity (Monsters here),
 *  this parsing code has to understand the structure of the JSON file to know what data to pull out and
 *  where to save that data (i.e., what structs and members to use.)
 *  In Python, we'd just keep attributes for a monster in a dict and look up keys as we needed certain values, like
 *  'strength' or 'name.' In C, we need to have well-defined blocks of memory with those values so we know
 *  what pointer offsets to use to get that data.
 *  Thus, a "dynamic" JSON loader in C is not going to result in a performant data structure in the "C way" of
 *  doing things.
 *  Still, rather than having a long stream of if-statements that check if a key exists in the JSON object and then
 *  assign the key's value to the right struct member, we can define a mapping between key names and offsets into
 *  the struct. We simplify the algorithm logic at the expense of having to create the correct metadata structure
 *  for the key-offset mappings.
 *  This is a test of this concept.
 */

typedef struct {
    const char* label;       // The string name (e.g., "id", "name", "ff")
    size_t offset;           // The offset calculated via offsetof()
    json_type type;     // The type for safe casting
} MemberMetadata;

// each struct we are populating from JSON data requires one of these arrays.
// The json_value knows the type of each key, so we don't actually need the type field!
// static const MemberMetadata monster_schema[] = {
//     { "name", offsetof(MonsterPrototype, name),             JSON_STRING },
//     { "id",   offsetof(MonsterPrototype, id),               JSON_INT    },
//     { "ff",   offsetof(MonsterPrototype, ferocity_factor),  JSON_INT    },
//     { nullptr,   0,                                0           } // Sentinel to mark end of array
// };

#define REFLECT(struct_type, field, type_enum) \
{ #field, offsetof(struct_type, field), type_enum }

// The 'type' field in MemberMetadata probably needs to be based on the C types of the struct members.
static const MemberMetadata monster_schema[] = {
    REFLECT(MonsterPrototype, name, JSON_STRING),
    REFLECT(MonsterPrototype, id,   JSON_INT),
    REFLECT(MonsterPrototype, ferocity_factor,   JSON_INT),  // we need the ability to use nicknames here, like ff?
    { nullptr, 0, 0 }
};

void set_struct_value(MonsterPrototype *m, const char *field_name, JsonValue *value_ptr) {
    for (int i = 0; monster_schema[i].label != nullptr; i++) {
        if (strcmp(monster_schema[i].label, field_name) == 0) {

            // Calculate the exact destination address in memory
            void *dest = (char *)m + monster_schema[i].offset;

            switch (monster_schema[i].type) {
                case JSON_INT:
                    // wrinkle. the JSON parser stores int values as long, but we can't force every struct to
                    // use long ints. For now, casting to int seems safe and reasonable; However, this means numbers
                    // in the JSON that are outside the int range will be truncated. We note this, but also note the
                    // domain of these values thus far fits in an int. I.e., no monsters have stats that require a
                    // long to represent them.
                    *(int *)dest = (int)value_ptr->u.n_long;
                    break;
                case JSON_FLOAT:
                    *(double *)dest = value_ptr->u.n_double;
                    break;
                case JSON_BOOLEAN:
                    *(bool *)dest = value_ptr->u.boolean;
                    break;
                case JSON_STRING:
                    // The string from the JSON parser arena needs to be duplicated for long-term storage.
                    *(char const **)dest = strdup(value_ptr->u.string);
                    break;
                case JSON_NULL:
                    // for config data we don't expect to see null, but if we do, we'll just ignore it
                    break;
                case JSON_NUMBER:
                    // the parser will never use this type, as we use either double or long for actual variables
                    break;
                case JSON_ARRAY:
                    // todo perhaps we can automate nested structs here, but I'm dubious. Currently the parsing code
                    // will decide into what variables an array or object get placed into.
                case JSON_OBJECT:
                    break;
            }
            return;
        }
    }
    fprintf(stderr, "Field '%s' not found in schema.\n", field_name);
}

// parses the JSON file from the argument stream pointer and extracts monster objects into a Monster struct instance
// returns the result in the out ptr, a *Monster
int load_monster_json_file(FILE *fptr, void **monster_array_out) {
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

    // printf("\nParsing JSON string '%s': \n", json_text_buffer);

    JsonValue *jval = json_parse(json_text_buffer, &err);

    /*if (!jval) {
        printf("ERROR : line:%d col:%d start:%d end:%d  %s\n",
            err.line, err.column, err.parse_start, err.parse_end -1, err.message);
    }
    else {
        json_value_str(jval);
        printf("\n");
    }*/

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

    // populate our Monster[] with entries from the parsed JSON file
    ma->len = num_monsters + 1;
    for ( size_t i = 0; i < num_monsters; ++i) {
        // we don't own the jason parser arena, so we have to make a copy of this string
        JsonValue *map = jval->u.array.elements[i];
        assert(map->type == JSON_OBJECT);

        // old method, explicitly looking up keys
        // JsonObjectEntry *id_joe = jsonp_entry_for_key(map, "id");
        // JsonObjectEntry *name_joe = jsonp_entry_for_key(map, "name");
        // JsonObjectEntry *ff_joe = jsonp_entry_for_key(map, "ff");


        JsonObjectEntry **entries = map->u.object.entries;
        for ( size_t j = 0; j < map->u.object.count; ++j) {
            //set_struct_value loops over every member field in the MemberMetadata[], which makes the
            // combined operation O(N^2). If either the MemberMetadata entries or the
            // JsonObjectEntry entries used a hash map, where lookups were O(1), then this operation
            // becomes O(N). We could also forego a hashmap if we mandated that fields in the JSON
            // must be in the same order as fields in the struct. We can still have optional (i.e, missing)
            // members in the JSON file; we would just skip over those when iterating the MemberMetadata[].
            // That would again make this O(N) at the cost of some flexibility in the JSON file data.
            // I think it's preferable to allow out-of-order JSON object members, so we should
            // adopt a hash table for one of these. Since we're parsing in a JSON object which represents
            // a map already, we should add that function there. Then we would iterate over elements of
            // MemberMetadata[] in order and then look up each member field name in the JSON object via its
            // hash map.
            set_struct_value(&ma->monsters[i+1], entries[j]->key, entries[j]->value);
        }


        // //optional field
        // const int ff = ff_joe ? (int)ff_joe->value->u.n_long : 0;
        //
        // char const *dup_str = strdup(name_joe->value->u.string);
        // ma->monsters[i+1] = (MonsterPrototype){
        //     .name = dup_str,
        //     .id = (int)id_joe->value->u.n_long,
        //     .ferocity_factor = ff
        // };
    }

    *monster_array_out = ma;

    jsonp_destroy();
    free(json_text_buffer);
    return 0;
}
