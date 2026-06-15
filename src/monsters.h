// monsters.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:38:00 PDT

#pragma once


#include "attribute_stats.h"
#include "rooms.h"

typedef struct Room Room;

//// ------------------------------------------------------------
////
////    MONSTERS
////
//// ------------------------------------------------------------




typedef int monster_id;

typedef struct monster_s {
    char const * name;
    monster_id id;
    int ferocity_factor;
    union {
        CharStats stats; // Named access: m.stats.strength
        union { CHAR_STATS_UNION_BODY }; // Anonymous access: m.strength or m.as_array[StatIndex]
    };
} Monster;


typedef struct monster_array_s {
    uint32_t len;
    Monster monsters[];  // flexible array
} MonsterArray;


int monsters_init(const char * monster_filename);
void monsters_destroy(void);
int monsters_num_monsters(void);
void monsters_names_repr(void);
bool monsters_monster_is_in_room( const char *monster_name, const Room *r );
const char * monsters_name_for_id(monster_id id);
Monster * monsters_find_monster(monster_id id);
void monsters_update_monster(const Monster *m);
// initializes all monster objects to 0 state
void monsters_clear_all(void);