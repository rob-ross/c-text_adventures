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
    -std=c23 -o chateau_gaillard.out chateau_gaillard.c mersenne_twister.c \
     common/console_utils.c common/string.c parser.c objects.c rooms.c monsters.c

*/
#include "chateau_gaillard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

uint32_t DEBUG_NORMAL_SLEEP = 0;
uint32_t DEBUG_VISITED_SLEEP = 0;
bool DEBUG = true;

static void cleanup(GameState *gs);

//// ------------------------------------------------------------
////
///     RANDOM
////    PRNG - Mersenne Twister
////
//// ------------------------------------------------------------

// return random int in range [min_inclusive, max_exclusive)
static int rnd_range(GameState *gs, int min_inclusive, int max_exclusive) {
    return (int) mt_rand_range(&gs->mt_state, min_inclusive, max_exclusive);
}

// return random double in range [0,1)
static double rnd_d(GameState *gs) {
    return mt_random_double(&gs->mt_state);
}


static int roll_d6(GameState *gs, const int num_dice) {
    int result = 0;
    for (int i = 0; i < num_dice; ++i) {
        result += rnd_range(gs, 1, 7);
    }

    return result;
}

static CharStats random_hero_stats(GameState *gs) {
    CharStats stats;
    stats.null_stat = 0;
    stats.strength = roll_d6(gs, 3);
    stats.charisma = roll_d6(gs, 3);
    stats.dexterity = roll_d6(gs, 3);
    stats.intelligence = roll_d6(gs, 3);
    stats.wisdom = roll_d6(gs, 3);
    stats.constitution = roll_d6(gs, 3);
    return stats;
}

static CharStats random_monster_stats(GameState *gs) {
    CharStats stats;
    stats.null_stat = 0;
    stats.strength = rnd_range(gs, 3, 18 + 1);
    stats.charisma = rnd_range(gs, 3, 18 + 1);
    stats.dexterity = rnd_range(gs, 3, 18 + 1);
    stats.intelligence = rnd_range(gs, 3, 18 + 1);
    stats.wisdom = rnd_range(gs, 3, 18 + 1);
    stats.constitution = rnd_range(gs, 3, 18 + 1);
    return stats;
}


static int count_rooms_visited(const GameState *gs) {
    int result = 0;
    for (int i = 0; i < NUM_ROOMS; ++i) {
        result += gs->rooms_visited[i];
    }
    return result;
}

static int actor_calc_inventory_value(const GameState *gs);
static int calc_score(GameState *gs) {
    int sum_attributes = gs->stats.strength + gs->stats.charisma + gs->stats.dexterity +
                         gs->stats.intelligence + gs->stats.wisdom + gs->stats.constitution;
    int cash = actor_calc_inventory_value(gs);
    gs->cash = cash;
    // printf("score_bonus:%d, cash:%d, mk:%d, sum_attributes:%d, turns:%d, QU:%d\n",
    //     score_bonus, cash, gs->monsters_killed, sum_attributes    ,gs->turns, gs->QU);

    return (int) ((gs->SC + 20 * cash + 47 * gs->monsters_killed + 3 * sum_attributes + gs->turns) / gs->QU);
}

//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------

static void display_score(GameState *gs) {
    if (GLOBAL_silent_mode) return;
    const int rooms_visited = count_rooms_visited(gs);

    vdisplay_line("SCORE: %d", calc_score(gs) );
    vdisplay_line("turns: %d, cash: %d, monsters fought: %d, killed: %d, rooms: %d",
                  gs->turns, gs->cash, gs->monsters_fought, gs->monsters_killed, rooms_visited);
    vdisplay_line("You completed %3.0f%% of the quest.", (double) rooms_visited * 100.0 / (NUM_ROOMS - NUM_DEATH_ROOMS - 1));
}

static void display_conclusion(const GameState *gs) {
    if (GLOBAL_silent_mode) return;

    set_char_sleep(_30ms); // so final text display is slowed down

    if (gs->completed && !gs->is_dead) {
        vdisplay_line("\nYou have succeeded, %s",gs->player_name->buffer);
        display_line("You have escaped the Chateau Gaillard.");
        display_line("\nWell done!");
    } else if (gs->is_dead) {
        display_line("You have died.........");
    }
}

static void display_random_room_text(GameState *gs, const RandomTextArray *rta) {
    if (GLOBAL_silent_mode) return;
    for (int i = 0; i < rta->length; ++i) {
        const RandomText rt = rta->lines[i];
        const double random = mt_random_double(&gs->mt_state); // random double in [0,1)
        if (random < rt.chance_percent) {
            display_line(rt.text);
        } else if (rt.else_text) {
            display_line(rt.else_text);
        }
    }
}


static void display_room_desc(GameState *gs) {
    if (GLOBAL_silent_mode) return;

    display_line("");
    if (!gs->has_torch && ROOM_GRAPH[gs->room][RGINDEX_TREASURE] != 1) {
        display_line("It is too dark to see anything.\n");
    } else {
        Room r = ROOMS[gs->room];
        if (r.preamble) {
            display_random_room_text(gs, r.preamble);
        }

        display_paginated(ROOMS[gs->room].desc, 80);

        if (r.epilog) {
            display_random_room_text(gs, r.epilog);
        }
    }
}

static void display_room_monster(GameState *gs) {
    if (GLOBAL_silent_mode) return;

    const monster_id id = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if (id == 0) {
        return;
    }
    display_line("\nLOOK OUT!");
    display("There is a ");
    display(monsters_name_for_id( id));
    display_line(" here!");
}

static void display_room_treasure(const GameState *gs) {
    if (GLOBAL_silent_mode) return;

    const Room *room = &ROOMS[gs->room];
    if (room_count_of_objects(room) > 0) {
        display("You can see\n");
    } else {
        return;
    }

    for (int i = 0; i < 10; ++i) {
        if (room->objects[i]) {
            // vdisplay_line("(id=%d) %s\n", room->objects[i], obj_name_for_object_id(room->objects[i]));
            display_line(obj_name_for_object_id(room->objects[i]));
        }
    }
}

static void display_room_content(GameState *gs) {
    if (GLOBAL_silent_mode) return;
    display_room_treasure(gs);
    display_room_monster(gs);
}

static void display_char_attributes(const CharStats stats) {
    if (GLOBAL_silent_mode) return;
    display("Strength:  ");
    printf("%2d", stats.strength);
    display("  Charisma:     ");
    printf("%2d\n", stats.charisma);

    display("Dexterity: ");
    printf("%2d", stats.dexterity);
    display("  Intelligence: ");
    printf("%2d\n", stats.intelligence);

    display("Wisdom:    ");
    printf("%2d", stats.wisdom);
    display("  Constitution: ");
    printf("%2d\n", stats.constitution);
}


// clear the monster in the current room and its entry in the ROOMS array
static void clear_monster(const GameState *gs) {
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
    ROOMS[gs->room].monster = 0;
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
        if (verbose) vdisplay_line("I don't know how to go %c.", direction);
        return false;
    }

    const int direction_index = calc_room_graph_direction_index((char) direction);
    if (direction_index == DIRECTION_ERR) {
        if (verbose) vdisplay_line("Bad direction_index, first_letter='%c'", direction);
        return false;
    }

    const int next_room_id = ROOM_GRAPH[room_id][direction_index];
    if (next_room_id == 0) {
        display_line(BAD_MOVE_DESC[direction_index]);
        return false;
    }

    if (next_room_id < 0 || next_room_id >= NUM_ROOMS) {
        if (verbose) {
            vdisplay_line("runtime error: next_room_index is out of bounds: %d, expected range [0, %d]",
                          next_room_id, NUM_ROOMS - 1);
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
            vdisplay_line("%s where?", pc->verb);
        } else {
            vdisplay_line("I don't know how to %s '%s'.", pc->verb, pc->verb_object);
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
    for (int j = 0; j < MAX_ITEMS; ++j) {
        if (gs->items[j] == weapon) {
            missing_weapon = false;
            break;
        }
    }
    if (missing_weapon) {
        const Object *o = obj_find_object(weapon);
        if (!o) {
            vdisplay_line("unknown weapon id=%d", weapon);
            return false;
        }
        vdisplay("You're not carrying a %s.", o->name);
        return false;
    }
    // We add to the hero_tally (8 - weapon)
    // that will give us 7 extra points for using the axe, down to 1 extra point for using a falchion.

    int weapon_strength = weapon <= OBJECT_FALCHION ? 8 - weapon : 8;
    // possible weapons range from 1. axe to 7. falchion. Axe is the best weapon and should add bonus points to hero tally.
    int weapon_bonus = weapon_strength; //here we use the ordinality of the object_ids, but eventually weapons will
    // have their own attributes we will query

    printf("weapon:%d, weapon_strength:%d, weapon_bonus:%d\n",weapon, weapon_strength, weapon_bonus);

    gs->monsters_fought++;
    // gs->must_fight = false;
    const monster_id mid = ROOMS[gs->room].monster;
    const Monster *m = monsters_find_monster(mid);
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


    if (!GLOBAL_silent_mode) {
        if (hero_tally == monster_tally) {
            display_line("You are evenly matched.");
        } else if (hero_tally > monster_tally) {
            display_line("It looks like the odds are in favor of you.");
        } else {
            vdisplay_line("It looks like the odds are in favor of the %s.", monster_name);
        }
        vdisplay_line("The %s - %d", monster_name, monster_tally);
        vdisplay_line("You - %d", hero_tally);
    }

    for (;;) {
        int attack_roll = roll_d6(gs, 1);
        switch (attack_roll) {
            case 0:
                display_line("You struck a splendid blow.");
                monster_tally--;
                break;
            case 1:
                vdisplay_line("The %s strikes out.", monster_name);
                hero_tally--;
                gs->stats.strength--;
                gs->stats.charisma--;
                break;
            case 2:
                vdisplay_line("You draw the %s's blood!", monster_name);
                monster_tally--;
                break;
            case 3:
                display_line("You are wounded!!");
                hero_tally -= rnd_range(gs, 1, 4);
                gs->stats.dexterity--;
                break;
            case 4:
                vdisplay_line("The %s is tiring.", monster_name);
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
                vdisplay_line("You wound the %s", monster_name);
                monster_tally--;
                break;
        }
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

    clear_monster(gs);
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

// Removes the object id from the user's list of items.
// Returns true if the user was carrying this item, or false if the item was not present.
// after this method completes, the object id will no longer be in the agent's item list.
static bool actor_remove_object(GameState *gs, const object_id id) {
    if (id < 1 || id >= NUM_OBJECTS) {
        return false;
    }

    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index) {
        if (gs->items[bag_index] == id) {
            gs->items[bag_index] = 0;
            obj_clear_location(id);
            return true;
        }
    }
    return false;
}


// Returns the object id of the first object in the actor's items[],
// or ROOM_OBJ_NOT_FOUND if there are no items
static int actor_first_object_id(const GameState *gs) {
    for (int i = 0; i < MAX_ITEMS; ++i) {
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

    for (int i = 0; i < MAX_ITEMS; ++i) {
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



static int actor_count_of_objects(const GameState *gs) {
    int count = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i]) ++count;
    }
    return count;
}

// return true if the user is carrying this item
static bool actor_has_item_named(const GameState *gs, char const *item_name) {
    if (!item_name) return false;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        const object_id id = gs->items[i];
        if ( id ) {
            const Object *o = obj_find_object(id);
            if ( o && strcmp(o->name, item_name) == 0) {
                return true;
            }
        }
    }
    return false;
}

// return true if the user is carrying this item
static bool actor_has_item(const GameState *gs, const object_id id) {
    if (id < 1 || id >= NUM_OBJECTS) {
        return false;
    }
    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index) {
        if (gs->items[bag_index] == id) {
            return true;
        }
    }
    return false;
}

// return true if carrying any items

static bool actor_has_any_items(const GameState *gs) {
    for (int bag_index = 1; bag_index < MAX_ITEMS; ++bag_index) {
        if (!gs->items[bag_index]) {
            return true;
        }
    }
    return false;
}

static int actor_calc_inventory_value(const GameState *gs) {
    int cash = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i] != 0) {
            const Object *o = obj_find_object(gs->items[i]);
            cash += o->value;
        }
    }
    return cash;
}

static void actor_display_inventory(const GameState *gs) {
    if (GLOBAL_silent_mode) return;

    bool has_items = false;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i] != 0) {
            has_items = true;
            break;
        }
    }

    if (!has_items) {
        display_line("You are not carrying anything.");
        return;
    }

    display_line("You are carrying:");
    int item_count = 0;
    int cash = 0;
    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index) {
        object_id id = gs->items[bag_index];
        if (id) {
            const Object *o = obj_find_object(id);
            vdisplay("%d. %s  ", bag_index + 1, o->name);
            item_count++;
            cash += o->value;
            if (!(item_count % 3)) {
                display_line(""); // display 3 items per line
            }
        }
    }

    if (item_count % 3) {
        display_line("");
    }

    if (cash > 0) {
        display("Total value - $");
        printf("%d\n", cash);
    }
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
    const Room *r = &ROOMS[gs->room];
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
        if (verbose) vdisplay_line("You are not carrying that item. object_id:%d", id);
        return false;
    }
    if (room_is_full(&ROOMS[gs->room])) {
        if (verbose) {
            display_line("This room already holds its maximum number of objects.");
        }
        return false;
    }
    return true;
}

/** Logic Entry Point: ML and Human both end up here */
bool acton_drop(GameState *gs, int object_id) {
    if (!can_drop_item(gs, object_id, false)) return false;

    if (!actor_remove_object(gs, object_id)) {
        return false;
    }
    return room_add_object(&ROOMS[gs->room], object_id) == ROOM_SUCCESS;
}

// Entry point for the human user path.
// This finds the item in the VO, and passes those to
// drop_action(), the ML entry point for the read action.
static bool cmd_drop(GameState *gs, const struct ParsedCommand *pc) {
    object_id id = 0;
    if (!pc->has_verb_object) {
        if (actor_count_of_objects(gs) == 1) {
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

    // Pre-check: If the room is already full, don't even start the loop
    if (!can_drop_item(gs, id, true)) return false;

    return perform_action(gs, CMD_DROP, id, 0, 0);
}


/**
 * Shared Validation: Can the item be be read?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_read_item(const GameState *gs, const object_id id, const bool verbose) {
    if (!id || !actor_has_item(gs, id)) {
        if (verbose) vdisplay_line("You are not carrying that item. object_id:%d", id);
        return false;
    }
    const Object *o = obj_find_object(id);
    if ( !o ) {
        if (verbose) vdisplay_line("unknown object id=%d", id);
        return false;
    }
    if ( !o->is_readable_bit) {
        if (verbose) vdisplay_line("You can't read the %s", o->name);
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
        vdisplay_line("You don't have a %s", pc->verb_object);
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
    if (id < 1 || id >= NUM_OBJECTS) {
        if (verbose) display_line("What's that?");
        return false;
    }

    if (room_index_for_object(&ROOMS[gs->room], id) == ROOM_ERR_OBJECT_NOT_FOUND) {
        if (verbose) {
            vdisplay_line("object_id:%d", id);
            display_line("That object is not here.");
        }
        return false;
    }

    if (actor_count_of_objects(gs) >= MAX_ITEMS) {
        if (verbose) vdisplay_line("You are already carrying your maximum of %d objects.", MAX_ITEMS);
        return false;
    }

    if (id == OBJECT_STONE_CHEST || id == OBJECT_IRON_CHEST) {
        if (verbose) display_line("It is far too heavy to lift.");
        return false;
    }

    return true;
}

/** Logic Entry Point: ML and Human both end up here */
bool action_take(GameState *gs, const object_id id) {
    if (!can_take_item(gs, id, false)) return false;

    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (!gs->items[i]) {
            room_id room_id = gs->room;
            gs->items[i] = id;
            room_remove_object(&ROOMS[room_id], id);
            obj_relocate_object(id, -1);
            // todo (rob) need to define location ids, player vs room. -1 means "player" but is a kludge
            // in zork, everything had a unique string id, ie, "player", "room-1", "axe", so there was one
            // global namespace for ids.
            return true;
        }
    }
    return false;
}

static bool cmd_take(GameState *gs, const ParsedCommand *pc) {
    object_id id = 0;
    const Room *current_room = &ROOMS[gs->room];
    if (!pc->has_verb_object) {
        if (room_count_of_objects(current_room) == 1) {
            // if only one object, we'll take whatever is in the room
            id = room_first_object_id(current_room);
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
        id = obj_id_for_partial_name(pc->verb_object);
    }

    // Pre-check: make sure the object_index is valid and exists in the room
    if (!can_take_item(gs, id, true)) return false;

    bool success = perform_action(gs, CMD_TAKE, id, 0, 0);

    if (success) {
        // vdisplay_line("You now have the object_index:%d, %s", id, obj_name_for_object_id(id));
        vdisplay_line("%s taken.", obj_name_for_object_id(id));
    }


    return success;
}

static void set_state_death_by_dwarf(GameState *gs) {
    gs->QU = 3;
    gs->is_dead = true;
    gs->game_over = true;
}

static bool can_pay(const GameState *gs, const monster_id unused, const bool verbose) {
    const Room *r = &ROOMS[gs->room];
    const monster_id mid = r->monster;

    if (mid != MONSTER_DWARF) {
        return false;
    }
    return true;
}

/** Logic Entry Point: ML and Human both end up here */
bool action_pay(GameState *gs, const monster_id unused ) {
    if (!can_pay(gs, 0, false)) return false;

    // the model action on success will be to
    // 1. Remove the dwarf from the room.
    // 2. Remove the amulet from the player's bag, set location to 0

    // on failure,
    // 1. 50% chance that the dwarf steals the first item in the player's bag.
    // 2. If the bag is empty, or if the first random check is the other 50%, the dwarf kills you and the game ends
    if (actor_has_item(gs, OBJECT_AMULET)) {
        display_line("Lucky for you that you had it!");
        clear_monster(gs);
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
        vdisplay_line(" the %s", o->name);
        clear_monster(gs);
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
    const Room *current_room = &ROOMS[gs->room];
    // case 1 no monster
    if (current_room->monster == 0) {
        if (pc->has_verb_object) {
            vdisplay_line("There is no %s here to %s.", pc->verb_object, pc->verb);
        } else {
            vdisplay_line("There is nothing here to %s.", pc->verb);
        }
        return false;
    }

    if (pc->has_verb_object ) {
        // player is referencing a monster by name
        const bool is_present = monsters_monster_is_in_room(pc->verb_object, current_room );
        if (!is_present) {
            vdisplay_line("There is no %s here.", pc->verb_object);
            return false;
        }
    }
    return true;
}


static bool cmd_pay(GameState *gs, const ParsedCommand *pc) {
    const Room *current_room = &ROOMS[gs->room];
    if (! verify_monster_choice(gs, pc) ) return false;

    // case 2, monster is present
    const Monster *m = monsters_find_monster(current_room->monster);
    // player is paying a monster by name
    if ( current_room->monster != MONSTER_DWARF ) {
        vdisplay_line("You cannot %s the %s", pc->verb, m->name);
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
        if (verbose) vdisplay_line("You are not carrying that item. object_id:%d", id);
        return false;
    }
    const Object *o = obj_find_object(id);
    if ( !o ) {
        if (verbose) vdisplay_line("unknown object id=%d", id);
        return false;
    }
    if ( !o->is_drinkable_bit) {
        if (verbose) vdisplay_line("You can't drink the %s", o->name);
        return false;
    }
    return true;
}

static bool action_drink( GameState *gs, object_id id) {
    if (!can_drink_item(gs, id, false)) return false;
    // currently there is only one potion in the game
    if (id != OBJECT_HEALING_POTION) return false;

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
        vdisplay_line("%s what?", pc->verb);
        return false;
    }
    // try to read this object by name
    const object_id id = actor_object_id_for_partial_name(gs, pc->verb_object);
    if ( id == OBJ_NOT_FOUND) {
        vdisplay_line("You don't have a %s to %s", pc->verb_object, pc->verb);
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
    if (room < 1 || room >= NUM_ROOMS) {
        if (verbose) vdisplay_line("Room id out of bounds: %d", room);
        return false;
    }
    if (key != OBJECT_SILVER_KEY && key != OBJECT_GOLD_KEY) {
        if (verbose) vdisplay_line("Invalid key id: %d", key);
    }
    if (! actor_has_item(gs, key)) {
        const char *obj_name = obj_name_for_object_id(key);
        if (verbose) vdisplay_line("You don't have the %s.", obj_name);
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
    gs->rooms_visited[gs->room] = true;

    bool result = false;

    switch (cmd) {
        case CMD_FIGHT:
            result = action_fight(gs, arg1, (enum StatIndex) arg2, (enum StatIndex) arg3);
            break;
        case CMD_MOVE:
            result = action_move(gs, arg1);
            break;
        case CMD_DROP:
            result = acton_drop(gs, arg1);
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
    const monster_id mid = ROOMS[gs->room].monster;
    Monster *m = monsters_find_monster(mid);
    const char * monster_name = monsters_name_for_id(mid);
    display_line("--------------------------------------");
    vdisplay_line("Your opponent is a %s.", monster_name);
    int ferocity_factor = m->ferocity_factor;

    vdisplay_line("The %s's danger level is %d", monster_name, ferocity_factor);

    // see what weapons the user has. Object ids 1 (axe) to 7 (falchion)
    int T[MAX_ITEMS] = {};
    for (int j = 0; j < MAX_ITEMS; ++j) {
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
                vdisplay_line("Your dagger is useful against %ss.", monster_name);
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
                vdisplay_line("Swinging your morning star may inflict heavy wounds on the %s.", monster_name);
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
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (T[i]) {
            ++weapon_count;
            last_weapon = T[i];
        }
    }
    int weapon_choice = 0;
    if (weapon_count == 0) {
        vdisplay_line("You must fight the %s with your bare hands.", monster_name);
    } else if (weapon_count == 1) {
        weapon_choice = last_weapon;
        const Object *o = obj_find_object(weapon_choice);
        const char *weapon_name = o ? o->name : "unknown weapon";
        vdisplay_line("You must fight with your %s.", weapon_name);
    } else {
        display_line("choose your weapon: ");
        for (int j = 0; j < MAX_ITEMS; ++j) {
            if (T[j]) {
                const object_id id = T[j];
                const Object *o = obj_find_object(id);
                if (!o) continue;
                vdisplay_line("%d - %s", (j+1), o->name);
            }
        }
        int choice = 0;
        for (;;) {
            choice = get_int("Enter the number to choose: ", 1, MAX_ITEMS);
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
            vdisplay_line("Right, so you choose to fight with the  %s.", weapon_name);
        }

    }

    vdisplay_line("The %s has the following attributes:", monster_name);
    display_char_attributes(m->stats);
    display_line("Your attributes are:");
    display_char_attributes(gs->stats);
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
    return perform_action(gs, CMD_FIGHT, weapon_choice, first_skill, second_skill);
}

static bool cmd_quit( GameState *gs) {
    gs->QU = 4;
    gs->game_over = true;
    gs->ended_by_quitting = true;
    // todo (rob) ask for confirmation?
    return END_GAME;
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
            if (!GLOBAL_silent_mode) {
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
bool adjust_stats(GameState *gs) {
    // we just lose points randomly here for some reason.
    if (gs->stats.strength < 0) {
        gs->stats.strength = 0;
    } else {
        if (rnd_d(gs) < .16) --gs->stats.strength;
    }
    if (gs->stats.charisma < 0) {
        gs->stats.charisma = 0;
    } else {
        if (rnd_d(gs) < .16) --gs->stats.charisma;
    }
    if (gs->stats.dexterity < 0) {
        gs->stats.dexterity = 0;
    } else {
        if (rnd_d(gs) < .16) --gs->stats.dexterity;
    }
    if (gs->stats.intelligence < 0) {
        gs->stats.intelligence = 0;
    } else {
        if (rnd_d(gs) < .16) --gs->stats.intelligence;
    }
    if (gs->stats.wisdom < 0) {
        gs->stats.wisdom = 0;
    } else {
        if (rnd_d(gs) < .16) --gs->stats.wisdom;
    }
    if (gs->stats.constitution < 0) {
        gs->stats.constitution = 0;
    } else {
        if (rnd_d(gs) < .16) --gs->stats.constitution;
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


bool str_in_array(char const *str, int len, char const *array[static len]) {
    size_t str_len = strlen(str);
    char upper[str_len + 1] = {};
    for (int i = 0; i < str_len; ++i) {
        upper[i] = (char) toupper(str[i]);
    }
    upper[str_len] = '\0';
    for (int i = 0; i < len; ++i) {
        if (strcmp(upper, array[i]) == 0) {
            return true;
        }
    }
    return false;
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

static bool main_game_loop(GameState *gs) {
    uint32_t saved_sleep_duration = GLOBAL_char_sleep_duration;
    const room_id room_id = gs->room;
    const Room *current_room = room_find_room(room_id);
    if (current_room->is_visited_bit) {
        // if we've already seen this room, speed up output display
        if (DEBUG) {
            set_char_sleep(DEBUG_VISITED_SLEEP);
        } else {
            set_char_sleep(1'000); // 1ms
        }
    }

    gs->rooms_visited[room_id] = true;
    room_set_visited_flag(current_room);
    char room_buffer[81] = "--------------------------------------------------------------------------------";
    const size_t room_name_len = strlen(current_room->name);
    for (int i = 0; i < room_name_len; ++i) {
        room_buffer[i] = current_room->name[i];
    }
    room_buffer[room_name_len ] = ' ';
    size_t required_len = snprintf(nullptr, 0, " %d", gs->turns);
    // Start at 80 - required_len and provide space for the null terminator (required_len + 1)
    snprintf(&room_buffer[80 - required_len], required_len + 1, " %d", gs->turns);

    // printf("---------------------------------------------------------------------------- %d\n", gs->turns);
    display_line(room_buffer);

    // display_status(gs);
    // display_line("");

    if (gs->room != gs->room_last_turn) {
        // only display room desc once when first entering room. Reduces screen clutter and scrolling.
        // user can always type "look" to re-display room desc.
        display_room_desc(gs);
    }

    if (check_game_over(gs)) {
        set_char_sleep(saved_sleep_duration);
        return END_GAME;
    }

    display_room_content(gs);

    const int monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if (monster_index > MONSTER_DWARF && rnd_d(gs) < .3) {
        // forced fight, monster attacks first
        display("The ");
        display(MONSTER_NAMES[monster_index]);
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

    if (room_id == ROOM_START && gs->room_prev == 0 ) {
        // first room, we display initial inventory. Afterward, the user can view them with explicit "inv" command
        actor_display_inventory(gs);
    }

    // process user input
    flush_input();
    const ParsedCommand pc = parse_user_command("\n>", "I don't know how to do that.");

    // display("You chose ");
    // printf("%d %s\n",pc.command, pc.object);
    const enum Command cmd = pc.verb_command;


    // -----------------------------------------------------------------
    //      Player Presentation Only
    // -----------------------------------------------------------------

    if (cmd == CMD_QUIT) {
        set_char_sleep(saved_sleep_duration);
        return cmd_quit(gs);
    }
    if (cmd == CMD_HELP) {
        display_paginated("No help for mortals in this game! Although, reading and drinking may help...", 80);
    }
    if (cmd == CMD_LOOK) {
        display_room_desc(gs);
    }
    if (cmd == CMD_INV) {
        actor_display_inventory(gs);
    }
    if (cmd == CMD_STATS) {
        display_char_attributes(gs->stats);
    }
    if (cmd == CMD_SCORE) {
        display_score(gs);
    }
    if (cmd == CMD_GOD ) {
        cmd_god_mode(gs, &pc);
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

    display_line("");
    if (room_id == gs->room) {
        // if room at end of turn is same as start of turn, update this so we
        gs->room_last_turn = room_id;
    } else {
        gs->room_last_turn = gs->room_prev;
    }

    return CONTINUE_GAME;
}



int sum_character_stats(const CharStats *s) {
    int total = 0;
    for (int i = 1; i < STAT_COUNT; ++i) {
        total += s->as_array[i];
    }
    return total;
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
    *gs = (GameState){.seed = seed, .player_name = gs->player_name, .room = ROOM_START, .has_torch = true, .QU = 1};

    mt_initialize_state(&gs->mt_state, seed); // initialize the PRNG

    gs->stats = random_hero_stats(gs);


    //clear all monsters, treasure
    for (int room_index = 0; room_index < NUM_ROOMS; ++room_index) {
        // note: if we dynamically modify the edge graph, we'll need to reset those edges here
        ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
        ROOM_GRAPH[room_index][RGINDEX_MONSTER] = 0;
        ROOM_GRAPH[room_index][RGINDEX_REQUIRED_KEY] = 0;
        ROOM_GRAPH[room_index][RGINDEX_TREASURE3] = 0;
        ROOMS[room_index].monster = 0;
    }
    monsters_clear_all();

    ROOM_GRAPH[ROOM_MAGICIAN][RGINDEX_TREASURE]    = OBJECT_SILVER_KEY;
    ROOM_GRAPH[ROOM_WOODEN][RGINDEX_TREASURE]      = OBJECT_SWORD;
    ROOM_GRAPH[ROOM_DUNGEON][RGINDEX_TREASURE]     = OBJECT_AXE;
    ROOM_GRAPH[ROOM_CHARISMA_REDUCE][RGINDEX_TREASURE]      = OBJECT_STONE_CHEST;
    ROOM_GRAPH[ROOM_TROPHY][RGINDEX_TREASURE]      = OBJECT_IRON_CHEST;
    ROOM_GRAPH[ROOM_SECRET_ROOM][RGINDEX_TREASURE] = OBJECT_AMULET;
    ROOM_GRAPH[ROOM_TURRET][RGINDEX_TREASURE]      = OBJECT_GOLD_KEY;

    ROOM_GRAPH[ROOM_KITCHEN][RGINDEX_REQUIRED_KEY]     = OBJECT_SILVER_KEY; // locked door i, requires silver key
    ROOM_GRAPH[ROOM_UNEVEN][RGINDEX_REQUIRED_KEY]      = OBJECT_GOLD_KEY; // locked door ii, requires golden key


    // -----------------------------------------------------------------
    //      NEW OBJECT ALLOCATION METHOD
    // -----------------------------------------------------------------

    room_add_object(&ROOMS[ROOM_MAGICIAN], OBJECT_SILVER_KEY);
    room_add_object(&ROOMS[ROOM_WOODEN], OBJECT_SWORD);
    room_add_object(&ROOMS[ROOM_DUNGEON], OBJECT_AXE);
    room_add_object(&ROOMS[ROOM_CHARISMA_REDUCE], OBJECT_STONE_CHEST);
    room_add_object(&ROOMS[ROOM_TROPHY], OBJECT_IRON_CHEST);
    room_add_object(&ROOMS[ROOM_SECRET_ROOM], OBJECT_AMULET);
    room_add_object(&ROOMS[ROOM_TURRET], OBJECT_GOLD_KEY);


    // allot random treasure
    for (int treasure_index = OBJECT_DAGGER; treasure_index <= OBJECT_DIADEM; ++treasure_index) {
        // if object has already been assigned to a room, skip this iteration.
        if (obj_find_object(treasure_index)->location != 0 ) {
            continue;
        }
        for (;;) {
            int rand_room = rnd_range(gs, 1, NUM_ROOMS);
            // todo (rob) this is an inefficient check. Put valid rooms in a list, shuffle the list, choose first N rooms
            if (!(ROOM_GRAPH[rand_room][RGINDEX_TREASURE] ||
                  rand_room == ROOM_START ||
                  rand_room == ROOM_END ||
                  rand_room == ROOM_STONE ||
                  rand_room == ROOM_CRAMPED ||
                  rand_room == ROOM_TRAPPED ||
                  (rand_room >= ROOM_TRAPPED && rand_room <= ROOM_SPIDER) ||
                  rand_room == ROOM_GARGOYLE)) {
                ROOM_GRAPH[rand_room][RGINDEX_TREASURE] = treasure_index;
                // new way to manage objects
                room_add_object(&ROOMS[rand_room], treasure_index);
                break;
            }
        }
    }


    ROOM_GRAPH[ROOM_YELLOW][RGINDEX_MONSTER] = MONSTER_DWARF;
    ROOMS[ROOM_YELLOW].monster = MONSTER_DWARF;
    CharStats stats = random_monster_stats(gs);
    int ff = sum_character_stats(&stats);
    monsters_update_monster( &(Monster) {
                                    .name = MONSTER_NAMES[MONSTER_DWARF],
                                    .id = MONSTER_DWARF,
                                    .ferocity_factor = ff,
                                    .stats = random_monster_stats(gs),
    });

    // allot random monsters
    for (int monster_index = MONSTER_DWARF + 1; monster_index < NUM_MONSTERS; ++monster_index) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, NUM_ROOMS);
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
                ROOMS[rand_room].monster = monster_index;
                monsters_update_monster( &(Monster) {
                            .name = MONSTER_NAMES[monster_index],
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

RandomTextArray * create_rta(int length) {
    const size_t mem_size = sizeof(RandomTextArray) + sizeof(RandomText) * length;
    RandomTextArray * result = calloc(1, mem_size);
    result->length = length;
    return result;
}

static void init_rooms(void) {
    // random text for rooms 4
    ROOMS[4].epilog = create_rta(1);
    ROOMS[4].epilog->lines[0] = (RandomText){
        .chance_percent = .5,
        .text="A mouse scampers across the floor.",
        .else_text = "A bat flits across the ceiling."};


    // special code for
    // room 32 counts down from 10 to 1 as you die from a spider bite
    // todo (rob) more console display features like a countdown
}


static struct ObjectData {
    Object data[20];
} get_object_data(void) {
    return (struct ObjectData){
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
    // note: random data is initialized in reset()
    parser_init();
    obj_init(20, get_object_data().data);
    init_rooms();
}

static CharBuffer *get_player_name() {
    cls();
    CharBuffer *cb = get_char_buffer("What is your name, explorer? ");
    display("Hello, Explorer ");
    display(cb->buffer);
    display_line(".");
    display_line("Type '[H]elp' for a list of commands.");
    return cb;
}

constexpr int DEBUG_RAND_SEED = 67;



int main_chateau_gaillard(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    set_silent_mode(false);

    if (DEBUG) {
        set_char_sleep(DEBUG_NORMAL_SLEEP);
    } else {
        set_char_sleep(10'000);
    }


    const CharBuffer *player_name = get_player_name();

    GameState gs = {.player_name = player_name};
    initialize();
    reset(&gs, DEBUG_RAND_SEED);
    display_line("Your attributes are:");
    display_char_attributes(gs.stats);
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


//// ------------------------------------------------------------
////
////    CLEANUP
////
//// ------------------------------------------------------------

static void destroy_rooms() {
    for (int room_index = 0; room_index < NUM_ROOMS; ++room_index) {
        free(ROOMS[room_index].preamble);
        free(ROOMS[room_index].epilog);
    }
}

static void cleanup(GameState *gs) {
    destroy_rooms();
    void *free_ptr = (void *) gs->player_name;
    gs->player_name = nullptr;
    free(free_ptr);
}


//// ------------------------------------------------------------
////
////    DEBUGGING
////
//// ------------------------------------------------------------

void display_all_room_desc() {
    const uint32_t saved_sleep = GLOBAL_char_sleep_duration;
    set_char_sleep(0);

    for (int i = 1; i < NUM_ROOMS; ++i) {
        display_line("");
        display_line(ROOMS[i].name);
        display_line("-------------------------------------------------------------------");
        display_paginated(ROOMS[i].desc, 80);
    }

    // restore previous sleep duration
    set_char_sleep(saved_sleep);
}

// main() is defined when running this TU stand-alone and including -DCHATEAU_GAILLARD_MAIN compiler flag.
#ifdef CHATEAU_GAILLARD_MAIN
int main(void) {
    return main_chateau_gaillard();
}
#endif
