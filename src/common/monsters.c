// monsters.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/19 01:40:00 PDT


/*
 *  This TU currently couples the functions of the Monster methods with the loading of JSON data to initialize
 *  the Monster data structures. The JSON loader and parser should probably be moved to a data loader module.
 *  Then the init() method could be passed the already initialized monster data.
 *  This is the classic "inversion of control."
 */


#include "monsters.h"
#include "files.c"

#include <assert.h>
#include <stdlib.h>


static MonsterPrototypeArray *monster_prototypes_array = nullptr;  // this acts like a singleton in this monster library
// we always spawn the same number of monsters in reset(). This will change as the game evolves
static SpawnedMonster *monsters = nullptr;
static int next_monster_id = 0; // "entity id" for monsters.




int monsters_init(MonsterPrototypeArray *mpa) {
    monster_prototypes_array = mpa;
    if ( ! monsters) {
        // show error message here?
        return -1;
    }
    return 0;
}

// Frees resources used by this module
void monsters_destroy(void) {
    const u32 num_monsters = monster_prototypes_array->len;

    // monster names were copied from JSON parser arena via strdup, so we must free them
    for ( size_t i = 0; i < num_monsters ; ++i) {
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
    if (id < 0 || (u64)id > (u64)monster_prototypes_array->len - 1 ) {
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
    for ( size_t i = 0; i < monster_prototypes_array->len; ++i) {
        monsters_repr(i);
    }
}

const char * monsters_name_for_id(const monster_id id) {
    const size_t num_monsters = monster_prototypes_array->len;
    if (id < 0 || (u64)id > (u64)num_monsters - 1) return "null";
    return monster_prototypes_array->monsters[id].name;
}
