// chateau_gaillard.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/02 21:58:37 PDT

// chateau_gaillard.c
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983
//
//
// Created by Rob Ross on 5/22/26.


// make :
// cd /Users/robross/Documents/Development/CLionProjects/text_adventures/src

/*
 * DEBUG:
clang -g -DCHATEAU_GAILLARD_MAIN -fsanitize=address -fsanitize=leak -Wall -Werror \
    -Wno-unused-const-variable -Wno-unused-variable -Wno-unused-function \
    -std=c23 -o chateau_gaillard.out chateau_gaillard.c ../adventure_shared.c \
      ../mersenne_twister.c ../common/console_utils.c ../common/string.c ../parser.c ../objects.c \
      ../rooms.c ../monsters.c ../roblib/string/string_utils.c

*/

#include "chateau_gaillard.h"

#include <limits.h>

int ROOM_GRAPH[][RGINDEX_COUNT] = {
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
    { 21,  0,  0,  0, 10, 39,  0,  0,  0,  0 },  //  ROOM 24
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




static void cleanup(GameState *gs);

static int calc_score(GameState *gs) {
    int cash = actor_calc_inventory_value(gs);
    gs->cash = cash;
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
    // cash : 0- 1135 in this game but assumes you keep amulet and healing potion which is unrealistic
    // sum_attributes - 63 is average at start when in good health
    // monsters_killed  - 19 total possible (can't kill dwarf)
    // monster_win_ratio - max 1
    // monster_fought_ratio - max 1
    // turns - 1 to ??? we'll tune this.
    // turn ratio - can be < or > 1 if user performs better than ideal
    // rooms_visited - max 37 in this game. 44 total rooms, -1 for NULL room, -6 death rooms = 37.
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
    if (monsters_killed >= 19) weighted_score += 100; // bonus
    weighted_score += (int)(200 * monster_fought_ratio);
    weighted_score +=  3 * monsters_killed;
    if (monster_win_ratio >= .9999) {
        weighted_score += 100;
    }

    if (rooms_visited >= 37) weighted_score += 100; // bonus
    weighted_score += (int)(250 * room_ratio);

    if (turns <= ideal_turns ) weighted_score += 100; // bonus
    weighted_score +=  (int)(250 * turn_ratio);

    if (gs->completed && !gs->is_dead) {
        weighted_score += 200;
    }

    return weighted_score;
}

//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------

static void display_score(GameState *gs) {
    if (GLOBALS.silent_mode) return;
    const int rooms_visited = room_count_visited();
    const int num_rooms = room_num_rooms();

    display_linef("\nSCORE: %d\n", calc_score(gs) );
    display_linef("turns: %d, cash: $%d, monsters fought: %d, killed: %d, rooms: %d",
                  gs->turns, gs->cash, gs->monsters_fought, gs->monsters_killed, rooms_visited);
    display_linef("You completed %3.0f%% of the quest.", (double) rooms_visited * 100.0 / (num_rooms - NUM_DEATH_ROOMS - 1));
}

// Print the name of the current room and current number of turns, along with
// dashes to separate each turn
static void display_room_header(const GameState *gs) {
    // Don't delete this. This is an example of how to dynamically build a string using
    // snprintf.
    const Room *r = room_find_room(gs->room);
    char room_buffer[81] = "--------------------------------------------------------------------------------";
    const size_t room_name_len = strlen(r->name);
    for (int i = 0; i < room_name_len; ++i) {
        room_buffer[i] = r->name[i];
    }
    room_buffer[room_name_len ] = ' ';
    size_t required_len = snprintf(nullptr, 0, " %d", gs->turns);
    // Start at 80 - required_len and provide space for the null terminator (required_len + 1)
    snprintf(&room_buffer[80 - required_len], required_len + 1, " %d", gs->turns);

    // printf("---------------------------------------------------------------------------- %d\n", gs->turns);
    display_line(room_buffer);
}


// Returns true if the user can move from the current room via the direction
bool check_can_move(GameState *gs, int const direction, const bool verbose) {
    const int room_id = gs->room;
    // check for transition guards, like locks on doors, puzzles solved, equipment carried, etc.
    // i.e., if room_index == ROOM_YELLOW && (dwarf_alive) print("Dwarf stops you"); return false;
    const Room * r = room_find_room(room_id);
    if (room_id == ROOM_YELLOW && r->monster == MONSTER_DWARF ) {
        if (verbose){ display_line("The dwarf refuses to let you proceed."); }
        return false;
    }

    if (!strchr(VALID_DIRECTIONS, direction)) {
        if (verbose) display_linef("I don't know how to go %c.", direction);
        return false;
    }

    const int direction_index = calc_room_graph_direction_index((char) direction);
    if (direction_index == DIRECTION_ERR) {
        if (verbose) display_linef("Bad direction_index, first_letter='%c'", direction);
        return false;
    }

    const int next_room_id = ROOM_GRAPH[room_id][direction_index];
    if (next_room_id == 0) {
        display_line(BAD_MOVE_DESC[direction_index]);
        return false;
    }

    const int num_rooms = room_num_rooms();

    if (next_room_id < 0 || next_room_id >= num_rooms) {
        if (verbose) {
            display_linef("runtime error: next_room_index is out of bounds: %d, expected range [0, %d]",
                          next_room_id, num_rooms - 1);
        }
        return false;
    }

    if ( ( next_room_id == ROOM_KITCHEN || next_room_id == ROOM_UNEVEN ) &&
           ROOM_GRAPH[next_room_id][RGINDEX_REQUIRED_KEY] > 0 ) {
        display_line("The door is locked.");
        return false;
    }

    return true;
}

// ML/engine path
static bool action_move(GameState *gs, int const first_letter) {
    if (!check_can_move(gs, first_letter, false)) {
        return false;
    }
    const int direction_index = calc_room_graph_direction_index(first_letter);
    gs->room_prev = gs->room;
    gs->room = ROOM_GRAPH[gs->room][direction_index];
    return true;
}

// Human user path
// Expect the first letter of the object[] to be in "NSEWUD".
// Return true if the command was successfully processed. If false, the move is not allowed and an error message
// will have been displayed.
static bool cmd_move(GameState *gs, const ParsedCommand *pc) {
    const enum Command ocmd = pc->verb_object_command;
    if ( (ocmd < CMD_NORTH || ocmd > CMD_DOWN)) {
        if (!pc->has_verb_object) {
            display_linef("%s where?", pc->verb);
        } else {
            display_linef("I don't know how to %s '%s'.", pc->verb, pc->verb_object);
        }
        return false;
    }

    const int first_letter = toupper(pc->verb_object[0]);
    if (!check_can_move(gs, first_letter, true)) {
        return false;
    }
    return perform_action(gs, CMD_MOVE, first_letter, 0, 0);
}

//return false if fight action could not be completed, otherwise return true
bool action_fight(GameState *gs, const object_id weapon, const enum StatIndex stat1, const enum StatIndex stat2) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        return false; // nothing to fight
    }
    // make sure we are actually carrying the weapon
    bool missing_weapon = true;
    const int items_len = gs->items_len;

    if (weapon == BARE_HANDS ) {
        missing_weapon = false;
    } else {
        for (int j = 0; j < items_len; ++j) {
            if (gs->items[j] == weapon) {
                missing_weapon = false;
                break;
            }
        }
    }
    if (missing_weapon) {
        const Object *o = obj_find_object(weapon);
        if (!o) {
            display_linef("unknown weapon id=%d", weapon);
            return false;
        }
        displayf("You're not carrying a %s.", o->name);
        return false;
    }
    // We add to the hero_tally (8 - weapon)
    // that will give us 7 extra points for using the axe, down to 1 extra point for using a falchion.
    int weapon_strength = 0;
    if (weapon > BARE_HANDS && weapon <= OBJECT_FALCHION) {
        weapon_strength = 8 - weapon;
    }
    //here we use the ordinality of the object_ids, but eventually weapons will
    // have their own attributes we will query
    int weapon_bonus = weapon_strength;


    // printf("weapon:%d, weapon_strength:%d, weapon_bonus:%d\n",weapon, weapon_strength, weapon_bonus);

    gs->monsters_fought++;
    // gs->must_fight = false;
    const Room *room = room_find_room(gs->room);
    const monster_id mid = room->monster;
    const MonsterPrototype *m = monsters_find_monster(mid);
    const char * monster_name = monsters_name_for_id(mid);
    int monster_tally = 0;
    int hero_tally = 0;
    // average stat is 10.5, sum of 6 average stats is 63. This is the total stats of an average monster.
    // higher than this is a stronger monster, lower is a weaker monster.
    double ferocity_multiplier = (m->ferocity_factor / 63.0 );
    hero_tally += gs->stats.as_array[stat1];
    hero_tally += gs->stats.as_array[stat2];
    hero_tally += weapon_bonus;

    monster_tally += m->stats.as_array[stat1];
    monster_tally += m->stats.as_array[stat2];
    // printf("before multiplying ff: ferocity_factor = %d, ferocity_multiplier=%g, monster_tally=%d\n", m->ferocity_factor, ferocity_multiplier, monster_tally);
    monster_tally = (int)(monster_tally * ferocity_multiplier);


    if (!GLOBALS.silent_mode) {
        if (hero_tally == monster_tally) {
            display_line("You are evenly matched.");
        } else if (hero_tally > monster_tally) {
            display_line("It looks like the odds are in favor of you.");
        } else {
            display_linef("It looks like the odds are in favor of the %s.", monster_name);
        }
        display_linef("The %s - %d", monster_name, monster_tally);
        display_linef("%s - %d", gs->player_name->buffer, hero_tally);
    }

    // we'll pause a bit after every turn during the fight
    uint32_t pause_seconds;
    if (GLOBALS.debug_mode ) {
        pause_seconds = 0;
    } else {
        pause_seconds = _1ms * 1000;
    }

    for (;;) {
        int attack_roll = roll_d6(gs, 1);
        switch (attack_roll) {
            case 0:
                display_line("You struck a splendid blow.");
                monster_tally--;
                break;
            case 1:
                display_linef("The %s strikes out.", monster_name);
                hero_tally--;
                gs->stats.strength--;
                gs->stats.charisma--;
                break;
            case 2:
                display_linef("You draw the %s's blood!", monster_name);
                monster_tally--;
                break;
            case 3:
                display_line("You are wounded!!");
                hero_tally -= rnd_range(gs, 1, 4);
                gs->stats.dexterity--;
                break;
            case 4:
                display_linef("The %s is tiring.", monster_name);
                monster_tally--;
                break;
            case 5:
                display_line("You are bleeding....");
                hero_tally -= 2;
                gs->stats.wisdom--;
                gs->stats.constitution--;
                break;
            case 6:
            default:
                display_linef("You wound the %s", monster_name);
                monster_tally--;
                break;
        }
        char_sleep((int32_t)pause_seconds);
        if (!(hero_tally > 0 && monster_tally > 0 && rnd_d(gs) < .75)) {
            break;
        }
    }

    if (hero_tally > monster_tally) {
        display_line("You have slain the beast.");
        gs->monsters_killed++;
    } else {
        display("The ");
        display(m->name);
        display_line(" got the better of you that time.");
        if (stat1 == STAT_STRENGTH || stat2 == STAT_STRENGTH) {
            gs->stats.strength = 4 * gs->stats.strength / 5;
        }
        if (stat1 == STAT_CHARISMA || stat2 == STAT_CHARISMA) {
            gs->stats.charisma = 3 * gs->stats.charisma / 4;
        }
        if (stat1 == STAT_DEXTERITY || stat2 == STAT_DEXTERITY) {
            gs->stats.dexterity = 6 * gs->stats.dexterity / 7;
        }
        if (stat1 == STAT_INTELLIGENCE || stat2 == STAT_INTELLIGENCE) {
            gs->stats.intelligence = 2 * gs->stats.intelligence / 3;
        }
        if (stat1 == STAT_WISDOM || stat2 == STAT_WISDOM) {
            gs->stats.wisdom = 5 * gs->stats.wisdom / 6;
        }
        if (stat1 == STAT_CONSTITUTION || stat2 == STAT_CONSTITUTION) {
            gs->stats.constitution = gs->stats.constitution / 2;
        }
    }

    room_clear_monster(room);
    //normalize any negative stats to 0
    for (int i = 0; i < STAT_COUNT; ++i) {
        if (gs->stats.as_array[i] < 0) {
            gs->stats.as_array[i] = 0;
        }
    }
    return true;
}



//// ------------------------------------------------------------
////
////    ACTOR FUNCTIONS
////
//// ------------------------------------------------------------

// Returns the object id of the first object in the actor's items[],
// or ROOM_OBJ_NOT_FOUND if there are no items
static int actor_first_object_id(const GameState *gs) {
    const int items_len = gs->items_len;
    for (int i = 0; i < items_len; ++i) {
        if (gs->items[i] != 0) {
            return gs->items[i];
        }
    }
    return OBJ_NOT_FOUND;
}

// return the object id for the given item_name. If a partial item_name is passed, performs a "starts with"
// search to match. But this will return the first object in the store that starts with the argument string,
// which may or may not be what you are looking for.
static int actor_object_id_for_partial_name(const GameState *gs, char const item_name[static 1]) {
    if (!item_name) return OBJ_NULL_OBJECT_NAME;
    const int items_len = gs->items_len;
    for (int i = 0; i < items_len; ++i) {
        if (gs->items[i] != 0) {
            const Object *o = obj_find_object(gs->items[i]);
            if (strncmp(item_name, o->name, strlen(item_name)) == 0) {
                // printf("actor_object_id_for_partial_name: item_name: %s, items[%d].name:%s, strlen:%zd\n",
                //        item_name, i, o->name, strlen(item_name));
                return o->id;
            }
        }
    }
    return OBJ_NOT_FOUND;
}



/**
 * Shared Validation: Can something be opened here?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_open(const GameState *gs, const bool verbose) {
    // todo (rob) simple implementation for now. This app has 2 chests,only the chest
    // in Room 40 has something in it.
    const Room *r = room_find_room(gs->room);
    if (room_contains_object(r, OBJECT_STONE_CHEST) ||
        room_contains_object(r, OBJECT_IRON_CHEST)) {
        return true;
    }
    if (verbose) display_line("There is nothing here to open.");
    return false;
}

bool action_open(GameState *gs, object_id id) {
    if (!can_open(gs, false)) {
        return false;
    }
    return obj_set_open_flag(id);
}

static bool cmd_open(GameState *gs, const struct ParsedCommand *pc) {
    if (!can_open(gs, true)) {
        return false;
    }
    const Room *r = room_find_room(gs->room);
    const Object *o;
    if (room_contains_object(r, OBJECT_IRON_CHEST)) {
        o = obj_find_object(OBJECT_IRON_CHEST);
    } else {
        o = obj_find_object(OBJECT_STONE_CHEST);
    }
    if (!o) return false;
    const object_id id = o->id;
    const bool open_flag = o->is_open_bit;

    if (id == OBJECT_IRON_CHEST && !open_flag) {
        // HAPPY PATH: opening the iron chest for the first time
        display_line("Inside you find a parchment with the following message:");
        display_line("        'A little man can be bound by gold.'");
    } else if (id == OBJECT_STONE_CHEST && !open_flag) {
        display_line("It is empty.");
    } else {
        if (rnd_d(gs) < .4) {
            display_line("It holds nothing but dust.");
        } else {
            display_line("It is empty.");
        }
    }
    return perform_action(gs, CMD_OPEN, id, 0, 0);
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

// Entry point for the human user path.
static bool cmd_drop(GameState *gs, const ParsedCommand *pc) {
    object_id id = 0;
    if (!pc->has_verb_object) {
        const int object_count = actor_count_of_objects(gs);
        if ( object_count == 0 ) {
            display_line("You are not carrying anything.");
            return false;
        }
        if ( object_count == 1) {
            // if only one object, we'll drop whatever the user carries
            id = actor_first_object_id(gs);
            if (id == ROOM_ERR_OBJECT_NOT_FOUND) {
                display_line("drop what?");
                return false;
            }
        }
    } else {
        // try to drop this object by name
        id = actor_object_id_for_partial_name(gs, pc->verb_object);
    }

    if (!can_drop_item(gs, id, true)) return false;

    return perform_action(gs, CMD_DROP, id, 0, 0);
}

/** Logic Entry Point: ML and Human both end up here */
static bool action_drop(GameState *gs, object_id id) {
    if (!can_drop_item(gs, id, false)) return false;
    if (!actor_remove_object(gs, id)) return false;

    const Room *r = room_find_room(gs->room);
    return room_add_object( r, id ) == ROOM_SUCCESS;
}


/**
 * Shared Validation: Can the item be be read?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_read_item(const GameState *gs, const object_id id, const bool verbose) {
    if (!id || !actor_has_item(gs, id)) {
        if (verbose) display_linef("You are not carrying that item. object_id:%d", id);
        return false;
    }
    const Object *o = obj_find_object(id);
    if ( !o ) {
        if (verbose) display_linef("unknown object id=%d", id);
        return false;
    }
    if ( !o->is_readable_bit) {
        if (verbose) display_linef("You can't read the %s", o->name);
        return false;
    }
    return true;
}

static bool action_read(GameState *gs, object_id id) {
    if (!can_read_item(gs, id, false)) return false;
    // todo (rob) the object needs to contain the text of what can be read.
    // in this current game there's only one scroll OBJECT_MYSTIC_SCROLL
    // all that happens when reading the scroll is text information presented to the user, which is handled in
    // cmd_read(). There is currently nothing else for this action_read() method to perform.

    return true;
}

// Entry point for the human user path.
// This finds the item in the VO, and passes those to
// read_action(), the ML entry point for the read action.
static bool cmd_read(GameState *gs, const ParsedCommand *pc) {
    if (!pc->has_verb_object) {
        display_line("read what?");
        return false;
    }
    // try to read this object by name
    const object_id id = actor_object_id_for_partial_name(gs, pc->verb_object);
    if ( id == OBJ_NOT_FOUND) {
        display_linef("You don't have a %s", pc->verb_object);
        return false;
    }

    // Pre-check: If the user doesn't have the item, skip the action
    if (!can_read_item(gs, id, true)) return false;

    if (id != OBJECT_MYSTIC_SCROLL ) return false;

    const double roll = rnd_d(gs);
    if (roll < .333)       display_line("It says 'The locks need special keys.'");
    else if (roll < .666 ) display_line("The scroll reads: 'Chests can contain aid.'");
    else                   display_line("It says 'The amulet is important.'");

    return perform_action(gs, CMD_READ, id, 0, 0);
}


/**
 * Shared Validation: Can we take the object? Make sure the object_id is valid and exists in the room
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_take_item(const GameState *gs, const object_id id, const bool verbose) {
    if (id < 1 || id >= obj_num_objects()) {
        if (verbose) display_line("What's that?");
        return false;
    }
    const Room *r = room_find_room(gs->room);
    if (room_index_for_object(r, id) == ROOM_ERR_OBJECT_NOT_FOUND) {
        if (verbose) {
            display_linef("object_id:%d", id);
            display_line("That object is not here.");
        }
        return false;
    }

    if (actor_count_of_objects(gs) >= MAX_PLAYER_OBJECTS) {
        if (verbose) display_linef("You are already carrying your maximum of %d objects.", MAX_PLAYER_OBJECTS);
        return false;
    }

    if (id == OBJECT_STONE_CHEST || id == OBJECT_IRON_CHEST) {
        if (verbose) display_line("It is far too heavy to lift.");
        return false;
    }

    return true;
}

/** Logic Entry Point: ML and Human both end up here */
static bool action_take(GameState *gs, const object_id id) {
    if (!can_take_item(gs, id, false)) return false;
    if (!actor_add_object(gs, id)) return false;

    const Room *r = room_find_room(gs->room);
    room_transfer_obj_location(r, id, PLAYER_LOCATION);
    return true;
}

static bool cmd_take(GameState *gs, const ParsedCommand *pc) {
    object_id id = 0;
    const Room *room = room_find_room(gs->room);
    if (!pc->has_verb_object) {
        if (room_count_of_objects(room) == 1) {
            // if only one object, we'll take whatever is in the room
            id = room_first_object_id(room);
            if (id == ROOM_ERR_OBJECT_NOT_FOUND) {
                display_line("take what?");
                return false;
            }
        } else {
            // multiple items in room with no verb-object specified
            display_line("take what?");
            return false;
        }
    } else {
        // try to take this object by name
        const Object *o = room_find_object_named(room, pc->verb_object);
        if (o) {
            id = o->id;
        } else {
            display_linef("I don't see any %s here.", pc->verb_object);
            return false;
        }
    }

    // Pre-check: make sure the object_index is valid and exists in the room
    if (!can_take_item(gs, id, true)) return false;

    bool success = perform_action(gs, CMD_TAKE, id, 0, 0);

    if (success) {
        // vdisplay_line("You now have the object_index:%d, %s", id, obj_name_for_object_id(id));
        display_linef("%s taken.", obj_name_for_id(id));
    }


    return success;
}

static void set_state_death_by_dwarf(GameState *gs) {
    gs->QU = 3;
    gs->is_dead = true;
    gs->game_over = true;
}

static bool can_pay(const GameState *gs, const monster_id unused, const bool verbose) {
    const Room *r = room_find_room(gs->room);
    const monster_id mid = r->monster;

    if (mid != MONSTER_DWARF) {
        return false;
    }
    return true;
}

/** Logic Entry Point: ML and Human both end up here */
bool action_pay(GameState *gs, const monster_id unused ) {
    if (!can_pay(gs, 0, false)) return false;
    const Room *r = room_find_room( gs->room);

    // the model action on success will be to
    // 1. Remove the dwarf from the room.
    // 2. Remove the amulet from the player's bag, set location to 0

    // on failure,
    // 1. 50% chance that the dwarf steals the first item in the player's bag.
    // 2. If the bag is empty, or if the first random check is the other 50%, the dwarf kills you and the game ends
    if (actor_has_item(gs, OBJECT_AMULET)) {
        display_line("Lucky for you that you had it!");
        room_clear_monster(r);
        actor_remove_object(gs, OBJECT_AMULET);
    } else {
        display_line("You do not have it!");
        if (rnd_d(gs) < .5 ) {
            display_line("He would accept anything that he really wants.\nBut you have nothing, and so he kills you.");
            set_state_death_by_dwarf(gs);
            return false;
        }
        display("He decides, however, to accept a 'gift' of");
        const object_id id = actor_first_object_id(gs);
        if (id == OBJ_NOT_FOUND) {
            display_line(" anything valuable.\nBut you have nothing and so he kills you.");
            set_state_death_by_dwarf(gs);
            return false;
        }
        const Object *o = obj_find_object(id);
        display_linef(" the %s", o->name);
        room_clear_monster(r);
        actor_remove_object(gs, id);
    }

    return true;
}

// Helper method that checks if the name in the argument matches the name of any monster in the
// current room. Performs a starts-width, case-insensitive search. Thus, if there is a "Dragon" in the room,
// search strings "Dragon", "DRAGON", "drag", etc., will all match. The search string is extracted from
// pc->verb_object. pc->verb is also used for error messages.
//
static bool verify_monster_choice(const GameState *gs, const ParsedCommand *pc) {
    const Room *current_room = room_find_room(gs->room);
    // case 1 no monster
    if (current_room->monster == 0) {
        if (pc->has_verb_object) {
            display_linef("There is no %s here to %s.", pc->verb_object, pc->verb);
        } else {
            display_linef("There is nothing here to %s.", pc->verb);
        }
        return false;
    }

    if (pc->has_verb_object ) {
        // player is referencing a monster by name
        const bool is_present = monsters_monster_is_in_room(pc->verb_object, current_room );
        if (!is_present) {
            display_linef("There is no %s here.", pc->verb_object);
            return false;
        }
    }
    return true;
}


static bool cmd_pay(GameState *gs, const ParsedCommand *pc) {
    const Room *current_room = room_find_room(gs->room);
    if (! verify_monster_choice(gs, pc) ) return false;

    // case 2, monster is present
    const MonsterPrototype *m = monsters_find_monster(current_room->monster);
    // player is paying a monster by name
    if ( current_room->monster != MONSTER_DWARF ) {
        display_linef("You cannot %s the %s", pc->verb, m->name);
        //monster_is_insulted= true;
        return false;
    }

    if (!can_pay(gs, MONSTER_DWARF, true)) return false;

    // here we know the room has a dwarf and the command was properly formatted
    display_line("He demands the amulet!");

    return perform_action(gs, CMD_PAY, MONSTER_DWARF, 0, 0);
}

static bool can_drink_item(const GameState *gs, const object_id id, const bool verbose) {
    if (!id || !actor_has_item(gs, id)) {
        if (verbose) display_linef("You are not carrying that item. object_id:%d", id);
        return false;
    }
    const Object *o = obj_find_object(id);
    if ( !o ) {
        if (verbose) display_linef("unknown object id=%d", id);
        return false;
    }
    if ( !o->is_drinkable_bit) {
        if (verbose) display_linef("You can't drink the %s", o->name);
        return false;
    }
    return true;
}

static bool action_drink( GameState *gs, object_id id) {
    if (!can_drink_item(gs, id, false)) return false;
    // currently there is only one potion in the game
    if (id != OBJECT_HEALING_POTION) return false;

    // make strength 20 and increase all other stats by 2
    for (int i = 0; i < STAT_COUNT; ++i) {
        gs->stats.as_array[i] += 2;
    }
    gs->stats.strength = 20;
    actor_remove_object(gs, id);

    return true;
}

// Entry point for the human user path.
// This finds the item in the VO, and passes those to
// drink_action(), the ML entry point for the drink action.
static bool cmd_drink(GameState *gs, const ParsedCommand *pc) {
    // this command requires a verb object.
    // DESIGN: some commands require vo, some do not, some are optional.
    // This code can be pulled up and reused whenever a verb requires a vo
    // but one was not provided, or when the vo doesn't match anything in the user's inventory
    if (!pc->has_verb_object) {
        display_linef("%s what?", pc->verb);
        return false;
    }
    // try to read this object by name
    const object_id id = actor_object_id_for_partial_name(gs, pc->verb_object);
    if ( id == OBJ_NOT_FOUND) {
        display_linef("You don't have a %s to %s", pc->verb_object, pc->verb);
        return false;
    }

    // Pre-check: If the user doesn't have the item, skip the action
    if (!can_drink_item(gs, id, true)) return false;

    // currently there is only one potion in the game
    if (id == OBJECT_HEALING_POTION) {
        display_line("You are instantly filled with healing, and your strength is restored.");
        display_line("The bottle holding the potion magically fades from view.");
    }

    return perform_action(gs, CMD_DRINK, id, 0, 0);
}


static bool can_unlock_room(const GameState *gs, const room_id room, const object_id key, const bool verbose) {
    const int num_rooms = room_num_rooms();

    if (room < 1 || room >= num_rooms) {
        if (verbose) display_linef("Room id out of bounds: %d", room);
        return false;
    }
    if (key != OBJECT_SILVER_KEY && key != OBJECT_GOLD_KEY) {
        if (verbose) display_linef("Invalid key id: %d", key);
    }
    if (! actor_has_item(gs, key)) {
        const char *obj_name = obj_name_for_id(key);
        if (verbose) display_linef("You don't have the %s.", obj_name);
        return false;
    }
    if (ROOM_GRAPH[room][RGINDEX_REQUIRED_KEY] == 0 ) {
        if (verbose) display_line("There are no locked doors here. ");
        return false;
    }
    if (ROOM_GRAPH[room][RGINDEX_REQUIRED_KEY] != key) {
        if (verbose) display_line("That key does not fit the door.");
        return false;
    }

    return true;
}

static bool action_unlock( GameState *gs, const room_id room, const object_id key ) {
    if (!can_unlock_room(gs, room, key, false)) return false;

    ROOM_GRAPH[room][RGINDEX_REQUIRED_KEY] = 0; // door is unlocked now.
    actor_remove_object(gs, key);
    display_line("There is a creak as the key turns.\nThe door is now unlocked.");
    return true;
}



static bool cmd_unlock(GameState *gs, const ParsedCommand *pc) {

    // check if unlock is needed
    const room_id current_room = gs->room;
    // until we can place an object in a room like a door guard for a particular edge, we have to kludge this here.
    // For the current room we look at all possible traversable edges.
    struct room_key { room_id room; object_id key;} room_key = {};
    for (int direction = RGINDEX_NORTH; direction <= RGINDEX_DOWN; ++direction) {
        room_id next_room =  ROOM_GRAPH[current_room][direction];
        // kludge method stops at first locked door found
        if ( next_room > 0 && ROOM_GRAPH[next_room][RGINDEX_REQUIRED_KEY] > 0 ) {
            room_key.room = next_room;
            room_key.key = ROOM_GRAPH[next_room][RGINDEX_REQUIRED_KEY] ;
            break;
        }
    }

    if (room_key.room == 0) {
        display_line("There is nothing to unlock here.");
        return false;
    }


    if (!actor_has_item(gs, room_key.key)) {
        display_line("You do not have the key.");
        return false;
    }

    if (!can_unlock_room(gs, room_key.room, room_key.key, true)) return false;

    return perform_action(gs, CMD_UNLOCK, room_key.room, room_key.key, 0);
}

// debugging tool, increase player stats to max
// useful while debugging so monsters don't kill player
static bool cmd_god_mode(GameState *gs, const ParsedCommand *pc) {
    for (int i = 1; i < STAT_COUNT; ++i) {
        gs->stats.as_array[i] = 18;
    }
    return true;
}
/**
  * Core Game Engine Logic
  * This function is "Pure Logic" - it updates state based on an action.
  * It returns true if the action was accepted as a turn, false otherwise.
  *
  * @param gs
  * @param cmd enum Command
  * @param arg1 For CMD_FIGHT: strategy (1:magic, 2:skill).
  *             For CMD_MOVE: ASCII value for one of 'NSEWUD'
  *             For CMD_DROP: object_index of the object to drop
  *             For CMD_TAKE: object_index of the object to take
  * @param arg2 For 'F': first skill stat index.
  * @param arg3 For 'F': second skill stat index.
  *
  *
  */
bool perform_action(GameState *gs, enum Command const cmd, const int arg1, const int arg2, const int arg3) {
    gs->turns++;

    bool result = false;

    switch (cmd) {
        case CMD_FIGHT:
            result = action_fight(gs, arg1, (enum StatIndex) arg2, (enum StatIndex) arg3);
            break;
        case CMD_MOVE:
            result = action_move(gs, arg1);
            break;
        case CMD_DROP:
            result = action_drop(gs, arg1);
            break;
        case CMD_TAKE:
            result = action_take(gs, arg1);
            break;
        case CMD_OPEN:
            result = action_open(gs, arg1);
            break;
        case CMD_READ:
            result = action_read(gs, arg1);
            break;
        case CMD_PAY:
            result = action_pay(gs, arg1);
            break;
        case CMD_DRINK:
            result = action_drink(gs, arg1);
            break;
        case CMD_UNLOCK:
            result = action_unlock(gs, arg1, arg2);
            break;
        default:
            result = false;
            break;
    }
    return result;
}


// Entry point for the human user path. This displays some information, prompts the user for some choices,
// and passes those to perform_action(), the ML entry point for the fight action.
static bool cmd_fight(GameState *gs, const ParsedCommand *pc) {
    if (!verify_monster_choice(gs, pc)) return false;

    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == MONSTER_DWARF) {
        display_line("The dwarf refuses to fight and his magic protects him.");
        return false;
    }
    const Room *r = room_find_room(gs->room);

    const monster_id mid = r->monster;
    MonsterPrototype *m = monsters_find_monster(mid);
    const char * monster_name = monsters_name_for_id(mid);
    display_line("--------------------------------------");
    display_linef("Your opponent is a %s.", monster_name);
    int ferocity_factor = m->ferocity_factor;

    display_linef("The %s's danger level is %d", monster_name, ferocity_factor);

    // todo : possibly a clang bug that erroneously warns
    // " variable length array folded to constant array as an extension [-Werror,-Wgnu-folding-constant]"
    // if I just use  T[MAX_ITEMS] below, as of 6/5/2026
    int max_items = MAX_PLAYER_OBJECTS;
    int T[max_items] = {};
    // see what weapons the user has. Object ids 1 (axe) to 7 (falchion)
    const int items_len = gs->items_len;
    for (int j = 0; j < items_len; ++j) {
        T[j] = 0;
        switch (gs->items[j]) {
            case OBJECT_AXE:
                display_line("Your axe could be handy.");
                T[j] = OBJECT_AXE;
                break;
            case OBJECT_SWORD:
                display_line("Your skill with the sword may stand you in good stead.");
                T[j] = OBJECT_SWORD;
                break;
            case OBJECT_DAGGER:
                display_linef("Your dagger is useful against %ss.", monster_name);
                T[j] = OBJECT_DAGGER;
                break;
            case OBJECT_MACE:
                display_line("The mace will make short work of it.");
                T[j] = OBJECT_MACE;
                break;
            case OBJECT_QUARTER_STAFF:
                display_line("Your quarterstaff will give it no quarter.");
                T[j] = OBJECT_QUARTER_STAFF;
                break;
            case OBJECT_MORNING_STAR:
                display_linef("Swinging your morning star may inflict heavy wounds on the %s.", monster_name);
                T[j] = OBJECT_MORNING_STAR;
                break;
            case OBJECT_FALCHION:
                display_line("A falchion is a useful weapon.");
                T[j] = OBJECT_FALCHION;
                break;
            default: break;
        }
    }
    int weapon_count = 0;
    int last_weapon = 0;
    for (int i = 0; i < items_len; ++i) {
        if (T[i]) {
            ++weapon_count;
            last_weapon = T[i];
        }
    }
    int weapon_choice = 0;
    if (weapon_count == 0) {
        display_linef("You must fight the %s with your bare hands.", monster_name);
    } else if (weapon_count == 1) {
        weapon_choice = last_weapon;
        const Object *o = obj_find_object(weapon_choice);
        const char *weapon_name = o ? o->name : "unknown weapon";
        display_linef("You must fight with your %s.", weapon_name);
    } else {
        display_line("choose your weapon: ");
        for (int j = 0; j < items_len; ++j) {
            if (T[j]) {
                const object_id id = T[j];
                const Object *o = obj_find_object(id);
                if (!o) continue;
                display_linef("%d - %s", (j+1), o->name);
            }
        }
        int choice = 0;
        for (;;) {
            choice = get_int("Enter the number to choose: ", 1, items_len);
            if (!T[choice - 1]) {
                display_line("Invalid item.");
            } else {
                break;
            }
        }
        weapon_choice = T[choice - 1];
        const Object *o = obj_find_object(weapon_choice);
        const char *weapon_name = o ? o->name : "unknown weapon";
        if (o) {
            display_linef("Right, so you choose to fight with the  %s.", weapon_name);
        }

    }

    display_linef("The %s has the following stats:", monster_name);
    display_char_attributes(m->stats);
    display_line("\nYour stats are:");
    display_char_attributes(gs->stats);
    display_line("Which attributes to fight with? (choose 2):");
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
    return perform_action(gs, CMD_FIGHT, weapon_choice, first_skill, second_skill);
}


/**
 * Death and Win condition check
 * RETURNS: true if the game is over (win or loss).
 * The caller should check gs->is_dead or gs->completed to see the outcome.
 */
bool check_game_over(GameState *gs) {
    if (gs->game_over) return true;
    const room_id room = gs->room;

    if ( room == ROOM_END ||
         room == ROOM_STONE ||
         ( room >= ROOM_TRAPPED && gs->room <= ROOM_SPIDER ) ||
         room == ROOM_GARGOYLE) {

        gs->game_over = true;
        if (room == ROOM_END) {
            gs->completed = true;
            gs->SC = 100;
        } else {
            gs->is_dead = true;
            if (room == ROOM_STONE) {
                gs->QU = 2;
                gs->SC = 50;
            } else if (room == ROOM_TRAPPED ) {
                gs->QU = 3.5;
                gs->SC = 100; // todo (rob) this makes no sense.
            } else if (room == ROOM_PIT_OF_FLAMES ) {
                gs->QU = 3.4;
                gs->SC = 10;
            } else if (room == ROOM_ACID ) {
                gs->QU = 3;
                gs->SC = 20;
            } else if (room == ROOM_SPIDER ) {
                gs->QU = 5;
                gs->SC = 3;
            }  else if (room == ROOM_GARGOYLE ) {
                gs->QU = 0;
                gs->SC = 3;
            }
        }

        return true;
    }

    for (int i = STAT_STRENGTH; i < STAT_COUNT; ++i) {
        if (gs->stats.as_array[i] <= 0) {
            if (!GLOBALS.silent_mode) {
                display_char_attributes(gs->stats);
                display_line("You are exhausted, so this adventure must end.");
                gs->QU = 2;
            }
            gs->is_dead = true;
            gs->game_over = true;
            return true;
        }
    }
    return false;
}

// return true if stats are too low to continue (end game) otherwise return false.
static bool adjust_stats(GameState *gs) {
    // we just lose strength points randomly here for some reason.
    if (rnd_d(gs) < .16) --gs->stats.strength;

    // clamp stats to min 0
    actor_clamp_stats(gs, 0, INT_MAX);
    for (int i = STAT_STRENGTH; i < STAT_COUNT; ++i) {
        if (gs->stats.as_array[i] == 0) {
            gs->stats.as_array[i] = 0;
        }
    }

    return check_game_over(gs);
}

void do_room_actions(GameState *gs) {
    // process dwarf
    if (gs->room == ROOM_YELLOW &&
        (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == MONSTER_DWARF) &&
        rnd_d(gs) < .16) {
        display_paginated("You hear a whispered voice warning you: 'You must do something about the dwarf.'",
                          80);
    } else if (gs->room == ROOM_CHARISMA_REDUCE ) {
        gs->stats.charisma--;
    }
}


ParsedCommand parse_user_command(char const *prompt, char const *err_msg) {
    constexpr size_t LINE_BUFFER_SIZE = 1024;
    ParsedCommand pc = {};

    for (;;) {
        char line[LINE_BUFFER_SIZE] = {};
        display(prompt);
        if (!fgets(line, sizeof(line), stdin)) {
            continue;
        }
        // Remove newline if present
        line[strcspn(line, "\n")] = 0;
        //Defensive measure to ensure that the string is always properly terminated within allocated bounds.
        line[LINE_BUFFER_SIZE - 1] = 0;
        pc = parse_cmd_string(line);
        const enum Command cmd = pc.verb_command;

        // printf("parse_user_command():    vc:%d, voc:%d   verb:'%s', verb_obj:'%s', has_vobj?:%d\n",
        //        pc.verb_command, pc.verb_object_command, pc.verb, pc.verb_object, pc.has_verb_object);

        if (cmd < 0) {
            display_line(err_msg);
            continue;
        }
        break;
    }


    return pc;
}




//// ------------------------------------------------------------
////
////    DEBUGGING
////
//// ------------------------------------------------------------

void display_all_room_desc() {
    const uint32_t saved_sleep = GLOBALS.char_sleep_duration;

    set_char_sleep(0);
    const int num_rooms = room_num_rooms();

    for (int i = 1; i < num_rooms; ++i) {
        const Room *r = room_find_room(i);
        display_line("");
        display_line(r->name);
        display_line("-------------------------------------------------------------------");
        display_paginated(r->desc, 80);
    }

    // restore previous sleep duration
    set_char_sleep(saved_sleep);
}




//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------


// -----------------------------------------------------------------
//      called at the start of each new game
// -----------------------------------------------------------------
void reset(GameState *gs, const uint32_t seed) {
    // reset GameState
    *gs = (GameState){ .seed = seed, .player_name = GLOBALS.player_name, .room = ROOM_START, .has_torch = true, .QU = 1};

    mt_initialize_state(&gs->mt_state, seed); // initialize the PRNG

    gs->stats = random_hero_stats(gs);

    //clear all monsters, treasure
    const int num_rooms = room_num_rooms();
    for (int room_index = 0; room_index < num_rooms; ++room_index) {
        // note: if we dynamically modify the edge graph, we'll need to reset those edges here
        ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
        ROOM_GRAPH[room_index][RGINDEX_MONSTER] = 0;
        ROOM_GRAPH[room_index][RGINDEX_REQUIRED_KEY] = 0;
        ROOM_GRAPH[room_index][RGINDEX_UNUSED] = 0;
        const Room *r = room_find_room(room_index);
        room_clear_monster( r );
        room_remove_all_objects(room_index);
    }
    monsters_clear_all();

    ROOM_GRAPH[ROOM_KITCHEN][RGINDEX_REQUIRED_KEY]     = OBJECT_SILVER_KEY; // locked door i, requires silver key
    ROOM_GRAPH[ROOM_UNEVEN][RGINDEX_REQUIRED_KEY]      = OBJECT_GOLD_KEY; // locked door ii, requires golden key


    // -----------------------------------------------------------------
    //      NEW OBJECT ALLOCATION METHOD
    // -----------------------------------------------------------------

    room_add_object(room_find_room(ROOM_MAGICIAN), OBJECT_SILVER_KEY);
    room_add_object(room_find_room(ROOM_WOODEN), OBJECT_SWORD);
    room_add_object(room_find_room(ROOM_DUNGEON), OBJECT_AXE);
    room_add_object(room_find_room(ROOM_CHARISMA_REDUCE), OBJECT_STONE_CHEST);
    room_add_object(room_find_room(ROOM_TROPHY), OBJECT_IRON_CHEST);
    room_add_object(room_find_room(ROOM_SECRET_ROOM), OBJECT_AMULET);
    room_add_object(room_find_room(ROOM_TURRET), OBJECT_GOLD_KEY);


    // allot random treasure
    for (int treasure_index = OBJECT_DAGGER; treasure_index <= OBJECT_DIADEM; ++treasure_index) {
        // if object has already been assigned to a room, skip this iteration.
        if (obj_find_object(treasure_index)->location != 0 ) {
            continue;
        }

        for (;;) {
            // todo (rob) this is an inefficient check. Put valid rooms in a list, shuffle the list, choose first N rooms
            int rand_room = rnd_range(gs, 1, num_rooms);
            const Room *r = room_find_room(rand_room);
            if (!( r->objects_len > 0 ||
                  rand_room == ROOM_START ||
                  rand_room == ROOM_END ||
                  rand_room == ROOM_STONE ||
                  rand_room == ROOM_CRAMPED ||
                  rand_room == ROOM_TRAPPED ||
                  (rand_room >= ROOM_TRAPPED && rand_room <= ROOM_SPIDER) ||
                  rand_room == ROOM_GARGOYLE)) {
                // new way to manage objects
                room_add_object(room_find_room( rand_room ), treasure_index);
                break;
            }
        }
    }

    // special monster locations
    ROOM_GRAPH[ROOM_YELLOW][RGINDEX_MONSTER] = MONSTER_DWARF;
    room_set_monster(room_find_room(ROOM_YELLOW), MONSTER_DWARF);
    CharStats stats = random_monster_stats(gs);
    int ff = sum_character_stats(&stats);
    monsters_update_monster( &(MonsterPrototype) {
                                    .name = monsters_name_for_id(MONSTER_DWARF),
                                    .id = MONSTER_DWARF,
                                    .ferocity_factor = ff,
                                    .stats = stats,
    });

    // allot random monsters
    for (int monster_index = MONSTER_DWARF + 1; monster_index < monsters_num_monsters(); ++monster_index) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, num_rooms);
            if (!(ROOM_GRAPH[rand_room][RGINDEX_MONSTER] ||
                  rand_room == ROOM_START ||
                  rand_room == ROOM_END ||
                  rand_room == ROOM_STONE ||
                  rand_room == ROOM_CRAMPED ||
                  rand_room == ROOM_TRAPPED ||
                  (rand_room >= ROOM_TRAPPED && rand_room <= ROOM_SPIDER) ||
                  rand_room == ROOM_GARGOYLE)) {
                ROOM_GRAPH[rand_room][RGINDEX_MONSTER] = monster_index;
                stats = random_monster_stats(gs);
                ff = sum_character_stats(&stats);
                room_set_monster(room_find_room(rand_room), monster_index);
                monsters_update_monster( &(MonsterPrototype) {
                            .name = monsters_name_for_id(monster_index),
                            .id = monster_index,
                            .ferocity_factor = ff,
                            .stats = stats
                });

                break;
            }
        }
    }

    // update_perception(gs);
}

static void init_string_assets() {
    // this will eventually be loaded from a text file
    global_string_assets.conclusion_completed = "You have succeeded!\nYou have escaped the Chateau Gaillard.\nWell done!";
    global_string_assets.conclusion_died      = "You have died.........";
}

static void init_rooms(void) {
    // random text for rooms 4
    RandomTextArray *rta = create_rta(1);
    rta->lines[0] = (RandomText){
        .chance_percent = .5,
        .text="A mouse scampers across the floor.",
        .else_text = "A bat flits across the ceiling."};

    room_set_epilog(4, rta);

    // special code for
    // room 32 counts down from 10 to 1 as you die from a spider bite
    // todo (rob) more console display features like a countdown
}

static constexpr size_t num_roomz = 45;
typedef struct RoomData {
    size_t size;
    Room data[num_roomz];
} RoomData;

static RoomData get_room_data(void) {
    return (RoomData){
        .size = num_roomz,
        .data = {
        {.id =  0,  .name= "NULL ROOM",  .desc = "" },
        {.id =  1,  .name= "Battlements",     .desc = "You are out on the battlements of the Chateau. There is only one way back." },
        {.id =  2,  .name= "Magician's Room",     .desc = "This is an eerie room, where once magicians consorted with evil sprites and werebeasts. Exits lead in three directions. An evil smell comes from the south." },
        {.id =  3,  .name= "Straw Mattress",     .desc = "An old straw mattress lies in one corner. It has been ripped apart to find any treasure which was hidden in it. Light comes fitfully from a window to the north, and around the doors to south, east, and west." },
        {.id =  4,  .name= "Wooden Panels",     .desc = "This wooden-panelled room makes you feel damp and uncomfortable. There are three doors leading from this room, one made of iron. Your sixth sense warns you to choose carefully..." },
        {.id =  5,  .name= "Living Stone",     .desc = "You ignore your intuition... A Spell of Living Stone, primed to trap the first intruder has been set on you. With your last seconds of life you have time only to feel profound regret..." },
        {.id =  6,  .name= "L-Shaped Room",     .desc = "You are in an L-shaped room. Heavy parchment lines the walls. You can see through an archway to the east, but that is not the only exit from this room." },
        {.id =  7,  .name= "Archway",     .desc = "There is an archway to the west, leading to an L-shaped room. A door leads in the opposite direction." },
        {.id =  8,  .name= "Kitchen",     .desc = "This must be the Chateau's main kitchen, but any food left here has long rotted away. A door leads to the north, and there is one to the west." },
        {.id =  9,  .name= "Black Dragon",     .desc = "You find yourself in a small room, which makes you feel claustrophobic. There is a picture of a black dragon painted on the north wall, above the door." },
        {.id = 10,  .name= "Landing",    .desc = "A stairwell ends in this 'room', which is more of a landing than an actual room. The door to the north is made of iron, which has rusted over the centuries." },
        {.id = 11,  .name= "Stone Archway",    .desc = "There is a stone archway to the north. You are in a very long room.\nFresh air blows down some stairs and rich red drapes cover the walls. You can see doors to the east." },
        {.id = 12,  .name= "Whirling Smoke",    .desc = "You have entered a room filled with swirling, choking smoke. You must leave quickly to remain healthy enough to continue your chosen quest." },
        {.id = 13,  .name= "Charism Reduction",    .desc = "There is a mirror in the corner. You glance at it, and feel suddenly very ill.\nYou realize the looking-glass has been infused with a Spell of Charisma Reduction... oh dear...." },
        {.id = 14,  .name= "White Marble",    .desc = "This room is richly finished with a white marble floor. Strange footprints lead to the two doors from this room. Dare you follow them?" },
        {.id = 15,  .name= "Red Drapes",    .desc = "You are in a long, long hallway, lined on each side with rich, red drapes.\nThey are parted halfway down the east wall where there is a door." },
        {.id = 16,  .name= "Yellow Room",    .desc = "Someone has spent a long time painting this room a bright yellow.\nYou remember reading that yellow is the Ancient Oracle's Color of Warning..." },
        {.id = 17,  .name= "Ladder",    .desc = "As you stumble down the ladder you fall into the room. The ladder crashes down behind you. There is now no way back.\nA small door leads east from this very cramped room." },
        {.id = 18,  .name= "Hall of Mirrors",    .desc = "You find yourself in the Hall of Mirrors, and see yourself reflected a hundred times or more. Through the bright glare you can make out doors in all directions. You notice the mirrors around the east door are heavily tarnished." },
        {.id = 19,  .name= "Long Corridor",    .desc = "You find yourself in a long corridor... Your footsteps echo as you walk." },
        {.id = 20,  .name= "Timbered Ceiling",    .desc = "You feel as if you've been wandering around this Chateau forever, and you begin to despair of ever escaping.\nStill, you can't get too depressed but must struggle on. Looking around, you see that you are in a room which has a heavy timbered ceiling and white roughly-finished walls.\nThere are two doors..." },
        {.id = 21,  .name= "Alcove",    .desc = "You are in a small alcove. You look around, but can see nothing in the gloom. Perhaps if you wait a while your eyes will adjust to the murky dark of this alcove." },
        {.id = 22,  .name= "Courtyard",    .desc = "A dried-up fountain stands in the center of this courtyard, which once held beautiful flowers but have have long since died." },
        {.id = 23,  .name= "Dying Flowers",    .desc = "The scent of dying flowers fills this brightly-lit room.\nThere are two exits from it." },
        {.id = 24,  .name= "Cavern",    .desc = "This is a round stone cavern off the side of the alcove to your north." },
        {.id = 25,  .name= "Games Room",    .desc = "You are in an enormous circular room, which looks as if it was used as a games room. Rubble covers the floor, partially blocking the only exit." },
        {.id = 26,  .name= "Potting Shed",    .desc = "Through the dim mustiness of this small potting shed you can see a stairwell." },
        {.id = 27,  .name= "Ramshackle Shed",    .desc = "You begin this Adventure in a small wood outside the Chateau.\nWhile out walking one day, you come across a small, ramshackle shed in the woods. Entering it, you see a hole in one corner. An old ladder leads down from the hole." },
        {.id = 28,  .name= "End",    .desc = "How wonderful! Fresh air, sunlight, birds are singing. You are free at last." },
        {.id = 29,  .name= "Death Trap",    .desc = "The smell came from bodies rotting in huge traps. One springs shut on you, trapping you forever!" },
        {.id = 30,  .name= "Hot hot hot",    .desc = "You fall into a pit of flames." },
        {.id = 31,  .name= "Acid Pool",    .desc = "Aaaaahhh... you have fallen into a pool of acid. Now you know - too late - why the mirrors were so badly tarnished." },
        {.id = 32,  .name= "Spider",    .desc = "It's too bad you chose that exit from the alcove. A giant funnel-web spider leaps on you, and before you can react, bites you on the neck. You have 10 seconds to live." },
        {.id = 33,  .name= "Hovel",    .desc = "A stairwell leads into this room, a poor and common hovel with many doors and exits." },
        {.id = 34,  .name= "Uneven Floor",    .desc = "It is hard to see in this room and you slip slightly on the uneven, rocky floor." },
        {.id = 35,  .name= "Torture Chamber",    .desc = "Horrors! This room was once the torture chamber of the Chateau.\nSkeletons lie on the floor, still with chains around their bones." },
        {.id = 36,  .name= "Dungeon",    .desc = "Another room with very unpleasant memories.\nThis foul hole was used as the Chateau dungeon." },
        {.id = 37,  .name= "Gargoyle",    .desc = "Oh no, this is a gargoyle's lair. It has been held prisoner here for three hundred years.\nIn his frenzy he thrashes out at you and... breaks your neck!!" },
        {.id = 38,  .name= "Dancing Hall",    .desc = "This was the Lower Dancing Hall. With doors to the north, the east, and to the west, you would seem to be able to flee any danger." },
        {.id = 39,  .name= "Dingy Pit",    .desc = "This is a dingy pit at the foot of some extremely dubious-looking stairs. A door leads to the east." },
        {.id = 40,  .name= "Trophy Room",    .desc = "Doors open to each compass point from the Trophy Room of the Chateau.\nThe heads of strange creatures shot by the ancestral owners are mounted high up on each wall." },
        {.id = 41,  .name= "Secret Room",    .desc = "You have stumbled on to a secret room.\nDown here, eons ago, the ancient Necromancers of Thorin plied their evil craft... and the remnant of their spells hangs heavy on the air." },
        {.id = 42,  .name= "Room of Shadows",    .desc = "Cobwebs brush your face as you make your way through the gloom of this room of shadows." },
        {.id = 43,  .name= "Gloomy Passage",    .desc = "This gloomy passage lies at the intersection of three rooms." },
        {.id = 44,  .name= "Rear Turret",    .desc = "You are in the rear turret room, below the extreme western wall of the ancient Chateau." },
        }
    };
}

constexpr size_t num_objectz = 20;
typedef struct ObjectData {
    size_t size;
    Object data[num_objectz];
} ObjectData;

static ObjectData get_object_data(void) {
    return (ObjectData){
        .size = num_objectz,
        .data = {
            {.id = 1, .name = "axe", .is_weapon = true },
            {.id = 2, .name = "sword", .is_weapon = true },
            {.id = 3, .name = "dagger", .is_weapon = true },
            {.id = 4, .name = "mace", .is_weapon = true },
            {.id = 5, .name = "quarterstaff", .is_weapon = true },
            {.id = 6, .name = "morning star", .is_weapon = true },
            {.id = 7, .name = "falchion", .is_weapon = true },
            {.id = 8, .name = "crystal ball", .value = 99},
            {.id = 9, .name = "amulet", .value = 247},
            {.id = 10, .name = "ebony ring", .value = 166},
            {.id = 11, .name = "gems", .value = 462},
            {.id = 12, .name = "mystic scroll", .value = 195,. is_readable_bit = true},
            {.id = 13, .name = "healing potion", .value = 231, .is_drinkable_bit = true},
            {.id = 14, .name = "dilithium crystals", .value = 162},
            {.id = 15, .name = "copper pieces", .value = 27},
            {.id = 16, .name = "diadem", .value = 141},
            {.id = 17, .name = "silver key"},
            {.id = 18, .name = "golden key"},
            {.id = 19, .name = "chest of stone"},
            {.id = 20, .name = "chest made of iron"},
        }
    };
}

// once time inits. Per-game inits happen in reset()
static void initialize() {
    // note: randomized data is initialized in reset()
    RoomData rd = get_room_data();
    room_init(rd.size,rd.data);

    parser_init();
    monsters_init("monsters.json");

    monsters_all_repr();

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


static void cleanup(GameState *gs) {
    room_destroy();
    void *free_ptr = (void *) gs->player_name;
    GLOBALS.player_name = nullptr;
    gs->player_name = nullptr;
    free(free_ptr);
    monsters_destroy();
    obj_destroy();
}




//// ------------------------------------------------------------
////
////    MAIN
////
//// ------------------------------------------------------------

static bool main_game_loop(GameState *gs) {
    uint32_t saved_sleep_duration = GLOBALS.char_sleep_duration;
    const room_id room_id = gs->room;
    const Room *current_room = room_find_room(room_id);
    room_set_visit_started_flag(current_room);

    if (current_room->is_visited_bit) {
        // if we've already seen this room, speed up output display
        if ( GLOBALS.debug_mode ) {
            set_char_sleep( GLOBALS.debug_visited_sleep );
        } else {
            set_char_sleep(GLOBALS.char_sleep_visited_duration);
        }
    }

    if (gs->room != gs->room_last_turn) {
        // only display room desc once when first entering room. Reduces screen clutter and scrolling.
        // user can always type "look" to re-display room desc.
        display_line("");
        display_room_desc(gs);
        display_room_content(gs);
    }

    if (check_game_over(gs)) {
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return END_GAME;
    }

    const monster_id id = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if (id > MONSTER_DWARF && rnd_d(gs) < .3) {
        // forced fight, monster attacks first
        display("The ");
        display(monsters_name_for_id(id));
        display_line(" attacks!");
    } else {
        // we just lose points randomly here for some reason.
        if (adjust_stats(gs)) {
            set_char_sleep(saved_sleep_duration);
            return END_GAME;
        }
    }


    // todo (rob) we need a framework hook for action routines for rooms and objects
    do_room_actions(gs);

    // speed up the display of text for the rest of the turn.
    if ( GLOBALS.debug_mode ) {
        set_char_sleep( GLOBALS.debug_visited_sleep );
    } else {
        set_char_sleep(GLOBALS.char_sleep_visited_duration);
    }



    // process user input
    flush_input();
    char prompt_buffer[1024] = {};
    snprintf(prompt_buffer, sizeof(prompt_buffer), "\n%s >", current_room->name);
    const ParsedCommand pc = parse_user_command( prompt_buffer, "I don't know how to do that.");
    const enum Command cmd = pc.verb_command;

    if (cmd == CMD_QUIT) {
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return cmd_quit(gs);
    }

    // ---------------------------------------------------------------------
    //      Player Presentation Only - doesn't require changes to the model
    // ---------------------------------------------------------------------

    switch (cmd) {
        case CMD_DEBUG: {
            if (pc.verb[0] == '1') {
                display_globals();
            }
            break;
        }
        case CMD_HELP: {
            display_paginated("No help for mortals in this game! Although, reading and drinking may help...", 80);
            break;
        }
        case CMD_LOOK: {
            cmd_look(gs);
            break;
        }
        case CMD_INV: {
            actor_display_inventory(gs, true, true);
            break;
        }
        case CMD_STATS: {
            display_char_attributes(gs->stats);
            break;
        }
        case CMD_SCORE: {
            display_score(gs);
            break;
        }
        case CMD_GOD: {
            cmd_god_mode(gs, &pc);
            break;
        }

        default:
            break;
    }

    // -----------------------------------------------------------------
    //      Engine/Model Commands
    // -----------------------------------------------------------------

    if (cmd == CMD_FIGHT) {
        //specialized code to prompt user and gather options to pass to perform_action()
        cmd_fight(gs, &pc);
    } else if (cmd == CMD_DROP) {
        cmd_drop(gs, &pc);
    } else if (cmd == CMD_TAKE) {
        cmd_take(gs, &pc);
    } else if (cmd == CMD_MOVE) {
        cmd_move(gs, &pc);
    } else if (cmd == CMD_OPEN) {
        cmd_open(gs, &pc);
    } else if (cmd == CMD_READ) {
        cmd_read(gs, &pc);
    } else if (cmd == CMD_PAY) {
        cmd_pay(gs, &pc);
    } else if (cmd == CMD_DRINK) {
        cmd_drink(gs, &pc);
    } else if (cmd == CMD_UNLOCK) {
        cmd_unlock(gs, &pc);
    } else {
        // Now the human call and the ML call use the exact same entry point
        perform_action(gs, cmd, 0, 0, 0);
    }

    set_char_sleep(saved_sleep_duration);

    // display_line("");
    if (room_id == gs->room) {
        // if room at end of turn is same as start of turn, update this so we
        gs->room_last_turn = room_id;
    } else {
        gs->room_last_turn = gs->room_prev;
    }

    room_set_visited_flag(current_room);
    return CONTINUE_GAME;
}




int main_chateau_gaillard(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    set_silent_mode(GLOBALS.silent_mode);

    if (GLOBALS.debug_mode) {
        set_char_sleep(GLOBALS.debug_normal_sleep);
    } else {
        set_char_sleep(GLOBALS.char_sleep_duration);
    }

    const CharBuffer *player_name = get_player_name();
    GLOBALS.player_name = player_name;

    GameState gs = {};

    initialize();
    reset(&gs, DEBUG_RAND_SEED);
    display_line("Your character attribute stats are:");
    display_char_attributes(gs.stats);
    display_line("");
    display_line("--------------------------------------------------------------------------------");
    display_line("");

    // obj_repr();
    // monsters_names_repr();
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

// main() is defined when running this TU stand-alone and including -DCHATEAU_GAILLARD_MAIN compiler flag.
#ifdef CHATEAU_GAILLARD_MAIN
int main(void) {
    return main_chateau_gaillard();
}
#endif
