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


typedef struct Treasure {
    char const * name;
    [[maybe_unused]] int treasure_index;
    int value;
} Treasure;