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



//// ------------------------------------------------------------
////
////    GAME STATE
////
//// ------------------------------------------------------------


// typedef struct GameState {
//     const CharBuffer * player_name;
//     uint32_t seed;
//     // state for Mersenne Twister PRNG
//     MTState mt_state;
//
//     room_id room;      // current room
//     room_id room_prev; // room user was in before this one
//     room_id room_last_turn; // updates every turn, if user in same room as last turn, will be same as `room`
//
//     int turns;
//     int cash;
//
//     int monsters_killed;  // number of monsters destroyed
//     int monsters_fought;
//
//     int magic;  // number of spells
//
//     union {
//         CharStats stats; // Named access: m.stats.strength
//         union { CHAR_STATS_UNION_BODY }; // Anonymous access: m.strength & m.as_array
//     };
//
//     bool has_torch;
//     bool completed; // true if reached final room
//     bool game_over;
//     bool is_dead;
//     bool ended_by_quitting;
//
//     bool must_fight; // true if user previously tried to retreat from monster and failed
//
//     object_id  items[MAX_ITEMS];  //
//
//     struct ObservationSpace {
//         // what the player can currently "see" in the environment that is not part of the game state model
//         bool     monster_is_visible;
//         bool     treasure_is_visible;
//         bool     must_fight; // Explicitly tell ML that movement/retreat is blocked
//         Monster  current_monster;
//         Object   current_treasure;
//         uint32_t legal_actions_mask; // Bitmask where each bit corresponds to VALID_COMMANDS
//     } perception;
//
//     double QU;  // end-of-game flag? Quit flag, used in final scoring
//     int SC;  // score bonus, depending on how game ends.
//     int BOX; // chest flag?
//
// } GameState;


bool perform_action(GameState *gs, enum Command  cmd,  int arg1,  int arg2,  int arg3);