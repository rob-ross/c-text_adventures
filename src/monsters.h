// monsters.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:38:00 PDT

#pragma once

#include "attribute_stats.h"



//// ------------------------------------------------------------
////
////    MONSTERS
////
//// ------------------------------------------------------------

constexpr int NUM_MONSTERS      = 21;

constexpr int MONSTER_DWARF = 1;

extern char const * const MONSTER_NAMES[NUM_MONSTERS];


typedef struct Monster {
    char const * name;
    [[maybe_unused]] int monster_index;
    int ferocity_factor;
    union {
        CharStats stats; // Named access: m.stats.strength
        union { CHAR_STATS_UNION_BODY }; // Anonymous access: m.strength or m.as_array[StatIndex]
    };
} Monster;


void monsters_repr(void);