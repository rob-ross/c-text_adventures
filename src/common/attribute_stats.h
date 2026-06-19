// attribute_stats.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/19 12:23:08 PDT

// attribute_stats.h
//
// Created by Rob Ross on 5/22/26.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//// ------------------------------------------------------------
////
////    CHARACTER / MONSTER STATS
////
//// ------------------------------------------------------------

enum StatIndex {
    STAT_NULL [[maybe_unused]],
    STAT_STRENGTH,
    STAT_CHARISMA,
    STAT_DEXTERITY,
    STAT_INTELLIGENCE,
    STAT_WISDOM,
    STAT_CONSTITUTION,
    STAT_COUNT // Useful for loops and array sizing
};

// Just the raw integer fields
#define CHAR_STATS_LIST   \
int null_stat;        \
int strength;         \
int charisma;         \
int dexterity;        \
int intelligence;     \
int wisdom;           \
int constitution;

// The union logic that maps the array to the fields
#define CHAR_STATS_UNION_BODY     \
int as_array[STAT_COUNT];     \
struct { CHAR_STATS_LIST };

typedef struct CharStats {
    union { CHAR_STATS_UNION_BODY };
} CharStats;

#ifdef __cplusplus
}
#endif

