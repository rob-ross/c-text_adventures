// mersenne_twister.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/11 19:52:00 PDT

//
// Created by Rob Ross on 5/11/26.
//

#pragma once

#include <stdint.h>

#include "base_types.h"


#define MT_NUM_STATES 624
typedef struct MTState
{
    u32 state_array[MT_NUM_STATES];         // the array for the state vector
    int state_index;                 // index into state vector array, 0 <= state_index <= n-1   always
} MTState;


// Must call to initialize and seed generator before calling mt_rand()
// seed can be nullptr
void mt_initialize_state(MTState* state, u32 seed);

// returns a random u32 in range 0 - (2^32)-1
u32 mt_random_uint32(MTState* state);

// return a random u32 in range [min_inclusive, max_exclusive)
u32 mt_rand_range(MTState* state, u32 min_inclusive, u32 max_exclusive);

// return a random u32 in range [min_inclusive, max_inclusive]
// This is a convenience function that calls mt_rand_range() and adds 1 to the `max` argument so that
// the bound is inclusive here
u32 mt_rand_range_inclusive(MTState* state, u32 min_inclusive, u32 max_inclusive);

// Function to generate a random float between 0.0f (inclusive) and 1.0f (exclusive)
float mt_random_float(MTState* state);

// Function to generate a random double between 0.0 (inclusive) and 1.0 (exclusive)
double mt_random_double(MTState* state);

