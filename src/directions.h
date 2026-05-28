// directions.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 05:10:40 PDT



#pragma once
#include <ctype.h>

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
static inline enum Direction calc_direction_index(int const direction_char) {
    switch (toupper(direction_char)) {
        case 'N': return DIRECTION_NORTH;
        case 'S': return DIRECTION_SOUTH;
        case 'E': return DIRECTION_EAST;
        case 'W': return DIRECTION_WEST;
        case 'U': return DIRECTION_UP;
        case 'D': return DIRECTION_DOWN;
        default:  return DIRECTION_ERR;
    }
}

static inline const char * direction_string(const enum Direction direction_index) {
    switch (direction_index) {
        case DIRECTION_ERR:   return "ERROR";
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
    "NO EXIT THAT WAY",
    "THERE IS NO EXIT SOUTH",
    "YOU CANNOT GO IN THAT DIRECTION",
    "IN THAT WAY LIES MADNESS",
    "THERE IS NO WAY UP FROM HERE",
    "YOU CANNOT DESCEND FROM HERE",
};