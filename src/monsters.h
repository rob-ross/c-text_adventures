// monsters.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:38:00 PDT

#pragma once



#include "attribute_stats.h"
#include "common/base_types.h"

typedef struct Room Room;

//// ------------------------------------------------------------
////
////    MONSTERS
////
//// ------------------------------------------------------------




typedef struct monster_prototype_s {
    char const * name;
    monster_id id;
    int ferocity_factor;
    union {
        CharStats stats; // Named access: m.stats.strength
        union { CHAR_STATS_UNION_BODY }; // Anonymous access: m.strength or m.as_array[StatIndex]
    };
} MonsterPrototype;

typedef struct spawned_monster_s {
    MonsterPrototype m;
    int entity_id;
    room_id location;
} SpawnedMonster;

typedef struct monster_prototype_array_s {
    u32 len;
    MonsterPrototype monsters[];  // flexible array
} MonsterPrototypeArray;


int monsters_init(const char * monster_filename);
void monsters_destroy(void);
int monsters_num_monsters(void);

bool monsters_monster_is_in_room( const char *monster_name, const Room *r );
const char * monsters_name_for_id(monster_id id);
MonsterPrototype * monsters_find_monster(monster_id id);
void monsters_update_monster(const MonsterPrototype *m);
// initializes all monster objects to 0 state
void monsters_clear_all(void);

void monsters_all_repr();
void monsters_repr(monster_id id);
