// chateau_gaillard.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/03 15:38:58 PDT

// chateau_gaillard.h
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983
//
//
// Created by Rob Ross on 5/22/26.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "../adventure_shared.h"
#include "../directions.h"
#include "../common/console_utils.h"
#include "../mersenne_twister.h"
#include "../rooms.h"
#include "../monsters.h"
#include "../objects.h"
#include "../parser.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <poll.h>
#endif

#include <sys/_types/_useconds_t.h>



constexpr bool CONTINUE_GAME = true;
constexpr bool END_GAME      = false;

char const * const VALID_COMMANDS = "HIQATRFPGNSEWUDLM12";



constexpr int OBJECT_AXE           =  1;
constexpr int OBJECT_SWORD         =  2;
constexpr int OBJECT_DAGGER        =  3;
constexpr int OBJECT_MACE          =  4;
constexpr int OBJECT_QUARTER_STAFF =  5;
constexpr int OBJECT_MORNING_STAR  =  6;
constexpr int OBJECT_FALCHION      =  7;
constexpr int OBJECT_AMULET        =  9;

constexpr int OBJECT_MYSTIC_SCROLL = 12;
constexpr int OBJECT_HEALING_POTION = 13;

constexpr int OBJECT_DIADEM      = 16;
constexpr int OBJECT_SILVER_KEY  = 17;
constexpr int OBJECT_GOLD_KEY    = 18;
constexpr int OBJECT_STONE_CHEST = 19;
constexpr int OBJECT_IRON_CHEST  = 20;

constexpr int NUM_TREASURES     = 21;

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

constexpr int MONSTER_DWARF = 1;

const int MAX_ROOM_OBJECTS =  10; //maximum number of items that can be placed in a room
const int MAX_PLAYER_OBJECTS = 5; // max number of items that can be carried


bool perform_action(GameState *gs, enum Command  cmd,  int arg1,  int arg2,  int arg3);