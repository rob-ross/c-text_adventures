// monsters.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/30 00:44:42 PDT


#include <stdio.h>
#include "common/string.h"
#include "common/files.c"
#include "monsters.h"


/*char const * const MONSTER_NAMES[NUM_MONSTERS] = {
    "NULL MONSTER",
    "Dwarf",
    "Monoceros",
    "Paradrus",
    "Vampyre",
    "Wrnach",
    "Giolla Dacker",
    "Kraken",
    "Fenris Wolf",
    "Calopus",
    "Basilisk",
    "Grimoire",
    "Flying Buffalo",
    "Ber Serkoid",
    "Wyrm",
    "Crowtherwood",
    "Gygax",
    "Ragnarok",
    "Fomorine",
    "Hafgygr",
    "Grendel",
};
*/

static Monster *monsters = nullptr;
static LenStrArray *global_lsa;

// reads a text file where each line is a string. This function will skip line comments and blank lines as well as
// multiline comments. Line comments start with '//' or '#' and multiline comments are C-style /* */
// Leading and trailing whitespace is trimmed.
static int monster_read_string_file(const char * monster_filename) {
    int err = process_file( monster_filename, create_string_array, (void**) &global_lsa);
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
    monsters = malloc(sizeof(Monster) * monsters_num_monsters());
    if (!monsters) {
        free_LenStrArray(global_lsa);
        global_lsa = nullptr;
        return ENOMEM;
    }
    return 0;
}

// Frees resources used by this module
void monsters_destroy(void) {
    free_LenStrArray(global_lsa);
    global_lsa = nullptr;
    free(monsters);
    monsters = nullptr;
}


int monsters_num_monsters(void) {
    return (int)global_lsa->size;
}

static Monster * pvt_monsters_find_monster(const monster_id id) {
    return &monsters[id];
}

// find_monster() will eventually use some better data structure, but we're using an internal array for now
// the pvt version is designed to return a non-const qualified Monster * so internal functions here can mutate it.
// the non-pvt version is intended for outside API use and should be const qualified. But for now, it's not because
// many methods are mutating the monsters. As we implement more service methods, we can eventually add const here
Monster * monsters_find_monster(const monster_id id) {
    return &monsters[id];
}

// overwrites the state of the monster object in storage for the argument's id member.
void monsters_update_monster(const Monster *m) {
    const int num_monsters = monsters_num_monsters();
    if (!m || m->id < 0 || m->id > num_monsters - 1 ) {
        return;
    }
    monsters[m->id] = *m;
}

void monsters_clear_all(void) {
    const int num_monsters = monsters_num_monsters();
    for (int i = 0; i < num_monsters; ++i) {
        monsters[i] = (Monster){};
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
        printf("'%s',\n", global_lsa->array[i].s);
    }
    printf("};\n");
}

const char * monsters_name_for_id(const monster_id id) {
    const size_t num_monsters = global_lsa->size;
    if (id < 0 || id > num_monsters - 1) return "null";

    return global_lsa->array[id].s;
}