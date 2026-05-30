// monsters.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/30 00:44:42 PDT


#include <stdio.h>
#include "monsters.h"

char const * const MONSTER_NAMES[NUM_MONSTERS] = {
    "NULL MONSTER",
    "Dwarf",
    "Monoceros",
    "Paradrus",
    "Vampyre",
    "Wrnach",
    "Giolla Dacker",
    "Kraken",
    "Fenris Wolf",
    "Calopus",
    "Basilisk",
    "Grimoire",
    "Flying Buffalo",
    "Ber Serkoid",
    "Wyrm",
    "Crowtherwood",
    "Gygax",
    "Ragnarok",
    "Fomorine",
    "Hafgygr",
    "Grendel",
};

void monsters_repr(void) {
    printf("MONSTER_NAMES[%d] {\n", NUM_MONSTERS);
    for (int i = 0; i < NUM_MONSTERS; ++i) {
        printf("'%s',\n", MONSTER_NAMES[i]);
    }
    printf("};\n");
}
