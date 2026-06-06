// rooms.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:28:34 PDT

#pragma once

#include <stddef.h>


#include "monsters.h"
#include "objects.h"

//// ------------------------------------------------------------
////
////    RANDOM TEXT
////
//// ------------------------------------------------------------

typedef struct RandomText {
    const char *text;  // displayed if chance_percent is satisfied
    const char *else_text; // if not null, displayed when chance_percent not satisfied
    // chance_percent: A random roll between 0 and 1 must be less (<) than this value for
    // `text` to be displayed, else `else_text` is displayed
    double chance_percent;  // between 0 and 1.
} RandomText;


typedef struct RandomTextArray {
    size_t length;
    RandomText lines[];  // flexible array
} RandomTextArray;


//// ------------------------------------------------------------
////
////    ROOMS
////
//// ------------------------------------------------------------

typedef int monster_id;
typedef int room_id;

// size of the objects[] array in struct Room
// this must be >= MAX_ROOM_OBJECTS
constexpr int ROOM_OBJECTS_CAPACITY = 10;

// users of this library may read from objects of this type. But
// the user must not create any objects of this type and pass them to any methods
// declared here. The library must manage the lifecycles of these objects.
typedef struct Room {
    int id;
    char const * name;
    char const * desc;
    RandomTextArray  * preamble;
    RandomTextArray  * epilog;
    monster_id monster;

    // flags. eventually can be bit flags for efficiency
    bool is_lit_bit; // The room has light, thus can be seen without a light source like a torch
    // todo (rob) is_visited_bit: when we implement multiple actors, this bit has to be moved to the actor object
    bool is_visit_started_bit; // true when actor has entered the room, but not yet completed a full turn
    bool is_visited_bit; // true after the actor has been in the room an entire turn.
    bool is_outside_bit; // true if the room is "outside", i.e., exposed to nature, a clearing, a road, forrest
    int         objects_len;
    object_id   objects[ROOM_OBJECTS_CAPACITY]; // a swap-and-pop list of size objects_len
} Room;

// RoomGraphIndex: maps to array indices in ROOM_GRAPH, so these must not be reordered or renumbered!
enum RoomGraphIndex {
    RGINDEX_NORTH,
    RGINDEX_SOUTH  [[maybe_unused]],
    RGINDEX_EAST   [[maybe_unused]],
    RGINDEX_WEST   [[maybe_unused]],
    RGINDEX_UP     [[maybe_unused]],
    RGINDEX_DOWN,
    RGINDEX_TREASURE,
    RGINDEX_MONSTER,
    RGINDEX_REQUIRED_KEY,
    RGINDEX_UNUSED,
    RGINDEX_COUNT
};

// ROOM_GRAPH: transition graph, room (node) to other rooms via directed edges
extern int ROOM_GRAPH[][RGINDEX_COUNT];  // defined in the main TUs, e.g., chateau_gaillard.c, citadel_of_pershu.c, etc.

constexpr int ROOM_ERR_OBJECT_ID_OUT_OF_BOUNDS = -4;
constexpr int ROOM_ERR_OBJECT_NOT_FOUND = -3;
constexpr int ROOM_ERR_ROOM_FULL = -2;
constexpr int ROOM_ERR_ALREADY_GOT_ONE_YOU_SEE_ITS_VERY_NICE =  -1;
constexpr int ROOM_SUCCESS = 0;


int room_init(size_t size, Room data[static size]);
void room_destroy();


int  room_add_object(const Room *room, int id);
int  room_remove_object(const Room *room, int object_id);
void room_remove_all_objects(room_id id);

bool room_clear_monster(const Room *r);
bool room_contains_object(const Room *r, object_id id);
int  room_count_of_objects(const Room *r);
int  room_count_visited();

// todo (rob) where does this method belong?
// we free it in this TU so it makes sense to create it here too.
RandomTextArray * create_rta(int length);

const Room * room_find_room(room_id id);
const Object * room_find_object_named(const Room *r, char const partial_name[static 1]);

// Returns the object id of the first object in the room, or ROOM_ERR_OBJECT_NOT_FOUND if there are no items
int  room_first_object_id(const Room *r);

int  room_index_for_object(const Room *r, int object_id );

bool room_is_full(const Room *r );
bool room_is_empty(const Room *r);
int  room_num_rooms(void);


void room_repr(const Room *r);
void room_all_rooms_repr();

const char * room_rgindex_label(enum RoomGraphIndex rg_index);
const char * room_rgindex_label_short(enum RoomGraphIndex rg_index);

bool room_set_epilog(room_id id, RandomTextArray *rta);
bool room_set_preamble(room_id id, RandomTextArray *rta);

bool room_set_monster(const Room *r, monster_id id);
void room_set_visited_flag(const Room *r);
void room_set_visit_started_flag(const Room *r);
