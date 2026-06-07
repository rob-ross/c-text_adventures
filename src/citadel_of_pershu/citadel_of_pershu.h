// citadel_of_pershu.h
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983
//
//
// Created 2026/05/15 01:35:01 PDT

#pragma once

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <poll.h>
#endif

#include <sys/_types/_useconds_t.h>

#include "../adventure_shared.h"
#include "../directions.h"
#include "../common/console_utils.h"
#include "../mersenne_twister.h"
#include "../rooms.h"
#include "../monsters.h"
#include "../objects.h"

constexpr int NUM_DEATH_ROOMS   =  4;
constexpr int NUM_TREASURES     = 19;

constexpr int ROOM_START                =  6;
constexpr int ROOM_END                  = 31;
constexpr int LIBRARY_ROOM              = 4;
constexpr int WINE_CELLAR_EAST          = 12;
constexpr int BEDCHAMBER_ROOM           = 15;
constexpr int MARBLE_HALL               = 20;
constexpr int GLOVE_STOREROOM           = 21;
constexpr int SILVER_CROSSES_STOREROOM  = 22;
constexpr int DROWNING_ROOM             = 44;


constexpr bool CONTINUE_GAME = true;
constexpr bool END_GAME      = false;

char const * const VALID_COMMANDS = "HIQACLRFTPNSEWUDM123";

const int MAX_ROOM_OBJECTS = 1; //maximum number of items that can be placed in a room
const int MAX_PLAYER_OBJECTS = 9; // max number of items that can be carried

/*

// Exit guard data structures for managing dynamic edges, e.g., user must have a particular item in order to
// travel west.... or needs a key to unlock a door, etc.
// Guards provide checking mechanism, and desc_altered can have a separate track of descriptions to use after
// the guard is met, I.e., Room desc originally says 'The door to the west is locked.' After the guard is met,
// 'There is an unlocked door leading west.'

enum GameFlag {
     FLAG_NONE = 0,
     FLAG_BEDCHAMBER_UNLOCKED,
     FLAG_SILVER_STOREROOM_UNLOCKED,
     FLAG_COUNT
 };

struct ExitGuard {
    enum Direction direction;
    enum Item required_item;   // Item needed to trigger the change
    enum GameFlag sets_flag;   // Flag to set once triggered
    const char *fail_msg;      // What to say if they don't have the item
};

struct Room {
    int id;
    char const * name;
    char const * desc;
    char const * desc_altered; // Description to show once a specific flag is set
    enum GameFlag desc_flag;   // Which flag triggers the alternate description

    struct ExitGuard *guards;  // Array of guards
    size_t num_guards;

    // ... rest of your struct ...
};

static bool process_move_command(struct GameState * gs, char const first_letter) {
    const int location = gs->room;
    const enum Direction dir = calc_direction_index(first_letter);
    struct Room *current_room = &ROOMS[location];

    // 1. Check if there is a guard on this exit
    for (size_t i = 0; i < current_room->num_guards; i++) {
        struct ExitGuard *g = &current_room->guards[i];

        if (g->direction == dir && !gs->flags[g->sets_flag]) {
            // Check if player has the key
            if (gs->items[g->required_item]) {
                display_line("You unlock the door with your key!");
                gs->flags[g->sets_flag] = true;
                // Optionally consume the key if it's one-time use
            } else {
                display_line(g->fail_msg);
                return false; // Movement blocked
            }
        }
    }



 *
 */
