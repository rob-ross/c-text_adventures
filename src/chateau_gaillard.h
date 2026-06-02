// chateau_gaillard.h
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983
//
//
// Created by Rob Ross on 5/22/26.

#pragma once


#include "common/console_utils.h"

#include "mersenne_twister.h"
#include "rooms.h"
#include "monsters.h"
#include "objects.h"
#include "directions.h"
#include "parser.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <poll.h>
#endif

#include <sys/_types/_useconds_t.h>



constexpr bool CONTINUE_GAME = true;
constexpr bool END_GAME      = false;

char const * const VALID_COMMANDS = "HIQATRFPGNSEWUDLM12";




static int ROOM_GRAPH[NUM_ROOMS][RGINDEX_COUNT] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  //  NULL ROOM 0
//                                T   M   K
    {  1,  1,  2,  1,  1,  1,  0,  0,  0,  0 },  //  ROOM 1
    {  0, 29,  3,  1,  0,  0, 17,  0,  0,  0 },  //  ROOM 2
    {  0,  8,  4,  2,  0,  0,  0,  0,  0,  0 },  //  ROOM 3
    {  0,  9,  5,  3,  0,  0,  2,  0,  0,  0 },  //  ROOM 4
    {  5,  5,  5,  5,  5,  5,  0,  0,  0,  0 },  //  ROOM 5, DEATH
    {  0, 11,  7, 30,  0,  0,  0,  0,  0,  0 },  //  ROOM 6
    {  0,  0,  8,  6,  0,  0,  0,  0,  0,  0 },  //  ROOM 7
    {  3,  0,  0,  7,  0,  0,  0,  0, 17,  0 },  //  ROOM 8, KITCHEN, locked, need silver key
    {  4, 10,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 9
    {  9,  0,  0,  0,  0, 39,  0,  0,  0,  0 },  //  ROOM 10
    {  6,  0,  0,  0, 28,  0,  0,  0,  0,  0 },  //  ROOM 11
    {  0, 16, 13,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 12
    {  0,  0, 14, 12,  0,  0, 19,  0,  0,  0 },  //  ROOM 13
    {  0, 18,  0, 13,  0,  0,  0,  0,  0,  0 },  //  ROOM 14
    {  0, 21, 16, 12,  0,  0,  0,  0,  0,  0 },  //  ROOM 15
    { 12, 20, 19, 15,  0,  0,  0,  1,  0,  0 },  //  ROOM 16
    {  0,  0, 18,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 17
    { 14, 19, 31, 17,  0,  0,  0,  0,  0,  0 },  //  ROOM 18
    { 18, 23,  0, 16,  0,  0,  0,  0,  0,  0 },  //  ROOM 19
    { 16, 25,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 20
    { 15, 24,  0, 32,  0,  0,  0,  0,  0,  0 },  //  ROOM 21
    {  0, 26, 23, 20,  0,  0,  0,  0,  0,  0 },  //  ROOM 22
    { 19,  0,  0, 22,  0,  0,  0,  0,  0,  0 },  //  ROOM 23
    { 21,  0,  0,  0, 10,  0,  0,  0,  0,  0 },  //  ROOM 24
    { 20, 25, 25, 25, 25, 25,  0,  0,  0,  0 },  //  ROOM 25
    { 22,  0,  0,  0,  0, 33,  0,  0,  0,  0 },  //  ROOM 26
    {  0,  0,  0,  0,  0, 17,  0,  0,  0,  0 },  //  ROOM 27, ENTRANCE
    {  0,  0,  0,  0,  0, 11,  0,  0,  0,  0 },  //  ROOM 28, END ROOM
    { 29, 29, 29, 29, 29, 29,  0,  0,  0,  0 },  //  ROOM 29, DEATH
    { 30, 30, 30, 30, 30, 30,  0,  0,  0,  0 },  //  ROOM 30, DEATH
    { 31, 31, 31, 31, 31, 31,  0,  0,  0,  0 },  //  ROOM 31, DEATH
    { 32, 32, 32, 32, 32, 32,  0,  0,  0,  0 },  //  ROOM 32, DEATH
    { 43, 42, 40,  0, 26,  0,  0,  0,  0,  0 },  //  ROOM 33
    {  0, 38, 35,  0,  0,  0,  0,  0, 18,  0 },  //  ROOM 34, UNEVEN ROOM, locked, need golden key
    {  0, 43, 36, 34,  0,  0,  0,  0,  0,  0 },  //  ROOM 35
    {  0, 40, 37, 35,  0,  0,  1,  0,  0,  0 },  //  ROOM 36
    { 37, 37, 37, 37, 37, 37,  0,  0,  0,  0 },  //  ROOM 37, DEATH
    { 34,  0, 43, 39,  0,  0,  0,  0,  0,  0 },  //  ROOM 38
    {  0,  0, 38,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 39
    { 36, 41, 44, 33,  0,  0, 20,  0,  0,  0 },  //  ROOM 40
    { 40, 41, 41, 42, 41, 41,  0,  0,  0,  0 },  //  ROOM 41
    { 33, 42, 41, 42, 42, 42,  0,  0,  0,  0 },  //  ROOM 42
    { 35, 33,  0, 38,  0,  0,  0,  0,  0,  0 },  //  ROOM 43
    {  0,  0,  0, 40,  0,  0, 18,  0,  0,  0 },  //  ROOM 44
};


constexpr int NUM_OBJECTS = 21;  // todo (rob) make data driven

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





//// ------------------------------------------------------------
////
////    GAME STATE
////
//// ------------------------------------------------------------

constexpr int MAX_ITEMS = 5; // max number of items that can be carried

typedef struct GameState {
    const CharBuffer * player_name;
    uint32_t seed;
    // state for Mersenne Twister PRNG
    MTState mt_state;

    room_id room;      // current room
    room_id room_prev; // room user was in before this one
    room_id room_last_turn; // updates every turn, if user in same room as last turn, will be same as `room`
    int turns;
    int cash;

    int monsters_killed;  // number of monsters destroyed
    int monsters_fought;
    union {
        CharStats stats; // Named access: m.stats.strength
        union { CHAR_STATS_UNION_BODY }; // Anonymous access: m.strength & m.as_array
    };

    bool has_torch;

    bool is_dead;
    bool completed; // true if reached final room
    bool ended_by_quitting;
    object_id  items[MAX_ITEMS];  //
    bool rooms_visited[NUM_ROOMS];

    int QU;  // end-of-game flag? Quit flag, used in final scoring
    int BOX; // chest flag?


} GameState;


bool perform_action(GameState *gs, enum Command  cmd,  int arg1,  int arg2,  int arg3);