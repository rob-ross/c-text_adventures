// treasure.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:27:11 PDT


#pragma once

//// ------------------------------------------------------------
////
////    TREASURE
////
//// ------------------------------------------------------------

enum Item {
    ITEM_NULL [[maybe_unused]],
    ITEM_TORCH,
    ITEM_SILVER_KEY,
    ITEM_GOLD_KEY,
    ITEM_SWORD,
    ITEM_WAR_HAMMER,
    ITEM_CHAIN_MAIL,
    ITEM_SHIELD,
    ITEM_CLOAK,
    ITEM_WAND,
    ITEM_COUNT
};



// todo (rob) need a better name than Item or Object. It's a thingee you can pick up and carry.
// at least 3 categories; weapon, treasure, usable item: e.g., a key, rope, things you use to advance game state.
typedef struct Object {
    char const * const name;
    int                id;
    int                value;
} Object;