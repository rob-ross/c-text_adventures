// directions.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 05:10:40 PDT



#pragma once
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

char const * const VALID_DIRECTIONS = "NSEWUD";

//// ------------------------------------------------------------
////
////    DIRECTIONS
////
//// ------------------------------------------------------------

enum Direction {
    DIRECTION_ERR = -1,
    DIRECTION_NONE = 0,
    DIRECTION_NORTH,
    DIRECTION_SOUTH,
    DIRECTION_EAST,
    DIRECTION_WEST,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_COUNT
};

// direction in "NSEWUD"
static inline enum Direction calc_room_graph_direction_index(int const direction_char) {
    switch (toupper(direction_char)) {
        case 'N': return DIRECTION_NORTH - 1;
        case 'S': return DIRECTION_SOUTH - 1;
        case 'E': return DIRECTION_EAST  - 1;
        case 'W': return DIRECTION_WEST  - 1;
        case 'U': return DIRECTION_UP    - 1;
        case 'D': return DIRECTION_DOWN  - 1;
        default:  return DIRECTION_ERR      ;
    }
}

static inline const char * direction_string(const enum Direction direction_index) {
    switch (direction_index) {
        case DIRECTION_ERR:   return "ERROR";
        case DIRECTION_NONE:  return "NONE";
        case DIRECTION_NORTH: return "NORTH";
        case DIRECTION_SOUTH: return "SOUTH";
        case DIRECTION_EAST:  return "EAST";
        case DIRECTION_WEST:  return "WEST";
        case DIRECTION_UP:    return "UP";
        case DIRECTION_DOWN:  return "DOWN";
        default:              return "UNKNOWN";
    }
}


// Currently a bad move in a direction results in the same message each time, as defined below
// todo (rob) do we want to make this more dynamic?
static char const * const BAD_MOVE_DESC[DIRECTION_COUNT] = {
    "There is no exit that way.",
    "There is no exit south.",
    "You cannot go in that direction.",
    "There is no way west.",
    "There is no way up from here.",
    "You cannot descend from here.",
};

#ifdef __cplusplus
}
#endif
