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
#include "treasure.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <poll.h>
#endif

#include <sys/_types/_useconds_t.h>


constexpr int NUM_ROOMS          = 45;  // todo (rob) these values should be data driven
constexpr int NUM_DEATH_ROOMS    =  6;

constexpr int NUM_TREASURES     = 21;
constexpr int NUM_MONSTERS      = 21;

constexpr int ROOM_START         = 27;
constexpr int ROOM_END           = 28;

// these constants are nice for static compiler checks but won't scale to a real world app. We're using these constsants
// to add things to a room, (treasure, monster), exclude things from being added, check special conditions, e.g.,
// do you have the right key to unlock the door, etc. These should all be pushed into the data layer.
constexpr int ROOM_EERIE         =  2;
constexpr int ROOM_MATTRESS      =  3;
constexpr int ROOM_WOODEN        =  4;
constexpr int ROOM_STONE         =  5;  // death
constexpr int ROOM_L_SHAPED      =  6;

constexpr int ROOM_KITCHEN       =  8;
constexpr int ROOM_MIRROR        = 13;
constexpr int ROOM_YELLOW        = 16;
constexpr int ROOM_CRAMPED       = 17;
constexpr int ROOM_TRAPPED       = 29;  // death
constexpr int ROOM_PIT_OF_FLAMES = 30;  // death
constexpr int ROOM_ACID          = 31;  // death
constexpr int ROOM_SPIDER        = 32;  // death
constexpr int ROOM_UNEVEN        = 34;

constexpr int ROOM_GARGOYLE      = 37;  // death
constexpr int ROOM_TROPHY        = 40;
constexpr int ROOM_TURRET        = 44;

constexpr bool CONTINUE_GAME = true;
constexpr bool END_GAME      = false;

/*
 *  Word wrap notes:
 *      We don't break words. Only wrap whole words. If word ends with punctuation like period or comma or semicolon,
 *      and punctuation makes the word long enough to wrap, keep it on the same line even if it exceeds max length by 3 chars max for an ellipses.
 *      If first characters of a new line is whitespace, eat it.
 *      A newline will force a new line with an extra line before the new line. Think of newline as new paragraph
 *
 */

static Room ROOMS[NUM_ROOMS] = {
{.id =  0,  .name= "NULL ROOM",  .desc = "" },
{.id =  1,  .name= "ROOM 1",     .desc = "You are out on the battlements of the Chateau. There is only one way back." },
{.id =  2,  .name= "ROOM 2",     .desc = "This is an eerie room, where once magicians consorted with evil sprites and werebeasts. Exits lead in three directions. An evil smell comes from the south." },
{.id =  3,  .name= "ROOM 3",     .desc = "An old straw mattress lies in one corner. It has been ripped apart to find any treasure which was hidden in it. Light comes fitfully from a window to the north, and around the doors to south, east, and west." },
{.id =  4,  .name= "ROOM 4",     .desc = "This wooden-panelled room makes you feel damp and uncomfortable. There are three doors leading from this room, one made of iron. Your sixth sense warns you to choose carefully..." },
{.id =  5,  .name= "ROOM 5",     .desc = "You ignore your intuition... A Spell of Living Stone, primed to trap the first intruder has been set on you. With your last seconds of life you have time only to feel profound regret..." },
{.id =  6,  .name= "ROOM 6",     .desc = "You are in an L-shaped room. Heavy parchment lines the walls. You can see through an archway to the east, but that is not the only exit from this room." },
{.id =  7,  .name= "ROOM 7",     .desc = "There is an archway to the west, leading to an L-shaped room. A door leads in the opposite direction." },
{.id =  8,  .name= "ROOM 8",     .desc = "This must be the Chateau's main kitchen, but any food left here has long rotted away. A door leads to the north, and there is one to the west." },
{.id =  9,  .name= "ROOM 9",     .desc = "You find yourself in a small room, which makes you feel claustrophobic. There is a picture of a black dragon painted on the north wall, above the door." },
{.id = 10,  .name= "ROOM 10",    .desc = "A stairwell ends in this 'room', which is more of a landing than an actual room. The door to the north is made of iron, which has rusted over the centuries." },
{.id = 11,  .name= "ROOM 11",    .desc = "There is a stone archway to the north. You are in a very long room. You are in a very long room.\nFresh air blows down some stairs and rich red drapes cover the walls. You can see doors to the south and east." },
{.id = 12,  .name= "ROOM 12",    .desc = "You have entered a room filled with swirling, choking smoke. You must leave quickly to remain healthy enough to continue your chosen quest." },
{.id = 13,  .name= "ROOM 13",    .desc = "There is a mirror in the corner. You glance at it, and feel suddenly very ill.\nYou realize the looking-glass has been infused with a Spell of Charisma Reduction... oh dear...." },
{.id = 14,  .name= "ROOM 14",    .desc = "This room is richly finished with a white marble floor. Strange footprints lead to the two doors from this room. Dare you follow them?" },
{.id = 15,  .name= "ROOM 15",    .desc = "You are in a long, long hallway, lined on each side with rich, red drapes.\nThey are parted halfway down the east wall where there is a door." },
{.id = 16,  .name= "ROOM 16",    .desc = "Someone has spent a long time painting this room a bright yellow.\nYou remember reading that yellow is the Ancient Oracle's Color of Warning..." },
{.id = 17,  .name= "ROOM 17",    .desc = "As you stumble down the ladder you fall into the room. The ladder crashes down behind you. There is now no way back.\nA small door leads east from this very cramped room." },
{.id = 18,  .name= "ROOM 18",    .desc = "You find yourself in the Hall of Mirrors, and see yourself reflected a hundred times or more. Through the bright glare you can make out doors in all directions. You notice the mirrors around the east door are heavily tarnished." },
{.id = 19,  .name= "ROOM 19",    .desc = "You find yourself in a long corridor... Your footsteps echo as you walk." },
{.id = 20,  .name= "ROOM 20",    .desc = "You feel as if you've been wandering around this Chateau forever, and you begin to despair of ever escaping.\nStill, you can't get too depressed but must struggle on. Looking around, you see that you are in a room which has a heavy timbered ceiling and white roughly-finished walls.\nThere are two doors..." },
{.id = 21,  .name= "ROOM 21",    .desc = "You are in a small alcove. You look around, but can see nothing in the gloom. Perhaps if you wait a while your eyes will adjust to the murky dark of this alcove." },
{.id = 22,  .name= "ROOM 22",    .desc = "A dried-up fountain stands in the center of this courtyard, which once held beautiful flowers but have have long since died." },
{.id = 23,  .name= "ROOM 23",    .desc = "The scent of dying flowers fills this brightly-lit room.\nThere are two exits from it." },
{.id = 24,  .name= "ROOM 24",    .desc = "This is a round stone cavern off the side of the alcove to your north." },
{.id = 25,  .name= "ROOM 25",    .desc = "You are in an enormous circular room, which looks as if it was used as a games room. Rubble covers the floor, partially blocking the only exit." },
{.id = 26,  .name= "ROOM 26",    .desc = "Through the dim mustiness of this small potting shed you can see a stairwell." },
{.id = 27,  .name= "ROOM 27",    .desc = "You begin this Adventure in a small wood outside the Chateau.\nWhile out walking one day, you come across a small, ramshackle shed in the woods. Entering it, you see a hole in one corner. An old ladder leads down from the hole." },
{.id = 28,  .name= "ROOM 28",    .desc = "How wonderful! Fresh air, sunlight, birds are singing. You are free at last." },
{.id = 29,  .name= "ROOM 29",    .desc = "The smell came from bodies rotting in huge traps. One springs shut on you, trapping you forever!" },
{.id = 30,  .name= "ROOM 30",    .desc = "You fall into a pit of flames." },
{.id = 31,  .name= "ROOM 31",    .desc = "Aaaaahhh... you have fallen into a pool of acid. Now you know - too late - why the mirrors were so badly tarnished." },
{.id = 32,  .name= "ROOM 32",    .desc = "It's too bad you chose that exit from the alcove. A giant funnel-web spider leaps on you, and before you can react, bites you on the neck. You have 10 seconds to live." },
{.id = 33,  .name= "ROOM 33",    .desc = "A stairwell leads into this room, a poor and common hovel with many doors and exits." },
{.id = 34,  .name= "ROOM 34",    .desc = "It is hard to see in this room and you slip slightly on the uneven, rocky floor." },
{.id = 35,  .name= "ROOM 35",    .desc = "Horrors! This room was once the torture chamber of the Chateau.\nSkeletons lie on the floor, still with chains around their bones." },
{.id = 36,  .name= "ROOM 36",    .desc = "Another room with very unpleasant memories.\nThis foul hole was used as the Chateau dungeon." },
{.id = 37,  .name= "ROOM 37",    .desc = "Oh no, this is a gargoyle's lair. It has been held prisoner here for three hundred years.\nIn his frenzy he thrashes out at you and... breaks your neck!!" },
{.id = 38,  .name= "ROOM 38",    .desc = "This was the Lower Dancing Hall. With doors to the north, the east, and to the west, you would seem to be able to flee any danger." },
{.id = 39,  .name= "ROOM 39",    .desc = "This is a dingy pit at the foot of some extremely dubious-looking stairs. A door leads to the east." },
{.id = 40,  .name= "ROOM 40",    .desc = "Doors open to each compass point from the Trophy Room of the Chateau.\nThe heads of strange creatures shot by the ancestral owners are mounted high up on each wall." },
{.id = 41,  .name= "ROOM 41",    .desc = "You have stumbled on to a secret room.\nDown here, eons ago, the ancient Necromancers of Thorin plied their evil craft... and the remnant of their spells hangs heavy on the air." },
{.id = 42,  .name= "ROOM 42",    .desc = "Cobwebs brush your face as you make your way through the gloom of this room of shadows." },
{.id = 43,  .name= "ROOM 43",    .desc = "This gloomy passage lies at the intersection of three rooms." },
{.id = 44,  .name= "ROOM 44",    .desc = "You are in the rear turret room, below the extreme western wall of the ancient Chateau." },
};

static int ROOM_GRAPH[NUM_ROOMS][RGINDEX_COUNT] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  //  NULL ROOM 0

    {  1,  1,  2,  1,  1,  1,  0,  0,  0,  0 },  //  ROOM 1
    {  0, 29,  3,  1,  0,  0, 17,  0,  0,  0 },  //  ROOM 2
    {  0,  8,  4,  2,  0,  0,  0,  0,  0,  0 },  //  ROOM 3
    {  0,  9,  5,  3,  0,  0,  2,  0,  0,  0 },  //  ROOM 4
    {  5,  5,  5,  5,  5,  5,  0,  0,  0,  0 },  //  ROOM 5
    {  0, 11,  7, 30,  0,  0,  1,  0,  0,  0 },  //  ROOM 6
    {  0,  0,  8,  6,  0,  0,  0,  0,  0,  0 },  //  ROOM 7
    {  3,  0,  0,  7,  0,  0, 99,  0,  0,  0 },  //  ROOM 8
    {  4, 10,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 9
    {  9,  0,  0,  0,  0, 39,  0,  0,  0,  0 },  //  ROOM 10
    {  6,  0,  0,  0, 28,  0,  0,  0,  0,  0 },  //  ROOM 11
    {  0,  6, 13,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 12
    {  0,  0, 14, 12,  0,  0, 19,  0,  0,  0 },  //  ROOM 13
    {  0, 18,  0, 13,  0,  0,  0,  0,  0,  0 },  //  ROOM 14
    {  0, 21, 16, 12,  0,  0,  0,  0,  0,  0 },  //  ROOM 15
    { 12, 20, 19, 15,  0,  0,  0,  1,  0,  0 },  //  ROOM 16
    {  0,  0, 18,  0, 27,  0,  0,  0,  0,  0 },  //  ROOM 17
    { 14, 19, 31, 17,  0,  0,  0,  0,  0,  0 },  //  ROOM 18
    { 18, 23,  0, 16,  0,  0,  0,  0,  0,  0 },  //  ROOM 19
    { 16, 25,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 20
    { 15, 24,  0, 32,  0,  0,  0,  0,  0,  0 },  //  ROOM 21
    {  0, 26, 23, 20,  0,  0,  0,  0,  0,  0 },  //  ROOM 22
    { 19,  0,  0, 22,  0,  0,  0,  0,  0,  0 },  //  ROOM 23
    { 21,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 24
    { 20, 25, 25, 25, 25, 25,  0,  0,  0,  0 },  //  ROOM 25
    { 22,  0,  0,  0,  0, 33,  0,  0,  0,  0 },  //  ROOM 26
    {  0,  0,  0,  0,  0, 17,  0,  0,  0,  0 },  //  ROOM 27, ENTRANCE
    {  0,  0,  0,  0,  0, 11,  0,  0,  0,  0 },  //  ROOM 28, END ROOM
    { 29, 29, 29, 29, 29, 29,  0,  0,  0,  0 },  //  ROOM 29
    { 30, 30, 30, 30, 30, 30,  0,  0,  0,  0 },  //  ROOM 30
    { 31, 31, 31, 31, 31, 31,  0,  0,  0,  0 },  //  ROOM 31
    { 32, 32, 32, 32, 32, 32,  0,  0,  0,  0 },  //  ROOM 32
    { 43, 42, 40,  0, 26,  0,  0,  0,  0,  0 },  //  ROOM 33
    {  0, 38, 35,  0,  0,  0,100,  0,  0,  0 },  //  ROOM 34
    {  0, 43, 36, 34,  0,  0,  0,  0,  0,  0 },  //  ROOM 35
    {  0, 40, 37, 35,  0,  0,  0,  0,  0,  0 },  //  ROOM 36
    { 37, 37, 37, 37, 37, 37,  0,  0,  0,  0 },  //  ROOM 37
    { 34,  0, 43, 39,  0,  0,  0,  0,  0,  0 },  //  ROOM 38
    {  0,  0, 38,  0, 10,  0,  0,  0,  0,  0 },  //  ROOM 39
    { 36, 41, 44, 33,  0,  0, 20,  0,  0,  0 },  //  ROOM 40
    { 40, 41, 41, 42, 41, 41,  0,  0,  0,  0 },  //  ROOM 41
    { 33, 42, 41, 42, 42, 42,  0,  0,  0,  0 },  //  ROOM 42
    { 35, 33,  0, 38,  0,  0,  0,  0,  0,  0 },  //  ROOM 43
    {  0,  0,  0, 40,  0,  0, 18,  0,  0,  0 },  //  ROOM 44
};

// todo (rob) need a better name than Item or Object. It's a thingee you can pick up and carry.
// at least 3 categories; weapon, treasure, usable item: e.g., a key, rope, things you use to advance game state.
typedef struct Object {
    char const * const name;
    int                id;
    int                value;
} Object;

constexpr int NUM_OBJECTS = 21;  // todo (rob) make data driven

static Object OBJECTS[NUM_OBJECTS] = {
    {.id =  0, .name="NULL OBJECT" },
    {.id =  1, .name="axe" },
    {.id =  2, .name="sword" },
    {.id =  3, .name="dagger" },
    {.id =  4, .name="mace" },
    {.id =  5, .name="quarterstaff" },
    {.id =  6, .name="morning star" },
    {.id =  7, .name="falchion" },
    {.id =  8, .name="crystal ball", .value=99 },
    {.id =  9, .name="amulet", .value=247 },
    {.id = 10, .name="ebony ring", .value=166 },
    {.id = 11, .name="gems", .value=462 },
    {.id = 12, .name="mystic scroll", .value=195 },
    {.id = 13, .name="healing potion", .value=231 },
    {.id = 14, .name="dilithium crystals", .value=162 },
    {.id = 15, .name="copper pieces", .value=27 },
    {.id = 16, .name="diadem", .value=141 },
    {.id = 17, .name="silver key" },
    {.id = 18, .name="golden key" },
    {.id = 19, .name="chest of stone" },
    {.id = 20, .name="chest made of iron" },
};

constexpr int OBJECT_AXE         =  1;
constexpr int OBJECT_SWORD       =  2;
constexpr int OBJECT_DIADEM      = 16;
constexpr int OBJECT_SILVER_KEY  = 17;
constexpr int OBJECT_GOLD_KEY    = 18;
constexpr int OBJECT_STONE_CHEST = 19;
constexpr int OBJECT_IRON_CHEST  = 20;



static char const * const MONSTER_NAMES[NUM_MONSTERS] = {
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

constexpr int MONSTER_DWARF = 1;


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
    bool rooms_visited[NUM_ROOMS];

    int QU;  // end-of-game flag?
    int BOX; // chest flag?

} GameState;