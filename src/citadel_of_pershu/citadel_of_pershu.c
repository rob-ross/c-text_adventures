// citadel_of_pershu.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/03 14:02:21 PDT

// citadel_of_pershu.c
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983
//
//
// Created 2026/05/15 01:35:01 PDT

/*

MAKE :

cd /Users/robross/Documents/Development/CLionProjects/citadel_of_pershu/text_adventures/src

 * DEBUG *


clang -g -DCITADEL_OF_PERSHU_MAIN -fsanitize=address -fsanitize=leak -Wall -Werror \
    -Wno-unused-const-variable -Wno-unused-variable -Wno-unused-function \
    -std=c23 -o citadel_of_pershu.out citadel_of_pershu.c  \
            ../adventure_shared.c           \
            ../mersenne_twister.c           \
            ../common/console_utils.c       \
            ../parser.c                     \
            ../rooms.c                      \
            ../objects.c                    \
            ../monsters.c                   \
            ../common/string.c              \
            ../roblib/string/string_utils.c \
            ../roblib/string/string_builder.c \
            ../roblib/json_parser/json_parser.c \
            ../roblib/json_parser/arena.c   \
            ../roblib/json_parser/error_result.c

*/




#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "../adventure_shared.h"
#include "../directions.h"
#include "../common/console_utils.h"
#include "../mersenne_twister.h"
#include "../rooms.h"
#include "../monsters.h"
#include "../objects.h"


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

constexpr int NUM_DEATH_ROOMS   =  4;
constexpr int NUM_TREASURES     = 19;

constexpr int ROOM_START                =  6;
constexpr int ROOM_END                  = 31;
constexpr int LIBRARY_ROOM              =  4;
constexpr int WINE_CELLAR_EAST          = 12;
constexpr int BEDCHAMBER_ROOM           = 15;
constexpr int MARBLE_HALL               = 20;
constexpr int GLOVE_STOREROOM           = 21;
constexpr int SILVER_CROSSES_STOREROOM  = 22;
constexpr int DROWNING_ROOM             = 44;


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


int ROOM_GRAPH[][RGINDEX_COUNT] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  //  NULL ROOM 0
//                                T   M   K
    {  1,  4,  1,  8,  0,  0,  0,  0,  0,  0 },  //  ROOM 1
    {  0,  5,  3,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 2
    {  3,  7,  3,  2,  0,  0,  0,  0,  0,  0 },  //  ROOM 3
    {  1,  0,  5,  0,  0,  0,  2,  0,  0,  0 },  //  ROOM 4
    {  2,  0,  0,  4,  0,  0,  0,  0,  0,  0 },  //  ROOM 5
    {  0,  0,  7,  0,  0,  0,  1,  0,  0,  0 },  //  ROOM 6, ENTRANCE
    {  3, 14, 15,  6,  0,  0,  0,  0,  0,  0 },  //  ROOM 7
    {  1,  8,  8,  8,  0,  0,  0,  0,  0,  0 },  //  ROOM 8
    { 10, 11,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 9
    {  0,  0, 11,  9,  0,  0,  0,  0,  0,  0 },  //  ROOM 10
    {  9, 13, 12, 10,  0,  0,  0,  0,  0,  0 },  //  ROOM 11
    {  0,  0,  0, 11,  0,  0,  0,  0,  0,  0 },  //  ROOM 12
    { 11, 16,  0, 44,  0,  0,  0,  0,  0,  0 },  //  ROOM 13
    {  7,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 14
    {  7, 45,  0, 12,  0,  0,  0,  0,  0,  0 },  //  ROOM 15
    {  0, 19,  0, 17,  0, 37,  0,  0,  0,  0 },  //  ROOM 16
    {  0,  0, 16,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 17
    {  0, 30,  0,  0,  0, 34,  0,  0,  0,  0 },  //  ROOM 18
    { 16, 28,  0,  0,  0, 43,  0,  0,  0,  0 },  //  ROOM 19
    {  0, 31, 22,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 20
    {  0, 23,  0, 45,  0,  0,  3,  0,  0,  0 },  //  ROOM 21
    {  0, 24,  0, 20,  0,  0,  0,  0,  0,  0 },  //  ROOM 22
    { 21, 25,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 23
    { 22,  0, 25,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 24
    { 23, 27, 30, 24,  0,  0,  0,  0,  0,  0 },  //  ROOM 25
    {  0, 29, 27,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 26
    { 25,  0,  0, 26,  0,  0,  0,  0,  0,  0 },  //  ROOM 27
    { 19, 28, 28, 28,  0, 47,  0,  0,  0,  0 },  //  ROOM 28
    { 26, 29, 29, 29,  0,  0,  0,  0,  0,  0 },  //  ROOM 29
    { 18,  0,  0, 25,  0,  0,  0,  0,  0,  0 },  //  ROOM 30
    { 20,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 31, END ROOM
    {  0,  0, 34,  0,  0, 47,  0,  0,  0,  0 },  //  ROOM 32
    { 34, 36,  0, 35,  0,  0,  0,  0,  0,  0 },  //  ROOM 33
    { 34, 33, 34, 32, 18,  0,  0,  0,  0,  0 },  //  ROOM 34
    { 33, 38, 36,  0,  0,  0,  0,  0,  0,  0 },  //  ROOM 35
    { 33, 39, 46, 35,  0,  0,  0,  0,  0,  0 },  //  ROOM 36
    {  0, 40,  0,  0, 16,  0,  0,  0,  0,  0 },  //  ROOM 37
    { 35,  0,  0,  0,  0, 41,  0,  0,  0,  0 },  //  ROOM 38
    { 36, 39, 40, 39,  0,  0,  0,  0,  0,  0 },  //  ROOM 39
    { 37,  0,  0, 39,  0,  0,  0,  0,  0,  0 },  //  ROOM 40
    {  0,  0, 42,  0, 38,  0,  0,  0,  0,  0 },  //  ROOM 41
    { 42, 43, 42, 41,  0, 47,  0,  0,  0,  0 },  //  ROOM 42
    {  0,  0, 42,  0, 19,  0,  0,  0,  0,  0 },  //  ROOM 43

        // Death rooms
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  DEATH BY DROWNING
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  DEATH BY BURNING
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  DEATH BY FREEZING
    {  0,  0,  0,  0,  0,  0,  0,  0 },  //  BOTTOMLESS PIT

};

constexpr int DEBUG_RAND_SEED = 67;
struct GlobalState GLOBALS = {
    .player_name = nullptr,
    .silent_mode = false,
    .char_sleep_duration = _10ms,
    .char_sleep_visited_duration = _1ms,
    .debug_normal_sleep = 0,
    .debug_visited_sleep = 0,
    .debug_mode = false,
};



// --------------------------------------------------------------
//      Forward references
// --------------------------------------------------------------
static void update_perception(GameState * gs);
static int calc_score(const GameState * gs) ;
bool perform_action(GameState *gs, char action, int arg1, int arg2, int arg3);


//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------



static void display_status(const GameState * gs) {
    if (GLOBALS.silent_mode ) return;
    displayf("magic spells: %d, $%d\n", gs->magic, gs->cash);
}

static void display_inventory(const GameState * gs) {
    display_status(gs);
    actor_display_inventory(gs, false, false);
}




static void display_help_info(void) {
    if (GLOBALS.silent_mode) return;

    display_line("\nVALID COMMANDS ARE:\n");

    display_line("[H]elp       [I]nventory  [Q]uit");
    display_line("[A]ttributes S[c]ore      [L]ook");
    display_line("[R]etreat    [F]ight");
    display_line("[T]ake       Dro[p]");
    display_line("[N]orth      [S]outh");
    display_line("[E]ast       [W]est");
    display_line("[U]p         [D]own");

    display_line("\nDEBUG:");
    display_line("[1]Globals  [2]GameState [3]Reset  [M]agic");
}



//// ------------------------------------------------------------
////
////    GAME FUNCTIONS
////
//// ------------------------------------------------------------



static int calc_score(const GameState * gs) {
    int cash = gs->cash;
    int sum_attributes = gs->stats.strength + gs->stats.charisma + gs->stats.dexterity +
        gs->stats.intelligence + gs->stats.wisdom + gs->stats.constitution;
    int monsters_killed = gs->monsters_killed;
    double monster_win_ratio = 0;
    if (gs->monsters_fought > 0) {
        monster_win_ratio = (double)monsters_killed / gs->monsters_fought;
    }

    int num_monsters = monsters_num_monsters();
    double monster_fought_ratio =
        (double)gs->monsters_fought / (num_monsters - 1.0);  // can't kill dwarf

    int turns = gs->turns;
    // we need to tune this. I am looking forward to using ML to determine this value!
    // This is the theoretical minimum number of turns required to maximize your score.
    // 100 is just a starting heuristic
    int ideal_turns = 107;
    double turn_ratio = 0;
    if (turns > 0 && gs->completed) {
        turn_ratio = (double)ideal_turns / turns;
    }

    int rooms_visited = room_count_visited();
    int nice_rooms    = room_num_nice_rooms();
    double room_ratio = (double)rooms_visited/nice_rooms;

    // todo tune this
    // cash : 0- 1730 in this game
    // sum_attributes - 63 is average at start when in good health
    // monsters_killed  - 17 total possible
    // monster_win_ratio - max 1
    // monster_fought_ratio - max 1
    // turns - 1 to ??? we'll tune this.
    // turn ratio - can be < or > 1 if user performs better than ideal
    // rooms_visited - max 43 in this game. 48 total rooms, -1 for NULL room, -4 death rooms = 43.
    // room_ratio - max 1

    // user can't fully control attribute values at end of game, so weight this least with 1
    // cash is fully controllable, but looks like it dominates scoring in this game.
    // it's unlikely the player can visit every room and kill every monster
    // there's an optimal number of turns. The user should be rewarded for achieving a low score
    // in this game rooms visited are slightly linked to monsters killed, as there are monsters in almost
    // half the rooms.
    // We should reward visiting all rooms

    int weighted_score = 0;

    weighted_score += cash;
    weighted_score += sum_attributes;
    if (monsters_killed >= 17) weighted_score += 100; // bonus
    weighted_score += (int)(200 * monster_fought_ratio);
    weighted_score +=  3 * monsters_killed;
    if (monster_win_ratio >= .9999) {
        weighted_score += 100;
    }

    if (rooms_visited >= 43) weighted_score += 100; // bonus
    weighted_score += (int)(250 * room_ratio);

    if (turns <= ideal_turns ) weighted_score += 100; // bonus
    weighted_score +=  (int)(250 * turn_ratio);

    if (gs->completed && !gs->is_dead) {
        weighted_score += 200;
    }

    return weighted_score;
}

static void display_score(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    display_linef("\nSCORE: %d\n", calc_score(gs) );
    const int rooms_visited = room_count_visited();
    display_linef("turns: %d, cash: $%d, monsters fought: %d, killed: %d, rooms: %d",
        gs->turns, gs->cash, gs->monsters_fought, gs->monsters_killed, rooms_visited);
    display_linef("You completed %3.0f%% of the quest.\n",
           (double) rooms_visited * 100.0 / (room_num_rooms() - NUM_DEATH_ROOMS - 1));

}






// first_letter must be in "NSEWUD"
// return true if command was successfully processed. If false, the move is not allowed and an error message
// will have been displayed
static bool cmd_move(GameState * gs, char const cmd_char) {
    const int location = gs->room;
    const int direction_index = calc_room_graph_direction_index(cmd_char);
    if (direction_index == DIRECTION_ERR) {
        display("Bad direction_index, first_letter='");
        printf("%c'\n", cmd_char);
        return false;
    }
    gs->room_prev = gs->room;

    if (ROOM_GRAPH[location][direction_index] > 0) {
        gs->room = ROOM_GRAPH[location][direction_index];
        return true;
    }

    display_line(BAD_MOVE_DESC[direction_index]);
    return false;
}

static bool can_take_item(const GameState *gs, const object_id id, const bool verbose) {
    if ( !gs->has_torch && id != ITEM_TORCH ) {
        if (verbose) display_line("It is too dark to see anything.");
        return false;
    }
    if ( id < 1 ) {
        display_line("There is nothing to pick up.");
        return false;
    }
    // in this game, objects with ids > ITEM_WAND are converted to cash and not 'takeable' by the player.
    if (id <= ITEM_WAND && actor_count_of_objects(gs) >= MAX_PLAYER_OBJECTS ) {
        if (verbose) {
            display_linef("You are already carrying your maximum of %d objects.",
                    MAX_PLAYER_OBJECTS);
        }
        return false;
    }
    return true;
}

static bool action_take(GameState * gs, const object_id id) {
    if (! can_take_item(gs, id, false )) return false;
    const Room *room = room_find_room(gs->room);
    if (id > ITEM_WAND) {
        // in this game, objects with ids > ITEM_WAND are converted to cash and not
        // 'takeable' by the player.
        const Object *o = obj_find_object(id);
        gs->cash += o->value;
    } else {
        if (!actor_add_object(gs, id)) {
            printf("action_take: actor_add_object failed for obj id=%d", id);
            return false;
        }
    }
    if ( id == ITEM_TORCH ) {
        gs->has_torch = true;
    }
    room_transfer_obj_location(room, id, PLAYER_LOCATION );
    return true;
}

static bool cmd_take(GameState * gs) {
    const Room *r = room_find_room(gs->room);
    object_id id = room_first_object_id(r);
    if ( !can_take_item(gs, id, true )) return false;
    if ( !action_take(gs, id)) return false;
    display_line("Taken.");
    return true;
}


/**
 * Shared Validation: Can an item be dropped here?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_drop_item(const GameState *gs, const object_id id, const bool verbose) {
    if (!id || !actor_has_item(gs, id)) {
        if (verbose) display_linef("You are not carrying that item. object_id:%d", id);
        return false;
    }
    const Room *r = room_find_room(gs->room);
    if ( room_is_full(r)) {
        if (verbose) {
            display_line("The room is full.");
        }
        return false;
    }

    if (room_contains_object(r, id)) {
        if (verbose) {
            display_linef( "There is already a %s here.",  obj_name_for_id(id));
        }
        return false;
    }

    return true;
}

// Entry point for human user path. This displays some information, prompts user for some choices, and passes those to
// drop_action(), the ML entry point for the drop action.
static bool cmd_drop(GameState * gs) {
    int item_index = 0;

    actor_display_inventory(gs, true, false);
    item_index = get_int("Enter the number of the object to drop (0 for none): ", 0, gs->items_len + 1);
    if ( item_index == 0 )   return true;  // exit without dropping anything

    object_id id = gs->items[item_index - 1];

    if (!can_drop_item(gs, id, true)) return false;
    return perform_action(gs, 'P', id, 0, 0);
}



/** Logic Entry Point: ML and Human both end up here */
static bool action_drop(GameState *gs, object_id id) {
    if (!can_drop_item(gs, id, false)) return false;
    if (!actor_remove_object(gs, id)) return false;

    if (id == ITEM_TORCH)  gs->has_torch = false;

    const Room *r = room_find_room(gs->room);
    return room_add_object( r, id ) == ROOM_SUCCESS;
}

//todo (rob) make `strategy` an enum
//return false if fight action could not be completed, otherwise return true
bool action_fight(GameState * gs, int strategy, enum StatIndex stat1, enum StatIndex stat2) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        return false;  // nothing to fight
    }    
    const Room *r = room_find_room(gs->room);
    MonsterPrototype *m = monsters_find_monster(r->monster);
    
    if (strategy == 1 && gs->magic == 0 ) {
        // not enough magic
        //todo (rob) - create return code for this case and others.
        return false;
    } 
    
    gs->monsters_fought++;
    gs->must_fight = false;

    // we'll pause a bit after every turn during the fight
    uint32_t pause_seconds;
    if (GLOBALS.debug_mode ) {
        pause_seconds = 0;
    } else {
        pause_seconds = _1ms * 1000;
    }
    
    if (strategy == 1) {
        display_line("Your magic destroys it!");
        char_sleep((int32_t)pause_seconds);
        gs->magic--;
        gs->monsters_killed++;
        room_clear_monster(r);
        return true;
    }
    
    int hero_tally = 0;
    int monster_tally = 0;
    
    // calc enhancements due to fighting items
    for (enum Item item = ITEM_SWORD; item <= ITEM_WAND; ++item ) {
        if (gs->items[item]) {
            hero_tally++;
        }
    }
    
    hero_tally += gs->stats.as_array[stat1];
    hero_tally += gs->stats.as_array[stat2];
    monster_tally += m->stats.as_array[stat1];
    monster_tally += m->stats.as_array[stat2];

    if (!gs->has_torch) {
        hero_tally -= 5;  // harder to see in the dark
    }
    
    if (!GLOBALS.silent_mode) {
        display("\nThe fight starts in favor of ");
        if (hero_tally > monster_tally ) {
            display_line("you.");
        } else {
            display_line(m->name);
        }
        char_sleep((int32_t)pause_seconds);
        display_linef("The %s - %d", m->name, monster_tally);
        char_sleep((int32_t)pause_seconds);
        display_linef("%s - %d", gs->player_name->buffer, hero_tally);
        char_sleep((int32_t)pause_seconds);

    }
    
    for (;;) {
        int attack = rnd_range(gs, 0,8 );
        switch (attack) {
            case 0: {
                display_line("You get in a glancing blow");
                monster_tally--;
            } break;
            case 1: {
                display("The ");
                display(m->name);
                display_line(" strikes out!");
                hero_tally -= 3;
                gs->stats.strength--;
                gs->stats.charisma--;
            } break;
            case 2: {
                display("You draw the ");
                display(m->name);
                display_line("'s blood!");
                monster_tally--;
            } break;
            case 3: {
                display_line("You are wounded!!");
                hero_tally -= rnd_range(gs, 1, 4);
                gs->dexterity--;  // annonymous union lets us do this!
            } break;
            case 4: {
                display("The ");
                display(m->name);
                display_line(" is tiring.");
                monster_tally--;
            } break;
            case 5: {
                display_line("You are bleeding....");
                hero_tally -= 2;
                gs->stats.wisdom--;
                gs->stats.constitution--;
            } break;
            case 6: {
                display("You wound the ");
                display(m->name);
                display_line("");
                monster_tally--;
            } break;
            case 7:
            default: {
                const int lost_cash = rnd_range(gs, 1, gs->cash/2 + 1);
                if (!GLOBALS.silent_mode) {
                    display("It knocks $");
                    printf("%d from your hand.\n",lost_cash);
                }
                gs->cash -= lost_cash;
            } break;
        }

        char_sleep((int32_t)pause_seconds);

        if (! (hero_tally > 0 && monster_tally > 0 && rnd_d(gs) < .75 ) ) {
            break;
        }
    }

    if (hero_tally > monster_tally ) {
        display_line("You bested the beast!");
        gs->monsters_killed++;
    } else {
        display_linef("The %s got the better of you that time.", m->name);

        if (stat1 == STAT_STRENGTH || stat2 == STAT_STRENGTH ) {
            gs->stats.strength = 4 *  gs->stats.strength / 5;
        }
        if (stat1 == STAT_CHARISMA || stat2 == STAT_CHARISMA ) {
            gs->stats.charisma = 3 *  gs->stats.charisma / 4;
        }
        if (stat1 == STAT_DEXTERITY || stat2 == STAT_DEXTERITY ) {
            gs->stats.dexterity = 6 *  gs->stats.dexterity / 7;
        }
        if (stat1 == STAT_INTELLIGENCE || stat2 == STAT_INTELLIGENCE ) {
            gs->stats.intelligence = 2 *  gs->stats.intelligence / 3;
        }
        if (stat1 == STAT_WISDOM || stat2 == STAT_WISDOM ) {
            gs->stats.wisdom = 5 *  gs->stats.wisdom / 6;
        }
        if (stat1 == STAT_CONSTITUTION || stat2 == STAT_CONSTITUTION ) {
            gs->stats.constitution = 3 *  gs->stats.constitution / 6;
        }
    }
    char_sleep((int32_t)pause_seconds);
    room_clear_monster(r);
    //normalize any negative stats to 0
    for (int i = 0; i < STAT_COUNT; ++i ) {
        if (gs->stats.as_array[i] < 0 ) {
            gs->stats.as_array[i] = 0;
        }
    }
    return true;
}

// Entry point for human user path. This displays some information, prompts user for some choices, and passes those to
// perform_action(), the ML entry point for the fight action.
static bool cmd_fight(GameState * gs) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        display_line("There is nothing to fight.");
        return false;
    }
    const Room *r = room_find_room(gs->room);
    MonsterPrototype *m = monsters_find_monster(r->monster);

    if (gs->has_torch) {
        display("\nYour opponent is a ");
        display_line(m->name);
        display_line("With the following attributes:");
        display_char_attributes(m->stats);
    }
        display_line("\nYour attributes are:");
        display_char_attributes(gs->stats);


    if (gs->items[ITEM_SWORD]) {
        display_line("You have a sword");
    }
    if (gs->items[ITEM_WAR_HAMMER]) {
        display_line("Your War Hammer will be of aid");
    }
    if (gs->items[ITEM_CHAIN_MAIL]) {
        display_line("Chainmail armor gives you an edge");
    }
    if (gs->items[ITEM_SHIELD]) {
        display("Your shield will help you in this fight against the ");
        display_line(m->name);
    }
    if (gs->items[ITEM_CLOAK]) {
        display_line("The Cloak of Protection surrounds you");
    }
    if (gs->items[ITEM_WAND]) {
        display_line("The Wand of Fireballs enhances your strength");
    }
    display_line("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
    if (gs->magic) {
        int choice = get_int("Enter 1 to fight with magic or 2 to rely on skill: ", 1, 2);
        if (choice == 1) {
            return perform_action(gs, 'F', 1, 0, 0);
        }
    }

    display_line("Which attributes to fight with (choose 2):");
    display_line("1: STR, 2: CHA, 3: DEX, 4: INT, 5: WIS, 6: CON");

    const int first_skill = get_int("Enter first  skill (1-6) ", 1, 6);
    int second_skill;
    for (;;) {
        second_skill = get_int("Enter second skill (1-6) ", 1, 6);
        if (first_skill != second_skill) {
            break;
        }
        display("Duplicate skill: ");
    }
    return perform_action(gs, 'F', 2, first_skill, second_skill);
}

static bool process_retreat(GameState * gs) {
    const int room = gs->room;
    if (!ROOM_GRAPH[room][RGINDEX_MONSTER]) {
        display_line("There is nothing to retreat from");
        return false;
    }

    display_line("RUN AWAY!!!!!!!");
    // determine possible exits
    int num_exits = 0;
    int exits[RGINDEX_DOWN + 1] = {};
    for (int exit_index = RGINDEX_NORTH; exit_index <= RGINDEX_DOWN; ++exit_index ) {
        const int room_index = ROOM_GRAPH[room][exit_index];
        if ( room_index ) {
            //todo retreat through unlocked doors should be ok, but not through locked doors
            if ( !( room_index == ROOM_END || room_index ==  WINE_CELLAR_EAST || room_index == MARBLE_HALL) ) {
                // don't retreat to end room or through locked doors
                exits[num_exits++] = room_index;
            }
        }
    }

    // randomly move to an adjacent room. If current room has paths to itself, new room may not change
    int retreat_index = rnd_range(gs, 0, num_exits);

    if ( rnd_d(gs) < .6 || num_exits == 0 || retreat_index == room) {
        display_line("The creature blocks your path. You must fight.");
        gs->must_fight = true;
        return false;
    }

    gs->room = exits[retreat_index];
    return true;
}

/**
 * Helper to check if a specific command character is currently legal 
 * based on the game rules and current room state.
 */
static bool is_action_legal(const GameState *gs, char c) {
    const char cmd = (char)toupper(c);
    const int room_index = gs->room;
    const int monster_index = ROOM_GRAPH[room_index][RGINDEX_MONSTER];
    const int treasure_index = ROOM_GRAPH[room_index][RGINDEX_TREASURE];

    // 1. Basic monster check
    if (monster_index > 0) {
        if (gs->must_fight && cmd != 'F') return false;
        if (cmd != 'F' && cmd != 'R') return false;
    }
    // 2. Directional check
    int dir_idx = calc_room_graph_direction_index(cmd);
    if (dir_idx != DIRECTION_ERR) {
        return ROOM_GRAPH[room_index][dir_idx] > 0;
    }
    // 3. Item check for 'P' (Pick up)
    if (cmd == 'P' && treasure_index == 0) return false;

    return true;
}

static void update_perception(GameState * gs) {
    const int room_index     = gs->room;
    const int monster_index  = ROOM_GRAPH[room_index][RGINDEX_MONSTER];
    const int treasure_index = ROOM_GRAPH[room_index][RGINDEX_TREASURE];

    // reset perceptions
    gs->perception.monster_is_visible  = false;
    gs->perception.treasure_is_visible = false;
    gs->perception.current_monster  = (MonsterPrototype){};
    gs->perception.current_treasure = (Object){};
    gs->perception.legal_actions_mask = 0;

    // Populate Action Mask for ML
    for (int i = 0; VALID_COMMANDS[i] != '\0'; ++i) {
        if (is_action_legal(gs, VALID_COMMANDS[i])) {
            gs->perception.legal_actions_mask |= (1u << i);
        }
    }

    const Room *r = room_find_room(gs->room);

    // Only see things if the room is lit
    // Note: Object index 1 is the Torch itself, which is visible in the dark.
    if (gs->has_torch || treasure_index == ITEM_TORCH ) {
        if (monster_index > 0 ) {
            MonsterPrototype *m = monsters_find_monster(monster_index);
            if (! m) {
                printf("update_perception: m is null: monster_index=%d\n", monster_index);
                display_game_state(gs);
            }
            gs->perception.current_monster = *m;
            gs->perception.monster_is_visible  = true;
        }
        if (treasure_index > 0 ) {
            gs->perception.current_treasure = *obj_find_object(treasure_index);
            gs->perception.treasure_is_visible  = true;
        }
    }

}

// checks if there is a monster and if so, that the user has selected either F or R. Returns true for success.
bool monster_check(const GameState * gs, const char cmd) {
    const bool has_monster = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    // Rule: Must deal with monsters first. Recognized command, but logic fails.
    if ( has_monster && gs->must_fight && cmd != 'F') {
        display_line("DANGER! You can only FIGHT!");
        return false;
    }
    if (has_monster && cmd != 'F' && cmd != 'R') {
        display_line("DANGER! You must either FIGHT or RETREAT.");
        return false;
    }

    return true;
}



/**
 * Death and Win condition check
 * RETURNS: true if the game is over (win or loss).
 * The caller should check gs->is_dead or gs->completed to see the outcome.
 */
bool check_game_over(GameState *gs) {
    if (gs->completed) return true;

    if (gs->room == ROOM_END || gs->room >= DROWNING_ROOM) {
        if (gs->room >= DROWNING_ROOM) gs->is_dead = true;
        gs->completed = true;
        return true;
    }

    for (int i = STAT_STRENGTH; i < STAT_COUNT; ++i) {
        if (gs->stats.as_array[i] <= 0) {
            if (!GLOBALS.silent_mode) {
                display_char_attributes(gs->stats);
                display_line("\nYour combined attributes are no longer\nenough to sustain you... You are dead.");
            }
            gs->is_dead = true;
            gs->game_over = true;
            return true;
        }
    }
    return false;
}




//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------


// -----------------------------------------------------------------
//      called at the start of each new game
// -----------------------------------------------------------------

void reset(GameState * gs, const uint32_t seed) {
    // reset GameState
    *gs = (GameState){ .seed = seed, .player_name = GLOBALS.player_name, .room = ROOM_START, .cash = 100, .magic = 3 };

    mt_initialize_state(&gs->mt_state, seed);  // initialize the PRNG

    gs->stats = random_hero_stats(gs);

    //clear all monsters, treasure
    const int num_rooms = room_num_rooms();
    for ( int room_index = 0; room_index < num_rooms; ++room_index ) {
        // note: if we dynamically modify the edge graph we'll need to reset those edges here
        ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
        ROOM_GRAPH[room_index][RGINDEX_MONSTER] = 0;
        ROOM_GRAPH[room_index][RGINDEX_REQUIRED_KEY] = 0;
        ROOM_GRAPH[room_index][RGINDEX_UNUSED] = 0;
        const Room *r = room_find_room(room_index);
        room_clear_monster( r );
        room_remove_all_objects(room_index);
    }
    monsters_clear_all();
    // special treasure items
    room_add_object(room_find_room(ROOM_START),      ITEM_TORCH);
    room_add_object(room_find_room(LIBRARY_ROOM),    ITEM_SILVER_KEY);
    room_add_object(room_find_room(GLOVE_STOREROOM), ITEM_GOLD_KEY);

    // allot random treasure
    const int num_objects = obj_num_objects();
    for (int treasure_index = 4; treasure_index < num_objects; ++treasure_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, 43 + 1);  // rooms after 43 are death rooms
            const Room *r = room_find_room(rand_room);
            if ( ! ( r->objects_len > 0 || rand_room == ROOM_START || rand_room == ROOM_END  ) ) {
                room_add_object(room_find_room( rand_room ), treasure_index);
                break;
            }
        }
    }

    const int num_monsters = monsters_num_monsters();
    // allot monsters
    for (int monster_index= 1; monster_index < num_monsters; ++monster_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, 43 + 1);
            if ( ! ( ROOM_GRAPH[rand_room][RGINDEX_MONSTER] ||
                    rand_room == ROOM_START ||
                    rand_room == ROOM_END ||
                    rand_room == LIBRARY_ROOM ||
                    rand_room == GLOVE_STOREROOM)) {
                ROOM_GRAPH[rand_room][RGINDEX_MONSTER]  = monster_index;
                CharStats stats = random_monster_stats(gs);
                int ff = sum_character_stats(&stats);
                room_set_monster(room_find_room(rand_room), monster_index);
                monsters_update_monster(
                    &(MonsterPrototype){
                        .name = monsters_name_for_id(monster_index),
                        .id = monster_index,
                        .ferocity_factor = ff,
                        .stats = random_monster_stats(gs)
                    });
                break;
            }
        }
    }


    update_perception(gs);
}

static void init_string_assets() {
    // this will eventually be loaded from a text file
    global_string_assets.conclusion_completed = "You have succeeded!\nYou have escaped the Citadel of Pershu.\nWell done!";
    global_string_assets.conclusion_died      = "You have died.........";
    global_string_assets.conclusion_quit      = "COWARD...QUITTER....TURNCOAT.....";
}

// one time inits of ROOM or ROOM_GRAPH data
static void  init_rooms() {
    RandomTextArray *rta;
    // randomized text in Rooms 1, 18, 37, 39
    // room 1
    rta = create_rta(2);
    // ROOMS[1].epilog = create_rta(2);
    rta->lines[0] = (RandomText){ .chance_percent = .5, .text="There is an exit to the west."};
    rta->lines[1] = (RandomText){ .chance_percent = .5, .text="A tunnel leads to the south."};
    room_set_epilog(1, rta);

    // room 18
    rta = create_rta(1);
    rta->lines[0] = (RandomText){ .chance_percent = .5, .text="A bat flies past you, shrieking."};
    room_set_epilog(18, rta);
    // room 37
    rta = create_rta(2);
    rta->lines[0] = (RandomText){ .chance_percent = .7, .text="But now it tells you there is\na hidden stairwell in the room."};
    rta->lines[1] = (RandomText){ .chance_percent = .3, .text="The voice faintly murmurs of the door to the south."};
    room_set_epilog(37, rta);
    // room 39
    rta = create_rta(1);
    rta->lines[0] = (RandomText){ .chance_percent = .6, .text="A small door leads to the north\nand another to the east."};
    room_set_epilog(39, rta);
}


static constexpr size_t num_roomz = 48;  // todo (temp) until room data is read from file
typedef struct RoomData {
    size_t size;
    Room data[num_roomz];
} RoomData;

static RoomData get_room_data(void) {
    return (RoomData){
        .size = num_roomz,
        .data = {
            {.id =  0,  .name= "NULL ROOM",   .desc = "NULL ROOM"},
            {.id =  1,  .name= "River",       .desc = "An underground river flows swiftly by."},
            {.id =  2,  .name= "Food Store",  .desc = "You are in the Citadel's food storage area.\nOld cheeses and black loaves of bread can be seen, as well as many sacks of supplies."},
            {.id =  3,  .name= "Kitchen",     .desc = "You are in the Citadel's kitchen. A huge joint of meat turns slowly over a raging fire. Doors lead into cupboards, as well as to the west and to the south."},
            {.id =  4,  .name= "Library",     .desc = "This is the Central Library. Leather-bound volumes line the walls, right up to the ornately carved ceiling."},
            {.id =  5,  .name= "Studio",      .desc = "This room is an awful mess. It used to be an artist's studio. Paint and old easels lie around the floor."},
            {.id =  6,  .name= "Entrance",    .desc = "This is the entrance to the Citadel of Pershu.\nTurn now, if you wish. Many stronger than you have taken fright at its menacing towers and dark portals. If you wish to proceed, move east towards the black gaping doorway."},
            {.id =  7,  .name= "Altar",       .desc = "A stone altar stands in the middle of the room with two dead candles on it. An old book lies on one part of the altar top, and a faded red parchment cloth covers the front of it."},
            {.id =  8,  .name= "Black Tower", .desc = "You stand high on the black tower, the Citadel stretches to the north, south and east of you.\nThere is only one way out."},
            {.id =  9,  .name= "North Cellar",.desc = "You are in the northern section of the Citadel's large wine cellar. Heavy barrels lie all around you in this end of the cellar. There is a door to the north and one to the south."},
            {.id = 10,  .name= "West Cellar", .desc = "You are in the west wing of the wine cellar. There is a door to the west and one to the east. The central circular part of the cellar lies beyond the east door."},
            {.id = 11,  .name= "Wine Cellar", .desc = "You are in the central circular area of the wine cellar. There is a door at each compass point."},
            {.id = 12,  .name= "East Cellar", .desc = "You are in the east section of the wine cellar. There is a door to the west and one - which you cannot use,as it only allows entrance to where you now stand - to the east."},
            {.id = 13,  .name= "South Cellar",.desc = "There are many, many wine bottles here lying on their sides in this southern section of the wine cellar. There is a dark, unfriendly-looking hole to the west and doors to the north and to the south."},
            {.id = 14,  .name= "Armory",      .desc = "This is the Citadel's armory. Row upon row of shiny suits of armor are stored here." },
            {.id = 15,  .name= "Bedchamber",    .desc = "You are in the ruler's bedchamber.\nA large fire burns in the south of the room, with a small door beside it. Other exits are to the north and to the west." },
            {.id = 16,  .name= "Sand",          .desc = "Sand covers the floor of this curious room, heaped into drifts.\nBy peeping over the 'dunes' you can see a golden passage way leads to the west, and there is a door to the south. You are not sure whether or not you have seen all the exits." },
            {.id = 17,  .name= "Gallery",       .desc = "You are in the picture gallery. Portraits of long-dead princes line all of the walls. The room is dominated by a huge landscape, hanging above the exit to the east which leads, via the gold passage way back to that curious room of sand." },
            {.id = 18,  .name= "Tower Balcony", .desc = "You are on a remote tower balcony.\nThere are stairs here." },
            {.id = 19,  .name= "Archway",       .desc = "You walk beneath a stone archway.\nYou can only walk north or south unless you decide to take the stairs." },
            {.id = 20,  .name= "Marble Hall",       .desc = "This vast hall has a marble floor, and the slightest sound echos violently.\nThere are purple drapes concealing the exits from this hall." },
            {.id = 21,  .name= "Glove Storeroom",   .desc = "You are in the glove storeroom.\nThe west door radiates heat.\nAnother door leads to the south." },
            {.id = 22,  .name= "Silver Storeroom",  .desc = "You are in the silver crosses storeroom.\nThere are only two exits." },
            {.id = 23,  .name= "Amulet Storeroom",  .desc = "You are in the amulet storeroom.\nDoors lead north and south." },
            {.id = 24,  .name= "Kazoom Storeroom",  .desc = "You are in the kazoo storeroom.\nThere are two exits." },
            {.id = 25,  .name= "Satchel Storeroom", .desc = "You are in the satchel storeroom." },
            {.id = 26,  .name= "Wooden Storeroom",  .desc = "You are in the storeroom for wooden boxes... There are two exits." },
            {.id = 27,  .name= "Vase Storage",      .desc = "This is where printed vases are stored... As you can easily see." },
            {.id = 28,  .name= "Mine",              .desc = "The heavy air of this area seems to make your torch very dim-> You can hardly see that air is rushing up from somewhere.\nYou can just make out that this area must be a mine of some sort." },
            {.id = 29,  .name= "Tunnels", .desc = "You appear to be in an endless labyrinth,lined with paintings.........\nWhichever way you turn, there seems to be more tunnels, all lined with paintings." },
            {.id = 30,  .name= "South Tower", .desc = "This is the southern tower of the Citadel." },
            {.id = 31,  .name= "Exit", .desc = "Well done, you have managed to find the exit.\nTake a deep breath of good, clean air..........." },
            {.id = 32,  .name= "Meditation Room", .desc = "This room is filled with swirling smoke,so you cannot see... Air rushes past a statue of the goddess Diana. This must be the Citadel's meditation chamber." },
            {.id = 33,  .name= "Bridge", .desc = "A small forked bridge crosses a stream here.\nYou can move north, south, or west." },
            {.id = 34,  .name= "Cavern", .desc = "You are in a rough stone cavern. Stairs lead up from here.\nThere is also a single door which leads away from the cavern." },
            {.id = 35,  .name= "Stable", .desc = "This is the former Citadel underground stable. It smells terrible." },
            {.id = 36,  .name= "Courtyard", .desc = "You find yourself in an underground courtyard. Strange, twisted trees are around you, and a wind of incredible coldness blows from the east." },
            {.id = 37,  .name= "Oracle", .desc = "This is the Oracle Room, although the mystic voice has not spoken for many years." },
            {.id = 38,  .name= "Sacrifice Room", .desc = "Horrors. A cold shudder passes through you as you realize this is the priests' sacrifice room.\nDried-up blood is on the floor and a skull grins at you from high on the wall." },
            {.id = 39,  .name= "Dungeon", .desc = "Old straw mattresses and rings chained to the wall tell you this was the Citadel's dungeon. The dungeon seems to stretch forever, with many small partitioned areas...." },
            {.id = 40,  .name= "Alcove",  .desc = "You are in a small alcove, with a solid gray granite throne in the middle of it." },
            {.id = 41,  .name= "Orc Guardroom", .desc = "This is the orc's guardroom, way below the ground. A stairwell ends here and a door leads to the east." },
            {.id = 42,  .name= "Healing Pool",  .desc = "There is a healing pool here, with a dangerous, swirling area of water." },
            {.id = 43,  .name= "Hall of Odric", .desc = "The Underpriests of Odric used this tiny hall for their forbidden worship eons ago. It is an unpleasant area,so you are thrilled to see a set of stone stairs." },

            {.id = 44,  .name= "DEATH BY DROWNING",  .desc = "Water covers your head.\nYou are drowning.\nGLUG... GASP............" },
            {.id = 45,  .name= "DEATH BY BURNING",  .desc = "The flames strike at you...\nas you slowly burn to death..."},
            {.id = 46,  .name= "DEATH BY FREEZING", .desc = "You are hit by a freezing spell and turn into a block of perpetual living stone. This is the end."},
            {.id = 47,  .name= "BOTTOMLESS PIT",    .desc = "You tumble down a bottomless pit.\nDown, down, down..."},

        }
    };
}

constexpr size_t num_objectz = 18;
typedef struct ObjectData {
    size_t size;
    Object data[num_objectz];
} ObjectData;

// first 9 elements are items the user can use, carry, or drop (and pick up again.)
// From Emeralds and higher, these are treasure that are converted to a cash equivalent
static ObjectData get_object_data(void) {
    return (ObjectData){
        .size = num_objectz,
        .data = {
            { .id =  1, .name = "Flaming Torch",         .is_light_source_bit = true, .is_lit_bit = true} ,
            { .id =  2, .name = "Silver Key",            },
            { .id =  3, .name = "Gold Key",              },
            { .id =  4, .name = "Sword",                 },
            { .id =  5, .name = "War Hammer",            },
            { .id =  6, .name = "Chain Mail Armor",      },
            { .id =  7, .name = "Shield",                },
            { .id =  8, .name = "Cloak of Protection",   },
            { .id =  9, .name = "Wand of Fireballs",     },
            { .id = 10, .name = "Emeralds",              .value =  99 },
            { .id = 11, .name = "Silver Rings",          .value = 247 },
            { .id = 12, .name = "Elven Amethysts",       .value = 166 },
            { .id = 13, .name = "Diamond Dragon Eyes",   .value = 462 },
            { .id = 14, .name = "Crystal Ball",          .value = 195 },
            { .id = 15, .name = "Pieces of Eight",       .value = 231 },
            { .id = 16, .name = "Elemental Gems",        .value = 162 },
            { .id = 17, .name = "Shape-Shifting Stones", .value =  27 },
            { .id = 18, .name = "Gold Doubloons",        .value = 141 },
        }
    };
}

// once time inits. Per-game inits happen in reset()
void initialize() {
    // note: random data is initialized in reset()
    RoomData rd = get_room_data();
    room_init(rd.size,rd.data);

    monsters_init("monsters.json");

    ObjectData od = get_object_data();
    obj_init(od.size, od.data);

    init_string_assets();

    init_rooms();
}


//// ------------------------------------------------------------
////
////    CLEANUP
////
//// ------------------------------------------------------------


static void cleanup(GameState * gs) {
    room_destroy();
    void *free_ptr = (void *) GLOBALS.player_name;
    GLOBALS.player_name = nullptr;
    gs->player_name = nullptr;
    free( free_ptr);
    monsters_destroy();
    obj_destroy();
}




//// ------------------------------------------------------------
////
////    MAIN
////
//// ------------------------------------------------------------


/**
  * Core Game Engine Logic
  * This function is "Pure Logic" - it updates state based on an action.
  * It returns true if the action was accepted as a turn, false otherwise.
  *
  * @param gs
  * @param action
  * @param arg1 For 'F': strategy (1:magic, 2:skill). For 'G': item index.
  * @param arg2 For 'F': first skill stat index.
  * @param arg3 For 'F': second skill stat index.
  *
  *
  */
bool perform_action(GameState *gs, char action, int arg1, int arg2, int arg3) {
    const char cmd = (char)toupper(action);

    if (!strchr(VALID_COMMANDS, cmd)) {
        return false; // Unknown command: Not a turn, no state change.
    }

    gs->turns++;

    if (gs->room == MARBLE_HALL ) {
        // if player is here, they already used the key to unlock the west door
        actor_remove_object(gs, ITEM_GOLD_KEY);
    }
    if (gs->room == WINE_CELLAR_EAST ) {
        actor_remove_object(gs, ITEM_SILVER_KEY);
    }
    if ( !monster_check(gs, cmd) ) {
        return false;
    }

    if (strchr(VALID_DIRECTIONS, cmd)) {
        // Special logic for locked doors

        if (gs->room == BEDCHAMBER_ROOM && cmd == 'W' && !actor_has_item( gs,ITEM_SILVER_KEY ) ) {
            display_line("You need the Silver Key to unlock the door.");
            return false;
        }
        if (gs->room == SILVER_CROSSES_STOREROOM && cmd == 'W' && !actor_has_item( gs,ITEM_GOLD_KEY )) {
            display_line("You need the Gold Key to unlock the door.");
            return false;
        }

        // We must update perception after the move is processed but before returning
        // so the new room's content is visible in the GameState.
        const bool result = cmd_move(gs, cmd);
        update_perception(gs);  // room may have changed
        return result;
    }

    bool result = false;
    switch (cmd) {
        case 'T':
            result = cmd_take(gs);
            break;
        case 'F':
            result = action_fight(gs, arg1, (enum StatIndex)arg2, (enum StatIndex)arg3);
            break;
        case 'R':
            result =  process_retreat(gs);
            break;
        case 'P':
            result =  action_drop(gs, arg1);
            break;
        default:
            // Unknown action
            result = false;
            break;
    }

    // Single point of truth. Update perception after any turn-based action.
    update_perception(gs);

    return result;
}


static bool main_game_loop(GameState * gs) {
    uint32_t saved_sleep_duration = GLOBALS.char_sleep_duration;
    const room_id room_id = gs->room;
    const Room *current_room = room_find_room(room_id);
    room_set_visit_started_flag(current_room);

    if (current_room->is_visited_bit) {
        // if we've already seen this room, speed up output display
        if ( GLOBALS.debug_mode ) {
            set_char_sleep( GLOBALS.debug_visited_sleep );
        } else {
            set_char_sleep( GLOBALS.char_sleep_visited_duration );
        }
    }

    if (gs->room != gs->room_last_turn) {
        // only display room desc once when first entering room. Reduces screen clutter and scrolling.
        // user can always type "look" to re-display room desc.
        display_line("");
        display_room_desc(gs);
        display_room_content(gs);  // we need to be able to query if any contents exist to add a newline before here
    }

    if (check_game_over(gs)){
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return END_GAME;
    }

    // speed up the display of text for the rest of the turn.
    if ( GLOBALS.debug_mode ) {
        set_char_sleep( GLOBALS.debug_visited_sleep );
    } else {
        set_char_sleep(GLOBALS.char_sleep_visited_duration);
    }

    // -----------------------------------------------------------------
    //      process user input
    // -----------------------------------------------------------------
    flush_input();
    char prompt_buffer[1024] = {};
    snprintf(prompt_buffer, sizeof(prompt_buffer), "\n%s >", current_room->name);
    char cmd = get_command_char(prompt_buffer, VALID_COMMANDS, nullptr);

    if (cmd == 'Q') {
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return cmd_quit(gs);
    }

    // -----------------------------------------------------------------
    //          DEBUG COMMANDS
    // -----------------------------------------------------------------

    if (cmd == '1') {
        display_globals();
    }
    if (cmd == '2') {
        display_game_state(gs);
    }
    if (cmd == '3') {
        reset(gs, DEBUG_RAND_SEED);
    }
    if (cmd == 'M') {
        gs->magic = 50;
    }

    // -----------------------------------------------------------------
    //      Player Presentation Only
    // -----------------------------------------------------------------


    if (cmd == 'H' ) {
        display_help_info();
    } else if (cmd == 'L') {
        cmd_look(gs);
    } else if (cmd == 'I' ) {
        display_inventory(gs);
    } else if (cmd == 'A' ) {
        display_char_attributes(gs->stats);
    } else if (cmd == 'C' ) {
        display_score(gs);
    } else if ( !monster_check(gs, cmd) ) {
        room_set_visited_flag(current_room);
        return CONTINUE_GAME;
    } else if ( cmd == 'F' ) {
        //specialized code to prompt user and gather options to pass to perform_action()
        cmd_fight(gs);
    } else if (cmd == 'P') {
        cmd_drop(gs);
    }
    else {
        // Now the human call and the ML call use the exact same entry point
        perform_action(gs, cmd, 0,0, 0);
    }

    set_char_sleep(saved_sleep_duration);

    if (room_id == gs->room) {
        // if room at end of turn is same as start of turn, update this so we don't display the room desc again
        gs->room_last_turn = room_id;
    } else {
        gs->room_last_turn = gs->room_prev;
    }

    room_set_visited_flag(current_room);
    return CONTINUE_GAME;
}





int main_citadel_of_pershu(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    set_silent_mode(GLOBALS.silent_mode);

    if (GLOBALS.debug_mode) {
        set_char_sleep(GLOBALS.debug_normal_sleep);
    } else {
        set_char_sleep(GLOBALS.char_sleep_duration);
    }

    const CharBuffer *player_name = get_player_name("Hello, Explorer ");
    GLOBALS.player_name = player_name;

    GameState gs = {};

    initialize();
    reset(&gs, DEBUG_RAND_SEED);

    display_line("Type '[H]elp' for a list of commands.");
    display_line("Your character attribute stats are:");
    display_char_attributes(gs.stats);
    display_line("");
    display_line("--------------------------------------------------------------------------------");
    display_line("");

    // obj_repr();
    // monsters_all_repr();
    // room_rooms_repr();

    bool continue_loop;
    do {
        continue_loop = main_game_loop(&gs);
    } while (continue_loop);

    display_conclusion(&gs);
    display_score(&gs);
    cleanup(&gs);
    display_line("");
    return EXIT_SUCCESS;
}

// main() is defined when running this TU stand-alone and including -DCITADEL_OF_PERSHU_MAIN compiler flag.
#ifdef CITADEL_OF_PERSHU_MAIN


int main(void) {
    return main_citadel_of_pershu();
}
#endif