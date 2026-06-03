// monsters.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/30 00:44:42 PDT


#include <stdio.h>
#include "common/string.h"
#include "monsters.h"


char const * const MONSTER_NAMES[NUM_MONSTERS] = {
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

static Monster monsters[NUM_MONSTERS];


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
    if (!m || m->id < 0 || m->id > NUM_MONSTERS - 1 ) {
        return;
    }
    monsters[m->id] = *m;
}

void monsters_clear_all(void) {
    for (int i = 0; i < NUM_MONSTERS; ++i) {
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
    printf("MONSTER_NAMES[%d] {\n", NUM_MONSTERS);
    for (int i = 0; i < NUM_MONSTERS; ++i) {
        printf("'%s',\n", MONSTER_NAMES[i]);
    }
    printf("};\n");
}

const char * monsters_name_for_id(const monster_id id) {
    if (id < 0 || id > NUM_MONSTERS - 1) return "null";

    const Monster *m =  pvt_monsters_find_monster(id);
    if (!m || !m->name) return "null";

    return m->name;
}