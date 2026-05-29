// rooms.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:28:34 PDT



#pragma once
#include <stddef.h>

#include "monsters.h"
#include "room_objects.h"

//// ------------------------------------------------------------
////
////    RANDOM TEXT
////
//// ------------------------------------------------------------

typedef struct RandomText {
    const char *text;  // displayed if chance_percent is satisfied
    const char *else_text; // if not null, displayed when chance_percent not satisfied
    double chance_percent;  // between 0 and 1. Random number between 0 and 1  must be less (<) than this to be displayed
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

typedef struct Room {
    [[maybe_unused]] const int id;
    [[maybe_unused]] char const * name;
    char const * desc;
    RandomTextArray  * preamble;
    RandomTextArray  * epilog;
    Monster  monster;
    Object   treasure;

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
    RGINDEX_TREASURE2,
    RGINDEX_TREASURE3,
    RGINDEX_COUNT
};

constexpr int ROOM_ERR_OBJECT_NOT_FOUND = -3;
constexpr int ROOM_ERR_ROOM_FULL = -2;
constexpr int ROOM_ERR_ALREADY_GOT_ONE_YOU_SEE_ITS_VERY_NICE =  -1;
constexpr int ROOM_SUCCESS = 0;


int  room_add_object(Room *r, int object_id);
int  room_remove_object(Room *r, int object_id);
bool room_is_full(const Room *r );
int  room_index_for_object(const Room *r, int object_id );
int  room_count_of_objects(const Room *r);
int  room_first_object_index(const Room *r);