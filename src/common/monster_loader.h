// monster_loader.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/19 01:29:52 PDT

//
// Created by Rob Ross on 6/19/26.
//

#ifndef TEXT_ADVENTURES_MONSTER_LOADER_H
#define TEXT_ADVENTURES_MONSTER_LOADER_H

#include "monsters.h"


int monster_read_json_file(const char * monster_json_filename, MonsterPrototypeArray **mpa_out);


#endif //TEXT_ADVENTURES_MONSTER_LOADER_H
