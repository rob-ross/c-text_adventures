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
    bool is_visited_bit; // true if an actor has visited this room.
    bool is_outside_bit; // true if the room is "outside", i.e., exposed to nature, a clearing, a road, forrest

    object_id   objects[10];
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
    RGINDEX_TREASURE3,
    RGINDEX_COUNT
};

constexpr int ROOM_ERR_OBJECT_NOT_FOUND = -3;
constexpr int ROOM_ERR_ROOM_FULL = -2;
constexpr int ROOM_ERR_ALREADY_GOT_ONE_YOU_SEE_ITS_VERY_NICE =  -1;
constexpr int ROOM_SUCCESS = 0;


int room_init(size_t size, Room data[static size]);
void room_destroy();

int room_num_rooms(void);

int  room_add_object(const Room *room, int object_id);

bool room_clear_monster(room_id id);

bool room_contains_object(const Room *r, object_id id);

int  room_count_of_objects(const Room *r);

const Room * room_find_room(room_id id);

int  room_first_object_id(const Room *r);

int  room_index_for_object(const Room *r, int object_id );

bool room_is_full(const Room *r );

int  room_remove_object(const Room *room, int object_id);
void room_remove_all_objects(room_id id);

void room_repr(const Room *r);

void room_rooms_repr();

bool room_set_epilog(room_id id, RandomTextArray *rta);
bool room_set_prolog(room_id id, RandomTextArray *rta);

bool room_set_monster(const Room *r, monster_id id);

void room_set_visited_flag(const Room *r);

int room_count_visited();