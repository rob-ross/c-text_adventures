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

#include "mersenne_twister.h"
#include "rooms.h"
#include "monsters.h"
#include "treasure.h"


constexpr int NUM_ROOMS         = 48;
constexpr int NUM_DEATH_ROOMS   =  4;
constexpr int NUM_TREASURES     = 19;
constexpr int NUM_MONSTERS      = 20;

constexpr int START_ROOM                =  6;
constexpr int END_ROOM                  = 31;
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
char const * const VALID_DIRECTIONS = "NSEWUD";


//// ------------------------------------------------------------
////
////    DIRECTIONS
////
//// ------------------------------------------------------------

enum Direction {
    DIRECTION_ERR = -1,
    DIRECTION_NORTH = 0,
    DIRECTION_SOUTH,
    DIRECTION_EAST,
    DIRECTION_WEST,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_COUNT
};

// direction in "NSEWUD"
static inline enum Direction calc_direction_index(char const direction_char) {
    switch (toupper(direction_char)) {
        case 'N': return DIRECTION_NORTH;
        case 'S': return DIRECTION_SOUTH;
        case 'E': return DIRECTION_EAST;
        case 'W': return DIRECTION_WEST;
        case 'U': return DIRECTION_UP;
        case 'D': return DIRECTION_DOWN;
        default:  return DIRECTION_ERR;
    }
}

static inline const char * direction_string(const enum Direction direction_index) {
    switch (direction_index) {
        case DIRECTION_ERR:   return "ERROR";
        case DIRECTION_NORTH: return "NORTH";
        case DIRECTION_SOUTH: return "SOUTH";
        case DIRECTION_EAST:  return "EAST";
        case DIRECTION_WEST:  return "WEST";
        case DIRECTION_UP:    return "UP";
        case DIRECTION_DOWN:  return "DOWN";
        default:              return "UNKNOWN";
    }
}

// Currently a bad move in a direction results in the same message each time, as defined below
// todo (rob) do we want to make this more dynamic?
static char const * const BAD_MOVE_DESC[DIRECTION_COUNT] = {
    "NO EXIT THAT WAY",
    "THERE IS NO EXIT SOUTH",
    "YOU CANNOT GO IN THAT DIRECTION",
    "IN THAT WAY LIES MADNESS",
    "THERE IS NO WAY UP FROM HERE",
    "YOU CANNOT DESCEND FROM HERE",
};

static char const * const MONSTER_NAMES[NUM_MONSTERS] = {
    "NULL MONSTER",
    "Swashbuckler", "Werebear",
    "Caecliae", //   ???
    "Manticore", // the face of a human, the body of a lion, and the tail of a scorpion
    "Vampire",
    "Predebeast", // ???
    "Gargoyle",
    "Medusae",  // plural of Medusa, a jellyfish
    "Magi", // plural of Magus, priests in Zoroastrianism and earlier Iranian religions.
    "Fire Lizard",
    "Phase Spider", // ??? D&D A phase spider possesses the magical ability to phase in and out of the Ethereal Plane.
    "Troll", "Hell Hound", "Frost Giant", "Necromancer",
    "Hydra of 10 Heads", // ???
    "Patriach", // ???
    "Master Thief", "Living Statue"
};


// first 9 elements are items the user can use, carry, or drop (and pick up again.)
// From Emeralds and higher, these are treasure that are converted to a cash equivalent
static char const * const TREASURE_NAMES[NUM_TREASURES] = {
    "NULL TREASURE", "Flaming Torch", "Silver Key", "Gold Key", "Sword", "War Hammer", "Chain Mail Armor", "Shield",
    "Cloak of Protection", "Wand of Fireballs",

    "Emeralds", "Silver Rings", "Elven Amethysts", "Diamond Dragon Eyes", "Crystal Ball", "Pieces of Eight",
    "Elemental Gems", "Shape-Shifting Stones", "Gold Doubloons"
};


// todo (rob) We should just store the full description on a single line of text (no embedded newlines), and let the
// display argument paginate the text as needed for the display. Currently we use the same line breaks as in the original
// BASIC app.
static Room ROOMS[NUM_ROOMS] = {
{.id =  0,  .name= "NULL ROOM",  .desc = "NULL ROOM"},
{.id =  1,  .name= "ROOM 1",  .desc = "An underground river flows swiftly by."},
{.id =  2,  .name= "ROOM 2",  .desc = "You are in the Citadel's food storage area.\nOld cheeses and black loaves of bread can\nbe seen, as well as many sacks of supplies."},
{.id =  3,  .name= "ROOM 3",  .desc = "You are in the Citadel's kitchen. A Huge\njoint of meat turns slowly over a raging\nfire. Doors lead into cupboards, as well\nas to the west and to the south."},
{.id =  4,  .name= "ROOM 4",  .desc = "This is the Central Library. Leather-bound\nvolumes line the walls, right up to the\nornately carved ceiling."},
{.id =  5,  .name= "ROOM 5",  .desc = "This room is an awful mess. It used to be\nan artist's studio. Paint and old\neasels lie around the floor."},
{.id =  6,  .name= "ROOM 6",  .desc = "This is the entrance to the Citadel of Pershu.\nTurn now, if you wish. Many stronger than you\nhave taken fright at its menacing towers and\ndark portals. If you wish to proceed, move\neast towards the black gaping doorway."},
{.id =  7,  .name= "ROOM 7",  .desc = "A stone altar stands in the middle of the room\nwith two dead candles on it. An old book lies\non one part of the altar top, and a faded red\nparchment cloth covers the front of it."},
{.id =  8,  .name= "ROOM 8",  .desc = "You stand high on the black tower, the\nCitadel stretches to the north, south\nand east of you.\nThere is only one way out."},
{.id =  9,  .name= "ROOM 9",  .desc = "You are in the northern section of the\nCitadel's large wine cellar. Heavy\nbarrels lie all around you in this end\nof the cellar. There is a door to the north\nand one to the south."},
{.id = 10,  .name= "ROOM 10", .desc = "You are in the west wing of the wine\ncellar. There is a door to the west and\none to the east. The central circular\npart of the cellar lies beyond the\neast door."},
{.id = 11,  .name= "ROOM 11", .desc = "You are in the central circular\narea of the wine cellar. There is\na door at each compass point."},
{.id = 12,  .name= "ROOM 12", .desc = "You are in the east section of the\nwine cellar. There is a door to the\nwest and one - which you cannot use,\nas it only allows entrance to where\nyou now stand - to the east."},
{.id = 13,  .name= "ROOM 13", .desc = "There are many, many wine bottles here\nlying on their sides in this southern\nsection of the wine cellar. There is a\ndark, unfriendly-looking hole to the west\nand doors to the north and to the south."},
{.id = 14,  .name= "ROOM 14", .desc = "This is the Citadel's armory. Row upon row\nof shiny suits of armor are stored here." },
{.id = 15,  .name= "ROOM 15", .desc = "You are in the ruler's bedchamber.\nA large fire burns in the south of\nthe room, with a small door beside\nit. Other exits are to the north\nand to the west." },
{.id = 16,  .name= "ROOM 16", .desc = "Sand covers the floor of this curious\nroom, heaped into drifts.\nBy peeping over the 'dunes' you can\nsee a golden passage way leads to the\nwest, and there is a door to the south. You are not sure whether or not you\nhave seen all the exits." },
{.id = 17,  .name= "ROOM 17", .desc = "You are in the picture gallery. Portraits\nof long-dead princes line all of the\nwalls. The room is dominated by a huge\nlandsape, hanging above the exit to the\neast which leads, via the gold passage way\nback to that curious room of sand." },
{.id = 18,  .name= "ROOM 18", .desc = "You are on a remote tower balcony.\nThere are stairs here." },
{.id = 19,  .name= "ROOM 19", .desc = "You walk beneath a stone archway.\nYou can only walk north or south\nunless you decide to take the stairs." },
{.id = 20,  .name= "ROOM 20", .desc = "This vast hall has a marble floor, and\nthe slightest sound echos violently.\nThere are purple drapes concealing\nthe exits from this hall." },
{.id = 21,  .name= "ROOM 21", .desc = "You are in the glove storeroom.\nThe west door radiates heat.\nAnother door leads to the south." },
{.id = 22,  .name= "ROOM 22", .desc = "You are in the silver crosses storeroom.\nThere are only two exits." },
{.id = 23,  .name= "ROOM 23", .desc = "You are in the amulet storeroom.\nDoors lead north and south." },
{.id = 24,  .name= "ROOM 24", .desc = "You are in the kazoo storeroom.\nThere are two exits." },
{.id = 25,  .name= "ROOM 25", .desc = "You are in the satchel storeroom." },
{.id = 26,  .name= "ROOM 26", .desc = "You are in the storeroom for wooden\nboxes... There are two exits." },
{.id = 27,  .name= "ROOM 27", .desc = "This is where printed vases are\nstored... As you can easily see." },
{.id = 28,  .name= "ROOM 28", .desc = "The heavy air of this area seems to make\nyour torch very dim. You can hardly see\nthat air is rushing up from somewhere.\nYou can just make out that this area must\nbe a mine of some sort." },
{.id = 29,  .name= "ROOM 29", .desc = "You appear to be in an endless labyrinth,\nlined with paintings.........\nWhichever way you turn, there seems to be\nmore tunnels, all lined with paintings." },
{.id = 30,  .name= "ROOM 30", .desc = "This is the southern tower of the Citadel." },
{.id = 31,  .name= "ROOM 31", .desc = "Well done, you have managed to find the exit.\nTake a deep breath of good, clean air..........." },
{.id = 32,  .name= "ROOM 32", .desc = "This room is filled with swirling smoke,\nso you cannot see... Air rushes past a\nstatue of the goddess Diana. This\nmust be the Citadel's meditation chamber." },
{.id = 33,  .name= "ROOM 33", .desc = "A small forked bridge crosses a stream here.\nYou can move north, south, or west." },
{.id = 34,  .name= "ROOM 34", .desc = "You are in a rough stone cavern. Stairs\nlead up from here.\nThere is also a single door which\nleads away from the cavern." },
{.id = 35,  .name= "ROOM 35", .desc = "This is the former Citadel underground\nstable. It smells terrible." },
{.id = 36,  .name= "ROOM 36", .desc = "You find yourself in an underground\ncourtyard. Strange, twisted trees are\naround you, and a wind of incredible\ncoldness blows from the east." },
{.id = 37,  .name= "ROOM 37", .desc = "This is the Oracle Room, although the\nmystic voice has not spoken for many\nyears." },
{.id = 38,  .name= "ROOM 38", .desc = "Horrors. A cold shudder passes through you as you\nrealize this is the priests' sacrifice room.\nDried-up blood is on the floor and a\nskull grins at you from high on the wall." },
{.id = 39,  .name= "ROOM 39", .desc = "Old straw mattresses and rings chained to the\nwall tell you this was the Citadel's dungeon. The dungeon seems to stretch forever, with many\nsmall partitioned areas...." },
{.id = 40,  .name= "ROOM 40", .desc = "You are in a small alcove, with a solid\ngray granite throne in the middle of it." },
{.id = 41,  .name= "ROOM 41", .desc = "This is the orc's guardroom, way below\nthe ground. A stairwell ends here and\na door leads to the east." },
{.id = 42,  .name= "ROOM 42", .desc = "There is a healing pool here, with a\ndangerous, swirling area of water." },
{.id = 43,  .name= "ROOM 43", .desc = "The Underpriests of Odric used this\ntiny hall for their forbidden worship\neons ago. It is an unpleasant area,\nso you are thrilled to see a set of\nstone stairs." },

{.id = 44,  .name= "DEATH BY DROWNING",  .desc = "Water covers your head.\nYou are drowning.\nGLUG... GASP............" },
{.id = 45,  .name= "DEATH BY BURNING",  .desc = "The flames strike at you...\nas you slowly burn to death..."},
{.id = 46,  .name= "DEATH BY FREEZING", .desc = "You are hit by a freezing spell\nand turn into a block of perpetual\nliving stone. This is the end."},
{.id = 47,  .name= "BOTTOMLESS PIT",    .desc = "You tumble down a bottomless pit.\nDown, down, down..."},

};



static int ROOM_GRAPH[NUM_ROOMS][RGINDEX_COUNT] = {
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
    const char * player_name;
    int room;  // current room
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

    // state for Mersenne Twister PRNG
    MTState mt_state;

    int  items[ITEM_COUNT];  // first 9 items of Treasure have a slot here with the same index
    bool rooms_visited[NUM_ROOMS];

    struct ObservationSpace {
        // what the player can currently "see" in the environment that is not part of the game state model
        bool     monster_is_visible;
        bool     treasure_is_visible;
        bool     must_fight; // Explicitly tell ML that movement/retreat is blocked
        Monster  current_monster;
        Treasure current_treasure;
        uint32_t legal_actions_mask; // Bitmask where each bit corresponds to VALID_COMMANDS
    } perception;
} GameState;


//// ------------------------------------------------------------
////
////    GLOBALS
////
//// ------------------------------------------------------------

// usleep() takes argument in microseconds
// these are equivalent milliseconds
constexpr uint32_t _10ms = 10'000; // NOLINT(*-reserved-identifier)
constexpr uint32_t _15ms = 15'000; // NOLINT(*-reserved-identifier)
constexpr uint32_t _30ms = 30'000; // NOLINT(*-reserved-identifier)

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
