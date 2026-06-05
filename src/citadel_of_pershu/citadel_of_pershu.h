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

#include "../common/console_utils.h"
#include "../mersenne_twister.h"
#include "../rooms.h"
#include "../monsters.h"
#include "../objects.h"
#include "../directions.h"

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

char const * const VALID_COMMANDS = "HIQATRFPGNSEWUDLM12";



static int ROOM_GRAPH[][RGINDEX_COUNT] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  NULL ROOM 0

    {  1,  4,  1,  8,  0,  0,  0,  0 },  //  ROOM 1
    {  0,  5,  3,  0,  0,  0,  0,  0 },  //  ROOM 2
    {  3,  7,  3,  2,  0,  0,  0,  0 },  //  ROOM 3
    {  1,  0,  5,  0,  0,  0,  2,  0 },  //  ROOM 4
    {  2,  0,  0,  4,  0,  0,  0,  0 },  //  ROOM 5
    {  0,  0,  7,  0,  0,  0,  1,  0 },  //  ROOM 6, ENTRANCE
    {  3, 14, 15,  6,  0,  0,  0,  0 },  //  ROOM 7
    {  1,  8,  8,  8,  0,  0,  0,  0 },  //  ROOM 8
    { 10, 11,  0,  0,  0,  0,  0,  0 },  //  ROOM 9
    {  0,  0, 11,  9,  0,  0,  0,  0 },  //  ROOM 10
    {  9, 13, 12, 10,  0,  0,  0,  0 },  //  ROOM 11
    {  0,  0,  0, 11,  0,  0,  0,  0 },  //  ROOM 12
    { 11, 16,  0, 44,  0,  0,  0,  0 },  //  ROOM 13
    {  7,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 14
    {  7, 45,  0, 12,  0,  0,  0,  0 },  //  ROOM 15
    {  0, 19,  0, 17,  0, 37,  0,  0 },  //  ROOM 16
    {  0,  0, 16,  0,  0,  0,  0,  0 },  //  ROOM 17
    {  0, 30,  0,  0,  0, 34,  0,  0 },  //  ROOM 18
    { 16, 28,  0,  0,  0, 43,  0,  0 },  //  ROOM 19
    {  0, 31, 22,  0,  0,  0,  0,  0 },  //  ROOM 20
    {  0, 23,  0, 45,  0,  0,  3,  0 },  //  ROOM 21
    {  0, 24,  0, 20,  0,  0,  0,  0 },  //  ROOM 22
    { 21, 25,  0,  0,  0,  0,  0,  0 },  //  ROOM 23
    { 22,  0, 25,  0,  0,  0,  0,  0 },  //  ROOM 24
    { 23, 27, 30, 24,  0,  0,  0,  0 },  //  ROOM 25
    {  0, 29, 27,  0,  0,  0,  0,  0 },  //  ROOM 26
    { 25,  0,  0, 26,  0,  0,  0,  0 },  //  ROOM 27
    { 19, 28, 28, 28,  0, 47,  0,  0 },  //  ROOM 28
    { 26, 29, 29, 29,  0,  0,  0,  0 },  //  ROOM 29
    { 18,  0,  0, 25,  0,  0,  0,  0 },  //  ROOM 30
    { 20,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 31, END ROOM
    {  0,  0, 34,  0,  0, 47,  0,  0 },  //  ROOM 32
    { 34, 36,  0, 35,  0,  0,  0,  0 },  //  ROOM 33
    { 34, 33, 34, 32, 18,  0,  0,  0 },  //  ROOM 34
    { 33, 38, 36,  0,  0,  0,  0,  0 },  //  ROOM 35
    { 33, 39, 46, 35,  0,  0,  0,  0 },  //  ROOM 36
    {  0, 40,  0,  0, 16,  0,  0,  0 },  //  ROOM 37
    { 35,  0,  0,  0,  0, 41,  0,  0 },  //  ROOM 38
    { 36, 39, 40, 39,  0,  0,  0,  0 },  //  ROOM 39
    { 37,  0,  0, 39,  0,  0,  0,  0 },  //  ROOM 40
    {  0,  0, 42,  0, 38,  0,  0,  0 },  //  ROOM 41
    { 42, 43, 42, 41,  0, 47,  0,  0 },  //  ROOM 42
    {  0,  0, 42,  0, 19,  0,  0,  0 },  //  ROOM 43

        // Death rooms
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  DEATH BY DROWNING
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  DEATH BY BURNING
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  DEATH BY FREEZING
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  BOTTOMLESS PIT

};

//// ------------------------------------------------------------
////
////    GAME STATE
////
//// ------------------------------------------------------------

typedef struct GameState {
    const CharBuffer * player_name;
    uint32_t seed;
    // state for Mersenne Twister PRNG
    MTState mt_state;

    int room;  // current room
    room_id room_prev; // room user was in before this one
    room_id room_last_turn; // updates every turn, if user in same room as last turn, will be same as `room`

    int turns; // 1 point per turn
    int cash;

    int monsters_killed;  // number aliens/androids destroyed
    int monsters_fought;

    int magic;  // number of spells

    union {
        CharStats stats; // Named access: m.stats.strength
        union { CHAR_STATS_UNION_BODY }; // Anonymous access: m.strength & m.as_array
    };

    bool has_torch;
    bool is_dead;
    bool completed; // true if reached final room
    bool must_fight; // true if user previously tried to retreat from monster and failed

    int  items[ITEM_COUNT];  // first 9 items of Treasure have a slot here with the same index

    struct ObservationSpace {
        // what the player can currently "see" in the environment that is not part of the game state model
        bool     monster_is_visible;
        bool     treasure_is_visible;
        bool     must_fight; // Explicitly tell ML that movement/retreat is blocked
        Monster  current_monster;
        Object   current_treasure;
        uint32_t legal_actions_mask; // Bitmask where each bit corresponds to VALID_COMMANDS
    } perception;
} GameState;


//// ------------------------------------------------------------
////
////    GLOBALS
////
//// ------------------------------------------------------------


struct GlobalState {
    const char * player_name;
    bool silent_mode;
    uint32_t char_sleep_duration;
};
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
