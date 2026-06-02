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

typedef struct Room {
    [[maybe_unused]] const int id;
    [[maybe_unused]] char const * name;
    char const * desc;
    RandomTextArray  * preamble;
    RandomTextArray  * epilog;
    monster_id monster;
    [[deprecated]] Object   treasure;

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

constexpr int NUM_TREASURES     = 21;


constexpr int NUM_ROOMS          = 45;  // todo (rob) these values should be data driven
constexpr int NUM_DEATH_ROOMS    =  6;


constexpr int ROOM_START         = 27;
constexpr int ROOM_END           = 28;

// these constants are nice for static compiler checks but won't scale to a real world app. We're using these constants
// to add things to a room, (treasure, monster), exclude things from being added, check special conditions, e.g.,
// do you have the right key to unlock the door, etc. These should all be pushed into the data layer.
constexpr int ROOM_MAGICIAN      =  2;
constexpr int ROOM_MATTRESS      =  3;
constexpr int ROOM_WOODEN        =  4;
constexpr int ROOM_STONE         =  5;  // death
constexpr int ROOM_L_SHAPED      =  6;

constexpr int ROOM_KITCHEN       =  8;
constexpr int ROOM_CHARISMA_REDUCE = 13;
constexpr int ROOM_YELLOW        = 16;
constexpr int ROOM_CRAMPED       = 17;
constexpr int ROOM_TRAPPED       = 29;  // death
constexpr int ROOM_PIT_OF_FLAMES = 30;  // death
constexpr int ROOM_ACID          = 31;  // death
constexpr int ROOM_SPIDER        = 32;  // death
constexpr int ROOM_UNEVEN        = 34;

constexpr int ROOM_DUNGEON       = 36;
constexpr int ROOM_GARGOYLE      = 37;  // death
constexpr int ROOM_TROPHY        = 40;
constexpr int ROOM_SECRET_ROOM   = 41;

constexpr int ROOM_TURRET        = 44;

extern Room ROOMS[NUM_ROOMS];

const Room * room_find_room(room_id id);
int  room_add_object(Room *r, int object_id);
int  room_remove_object(Room *r, int object_id);
bool room_is_full(const Room *r );
int  room_index_for_object(const Room *r, int object_id );
int  room_count_of_objects(const Room *r);
int  room_first_object_id(const Room *r);
bool room_contains_object(const Room *r, object_id id);
void room_set_visited_flag(const Room *r);
void room_repr(const Room *r);
void room_rooms_repr();