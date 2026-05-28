// rooms.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:28:34 PDT



#pragma once
#include <stddef.h>

#include "monsters.h"
#include "treasure.h"

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