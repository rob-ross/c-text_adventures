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
     common/console_utils.c common/string.c parser.c room_objects.c rooms.c monsters.c

*/
#include "chateau_gaillard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

static int calc_score(const GameState *gs) {
    int sum_attributes = gs->stats.strength + gs->stats.charisma + gs->stats.dexterity +
                         gs->stats.intelligence + gs->stats.wisdom + gs->stats.constitution;
    return 3 * gs->cash + 30 * gs->monsters_killed + 3 * sum_attributes + gs->turns;
}

//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------

static void display_score(const GameState *gs) {
    if (GLOBAL_silent_mode) return;

    display("\nSCORE: ");
    printf("%d\n", calc_score(gs));
    const int rooms_visited = count_rooms_visited(gs);
    printf("\nturns: %d, cash: %d, monsters fought: %d, killed: %d, rooms: %d\n",
           gs->turns, gs->cash, gs->monsters_fought, gs->monsters_killed, rooms_visited);
    printf("You completed %3.0f%% of the quest.\n", (double) rooms_visited * 100.0 / (NUM_ROOMS - NUM_DEATH_ROOMS - 1));
}

static void display_conclusion(const GameState *gs) {
    if (GLOBAL_silent_mode) return;

    set_char_sleep(_30ms); // so final text display is slowed down

    if (gs->completed && !gs->is_dead) {
        display("\nYou have succeeded, ");
        display_line(gs->player_name->buffer);
        display_line("You have escaped the Citadel of Pershu.");
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

    const int monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if (monster_index == 0) {
        return;
    }
    Room room = ROOMS[gs->room];
    display_line("\nLOOK OUT!");
    display("There is a ");
    display(room.monster.name);
    display_line(" here!");
}

static void display_room_treasure(const GameState *gs) {
    if (GLOBAL_silent_mode) return;
    const int treasure_index = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    const int rgindex1 = ROOM_GRAPH[gs->room][RGINDEX_TREASURE2];
    const int rgindex2 = ROOM_GRAPH[gs->room][RGINDEX_TREASURE3];

    if (treasure_index > 98 && rgindex1 == 0 && rgindex2 == 0) {
        return; // todo (rob) these are currently magic numbers until we figure out what they do
    }

    const Room *room = &ROOMS[gs->room];
    if (room_count_of_objects(room) > 0) {
        display("\nYou can see\n");
    } else {
        return;
    }

    for (int i = 0; i < 10; ++i) {
        if (room->objects[i]) {
            vdisplay_line("(id=%d) %s\n", room->objects[i], room_objects_name_for_object_id(room->objects[i]));
            // display_line(room_objects_name_for_object_id(room->objects[i]));
        }
    }
}

static void display_room_content(GameState *gs) {
    if (GLOBAL_silent_mode) return;

    display_room_treasure(gs);

    const int treasure_index = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    if (treasure_index > 98) {
        display_paginated("One of the doors is locked, preventing you from exploring further.", 80);
    }

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

static void display_inventory(const GameState *gs) {
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

    display_line("\nYou are carrying:");
    int item_count = 0;
    int cash = 0;
    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index) {
        if (gs->items[bag_index]) {
            printf("%d. ", bag_index + 1);
            display(OBJECTS[gs->items[bag_index]].name);
            display("  ");
            item_count++;
            cash += OBJECTS[gs->items[bag_index]].value;
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


// clear the monster in the current room and its entry in the ROOMS array
static void clear_monster(const GameState *gs) {
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
    ROOMS[gs->room].monster = (Monster){};
}

// Returns true if the user can move from the current room via the direction
bool check_can_move(GameState *gs, int const direction, const bool verbose) {
    if (!strchr(VALID_DIRECTIONS, direction)) {
        if (verbose) vdisplay_line("I don't know how to go %c.", direction);
        return false;
    }

    const int direction_index = calc_room_graph_direction_index((char) direction);
    if (direction_index == DIRECTION_ERR) {
        if (verbose) vdisplay_line("Bad direction_index, first_letter='%c'", direction);
        return false;
    }

    const int room_index = gs->room;
    const int next_room_index = ROOM_GRAPH[room_index][direction_index];
    if (next_room_index == 0) {
        display_line("");
        display_line(BAD_MOVE_DESC[direction_index]);
        return false;
    }

    if (next_room_index < 0 || next_room_index >= NUM_ROOMS) {
        vdisplay_line("runtime error: next_room_index is out of bounds: %d, expected range [0, %d]",
                      next_room_index, NUM_ROOMS - 1);
        return false;
    }

    // check for transition guards, like locks on doors, puzzles solved, equipment carried, etc.
    // i.e., if room_index == ROOM_YELLOW && (dwarf_alive) print("Dwarf stops you"); return false;


    return true;
}

// ML/engine path
static bool move_action(GameState *gs, int const first_letter) {
    if (!check_can_move(gs, first_letter, false)) {
        return false;
    }
    const int direction_index = calc_room_graph_direction_index(first_letter);
    gs->room = ROOM_GRAPH[gs->room][direction_index];
    return true;
}

// Human user path
// Expect the first letter of the object[] to be in "NSEWUD".
// Return true if the command was successfully processed. If false, the move is not allowed and an error message
// will have been displayed.
static bool cmd_move(GameState *gs, const struct ParsedCommand *pc) {
    const int first_letter = toupper(pc->verb_object[0]);
    if (!check_can_move(gs, first_letter, true)) {
        return false;
    }
    return perform_action(gs, CMD_MOVE, first_letter, 0, 0);
}

//return false if fight action could not be completed, otherwise return true
bool fight_action(GameState *gs, int weapon, enum StatIndex stat1, enum StatIndex stat2) {
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
        display("You're not carrying a ");
        display(OBJECTS[weapon].name);
        display_line(".");
        return false;
    }

    gs->monsters_fought++;
    // gs->must_fight = false;

    Monster m = ROOMS[gs->room].monster;
    int monster_tally = 0;
    int hero_tally = 0;
    int ferocity_factor = m.ferocity_factor * 2.0 / weapon;
    hero_tally += gs->stats.as_array[stat1];
    hero_tally += gs->stats.as_array[stat2];
    monster_tally += m.stats.as_array[stat1];
    monster_tally += m.stats.as_array[stat2];

    // todo (rob) ferocity_factor is computed but never used here. We should add to the hero_tally 8 - weapon -
    // that will give us 7 extra points for using the axe, down to 1 extra point for using a falchion.

    if (!GLOBAL_silent_mode) {
        if (hero_tally == monster_tally) {
            display_line("You are evenly matched.");
        } else if (hero_tally > monster_tally) {
            display_line("It looks like the odds are in favor of you.");
        } else {
            display("It looks like the odds are in favor of the ");
            display(m.name);
            display_line(".");
        }
        display("The ");
        display(m.name);
        display(" - ");
        printf("%d\n", monster_tally);
        display("You - ");
        printf("%d\n", hero_tally);
    }

    for (;;) {
        int attack_roll = roll_d6(gs, 1);
        switch (attack_roll) {
            case 0:
                display_line("You struck a splendid blow.");
                monster_tally--;
                break;
            case 1:
                display("The ");
                display(m.name);
                display_line(" strikes out.");
                hero_tally--;
                gs->stats.strength--;
                gs->stats.charisma--;
                break;
            case 2:
                display("You draw the ");
                display(m.name);
                display_line("'s blood!");
                monster_tally--;
                break;
            case 3:
                display_line("You are wounded!!");
                hero_tally -= rnd_range(gs, 1, 4);
                gs->stats.dexterity--;
                break;
            case 4:
                display("The ");
                display(m.name);
                display_line(" is tiring.");
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
                display("You wound the ");
                display(m.name);
                display_line("");
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
        display(m.name);
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

// Removes the object id from the user's list of items.
// Returns true if the user was carrying this item, or false if the item was not present.
// after this method completes, the object id will no longer be in the agent's item list.
static bool actor_clear_object_id(GameState *gs, const object_id id) {
    if (id < 1 || id >= NUM_OBJECTS) {
        return false;
    }

    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index) {
        if (gs->items[bag_index] == id) {
            gs->items[bag_index] = 0;
            return true;
        }
    }
    return false;
}


// Returns the object id of the first object in the actor's items[],
// or ACTOR_OBJECT_NOT_FOUND if there are no items
static int actor_first_object_id(const GameState *gs) {
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i] != 0) {
            return gs->items[i];
        }
    }
    return ROOM_OBJECT_NOT_FOUND;
}

// return the object id for the given item_name. If a partial item_name is passed, performs a "starts with"
// search to match. But this will return the first object in the store that starts with the argument string,
// which may or may not be what you are looking for.
static int actor_object_id_for_partial_name(const GameState *gs, char const item_name[static 1]) {
    if (!item_name) return ROOM_OBJECT_NULL_OBJECT_NAME;

    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i] != 0) {
            const Object *o = find_object(gs->items[i]);
            if (strncmp(item_name, o->name, strlen(item_name)) == 0) {
                printf("actor_object_id_for_partial_name: item_name: %s, items[%d].name:%s, strlen:%zd\n",
                       item_name, i, o->name, strlen(item_name));
                return o->id;
            }
        }
    }
    return ROOM_OBJECT_NOT_FOUND;
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
        if (gs->items[i]) {
            if (strcmp(OBJECTS[gs->items[i]].name, item_name) == 0) {
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

/**
 * Shared Validation: Can something be opened here?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_open(const GameState *gs, const bool verbose) {
    // todo (rob) simple implementation for now. This app has 2 chests,only the chest
    // in Room 40 has something in it.
    const Room *r = &ROOMS[gs->room];
    if (room_contains_object(r, OBJECT_STONE_CHEST) ||
        room_contains_object(r, OBJECT_STONE_CHEST)) {
        return true;
    }
    if (verbose) display_line("There is nothing here to open.");
    return false;
}

bool action_open(GameState *gs, object_id id) {
    if (!can_open(gs, false)) {
        return false;
    }
    return room_objects_set_open_flag(id);
}

static bool cmd_open(GameState *gs, const struct ParsedCommand *pc) {
    if (!can_open(gs, true)) {
        return false;
    }
    const Room *r = &ROOMS[gs->room];
    const Object *o;
    if (room_contains_object(r, OBJECT_IRON_CHEST)) {
        o = find_object(OBJECT_IRON_CHEST);
    } else {
        o = find_object(OBJECT_STONE_CHEST);
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
bool drop_action(GameState *gs, int object_id) {
    if (!can_drop_item(gs, object_id, false)) return false;

    if (!actor_clear_object_id(gs, object_id)) {
        return false;
    }
    return room_add_object(&ROOMS[gs->room], object_id) == ROOM_SUCCESS;
}

// Entry point for the human user path.
// This displays some information, prompts the user for some choices, and passes those to
// drop_action(), the ML entry point for the drop action.
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
        // try to take drop this object by name
        id = actor_object_id_for_partial_name(gs, pc->verb_object);
    }

    // Pre-check: If the room is already full, don't even start the loop
    if (!can_drop_item(gs, id, true)) return false;

    return perform_action(gs, CMD_DROP, id, 0, 0);
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
bool take_action(GameState *gs, const object_id id) {
    if (!can_take_item(gs, id, false)) return false;

    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (!gs->items[i]) {
            int room_index = gs->room;
            gs->items[i] = id;
            room_remove_object(&ROOMS[room_index], id);
            room_objects_relocate_object(id, -1);
            return true;
        }
    }
    return false;
}

static bool cmd_take(GameState *gs, const struct ParsedCommand *pc) {
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
        id = room_objects_id_for_partial_name(pc->verb_object);
    }

    // Pre-check: make sure the object_index is valid and exists in the room
    if (!can_take_item(gs, id, true)) return false;

    bool success = perform_action(gs, CMD_TAKE, id, 0, 0);

    if (success) {
        vdisplay_line("\nYou now have the object_index:%d, %s", id, room_objects_name_for_object_id(id));
    }

    return success;
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
            result = fight_action(gs, arg1, (enum StatIndex) arg2, (enum StatIndex) arg3);
            break;
        case CMD_MOVE:
            result = move_action(gs, arg1);
            break;
        case CMD_DROP:
            result = drop_action(gs, arg1);
            break;
        case CMD_TAKE:
            result = take_action(gs, arg1);
            break;
        case CMD_OPEN:
            result = action_open(gs, arg1);
            break;
        default:
            result = false;
            break;
    }

    return result;
}

// Entry point for the human user path. This displays some information, prompts the user for some choices,
// and passes those to perform_action(), the ML entry point for the fight action.
static bool process_fight(GameState *gs) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        display_line("There is nothing to fight.");
        return false;
    }
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == MONSTER_DWARF) {
        display_line("The dwarf refuses to fight and his magic protects him.");
        return false;
    }
    Monster m = ROOMS[gs->room].monster;
    display_line("--------------------------------------");
    display("Your opponent is a ");
    display(m.name);
    display_line(".");
    int ferocity_factor = m.ferocity_factor;

    display("The ");
    display(m.name);
    display("'s danger level is ");
    printf("%d.\n", ferocity_factor);

    // see what weapons the user has. Object indices 1 (axe) to 7 (falchion)
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
                display("Your dagger is useful against ");
                display(m.name);
                display_line("s."); //pluralization
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
                display("Swinging your morning star may inflict heavy wounds on the ");
                display(m.name);
                display_line("."); //pluralization
                T[j] = OBJECT_MORNING_STAR;
                break;
            case OBJECT_FALCHION:
                display_line("A falchion is a useful weapon.");
                T[j] = OBJECT_FALCHION;
                break;
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
        display("You must fight the ");
        display(m.name);
        display_line(" with your bare hands.");
    } else if (weapon_count == 1) {
        display("You must fight with your ");
        weapon_choice = last_weapon;
        display(OBJECTS[weapon_choice].name);
        display_line(".");
    } else {
        display_line("choose your weapon: ");
        for (int j = 0; j < MAX_ITEMS; ++j) {
            if (T[j]) {
                printf("%d - ", j + 1);
                display_line(OBJECTS[T[j]].name);
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
        display("Right, so you choose to fight with the ");
        display(OBJECTS[weapon_choice].name);
        display_line(".");
    }

    display("\nThe ");
    display(m.name);
    display_line(" has the following attributes:");
    display_char_attributes(m.stats);
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


/**
 * Death and Win condition check
 * RETURNS: true if the game is over (win or loss).
 * The caller should check gs->is_dead or gs->completed to see the outcome.
 */
bool check_game_over(GameState *gs) {
    if (gs->completed) return true;

    if (gs->room == ROOM_END ||
        gs->room == ROOM_STONE ||
        (gs->room >= ROOM_TRAPPED && gs->room <= ROOM_SPIDER) ||
        gs->room >= ROOM_GARGOYLE) {
        if (gs->room != ROOM_END) gs->is_dead = true;
        gs->completed = true;
        return true;
    }

    for (int i = STAT_STRENGTH; i < STAT_COUNT; ++i) {
        if (gs->stats.as_array[i] <= 0) {
            if (!GLOBAL_silent_mode) {
                display_char_attributes(gs->stats);
                display_line("\nYou are exhausted, so this adventure must end.");
                gs->QU = 2;
            }
            gs->is_dead = true;
            gs->completed = true;
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

void check_dwarf(GameState *gs) {
    // process dwarf
    if (gs->room == ROOM_YELLOW &&
        (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == MONSTER_DWARF) &&
        rnd_d(gs) < .16) {
        display_paginated("You hear a whispered voice warning you: 'You must do something about the dwarf.'",
                          80);
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

struct ParsedCommand parse_user_command(char const *prompt, char const *err_msg) {
    constexpr size_t LINE_BUFFER_SIZE = 1024;
    struct ParsedCommand pc = {};

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
        const enum Command ocmd = pc.verb_object_command;
        const size_t vo_len = strlen(pc.verb_object);

        printf("parse_user_command():    vc:%d, voc:%d   verb:'%s', verb_obj:'%s', has_vobj?:%d\n",
               pc.verb_command, pc.verb_object_command, pc.verb, pc.verb_object, pc.has_verb_object);

        if (cmd < 0) {
            display_line(err_msg);
            continue;
        }

        if (cmd == CMD_MOVE && (ocmd < CMD_NORTH || ocmd > CMD_DOWN)) {
            if (vo_len == 0) {
                vdisplay_line("%s where?", pc.verb);
            } else {
                vdisplay_line("I don't know how to %s '%s'.", pc.verb, pc.verb_object);
            }
            continue;
        }

        // if ( vo_len == 0 && (cmd == CMD_FIGHT || cmd == CMD_TAKE || cmd == CMD_DROP )) {
        //     vdisplay_line("%s what?", pc.verb);
        //     continue;
        // }

        break;
    }


    return pc;
}

static bool process_quit(const GameState *gs) {
    display_line("COWARD...QUITTER....TURNCOAT.....");
    // todo (rob) ask for confirmation?
    return END_GAME;
}

static bool main_game_loop(GameState *gs) {
    uint32_t saved_sleep_duration = GLOBAL_char_sleep_duration;
    if (gs->rooms_visited[gs->room]) {
        // if we've already seen this room, speed up output display
        set_char_sleep(1'000); // 1ms
    }

    gs->rooms_visited[gs->room] = true;
    printf("---------------------------------------------------------------------------- %d\n", gs->turns);

    // display_status(gs);
    // display_line("");
    display_room_desc(gs);

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

    check_dwarf(gs);

    display_line("\nYour attributes are:");
    display_char_attributes(gs->stats);

    display_inventory(gs);

    // process user input
    flush_input();
    struct ParsedCommand pc = parse_user_command("\nWhat do you want to do? ", "I don't know how to do that.");

    // display("You chose ");
    // printf("%d %s\n",pc.command, pc.object);
    enum Command cmd = pc.verb_command;

    if (cmd == CMD_QUIT) {
        set_char_sleep(saved_sleep_duration);
        return process_quit(gs);
    }
    if (cmd == CMD_HELP) {
        display_paginated("No help for mortals in this game! Although, reading and drinking may help...", 80);
    }

    if (cmd == CMD_FIGHT) {
        //specialized code to prompt user and gather options to pass to perform_action()
        process_fight(gs);
    } else if (cmd == CMD_DROP) {
        cmd_drop(gs, &pc);
    } else if (cmd == CMD_TAKE) {
        cmd_take(gs, &pc);
    } else if (cmd == CMD_MOVE) {
        cmd_move(gs, &pc);
    } else if (cmd == CMD_OPEN) {
        cmd_open(gs, &pc);
    } else {
        // Now the human call and the ML call use the exact same entry point
        perform_action(gs, cmd, 0, 0, 0);
    }

    set_char_sleep(saved_sleep_duration);

    display_line("");

    return CONTINUE_GAME;
}


// called at the start of each new game
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
        ROOM_GRAPH[room_index][RGINDEX_TREASURE2] = 0;
        ROOM_GRAPH[room_index][RGINDEX_TREASURE3] = 0;
        ROOMS[room_index].monster = (Monster){};
        // memcpy(&ROOMS[room_index].treasure, &(Object){}, sizeof(Object));
        ROOMS[room_index].treasure =  (Object){};
    }

    ROOM_GRAPH[ROOM_EERIE][RGINDEX_TREASURE] = OBJECT_SILVER_KEY;
    ROOM_GRAPH[ROOM_WOODEN][RGINDEX_TREASURE] = OBJECT_SWORD;
    ROOM_GRAPH[ROOM_L_SHAPED][RGINDEX_TREASURE] = OBJECT_AXE;
    ROOM_GRAPH[ROOM_KITCHEN][RGINDEX_TREASURE] = 99; // locked door i  99??
    ROOM_GRAPH[ROOM_MIRROR][RGINDEX_TREASURE] = OBJECT_STONE_CHEST;
    ROOM_GRAPH[ROOM_UNEVEN][RGINDEX_TREASURE] = 100; // locked door ii  100?
    ROOM_GRAPH[ROOM_TROPHY][RGINDEX_TREASURE] = OBJECT_IRON_CHEST;
    ROOM_GRAPH[ROOM_TURRET][RGINDEX_TREASURE] = OBJECT_GOLD_KEY;


    // create room Object for the treasures
    for (int room = 1; room < NUM_ROOMS; ++room) {
        int treasure_index = ROOM_GRAPH[room][RGINDEX_TREASURE];
        if (treasure_index > 0 && treasure_index < NUM_OBJECTS) {
            ROOMS[room].treasure = OBJECTS[treasure_index];
        }
    }

    // -----------------------------------------------------------------
    //      NEW OBJECT ALLOCATION METHOD
    // -----------------------------------------------------------------

    room_add_object(&ROOMS[ROOM_EERIE], OBJECT_SILVER_KEY);
    room_add_object(&ROOMS[ROOM_WOODEN], OBJECT_SWORD);
    room_add_object(&ROOMS[ROOM_L_SHAPED], OBJECT_AXE);
    // add_object_to_room(&ROOMS[ROOM_KITCHEN], OBJECT_SILVER_KEY);
    room_add_object(&ROOMS[ROOM_MIRROR], OBJECT_STONE_CHEST);
    // add_object_to_room(&ROOMS[ROOM_UNEVEN], OBJECT_SILVER_KEY);
    room_add_object(&ROOMS[ROOM_TROPHY], OBJECT_IRON_CHEST);
    room_add_object(&ROOMS[ROOM_TURRET], OBJECT_GOLD_KEY);


    // allot random treasure
    for (int treasure_index = OBJECT_DAGGER; treasure_index <= OBJECT_DIADEM; ++treasure_index) {
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
                memcpy(&ROOMS[rand_room].treasure, &OBJECTS[treasure_index], sizeof(Object));

                room_add_object(&ROOMS[rand_room], treasure_index);
                break;
            }
        }
    }


    ROOM_GRAPH[ROOM_YELLOW][RGINDEX_MONSTER] = MONSTER_DWARF;
    ROOMS[ROOM_YELLOW].monster  = (Monster) {
                                    .name = MONSTER_NAMES[MONSTER_DWARF],
                                    .monster_index = MONSTER_DWARF,
                                    .ferocity_factor = 20,
                                    .stats = random_monster_stats(gs),
                                 };

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
                CharStats s = random_monster_stats(gs);
                int ff = s.strength * rnd_range(gs, 1, 6 + 1);
                ROOMS[rand_room].monster =
                        (Monster){
                            .name = MONSTER_NAMES[monster_index],
                            .monster_index = monster_index,
                            .ferocity_factor = ff,
                            .stats = s
                        };
                break;
            }
        }
    }

    // update_perception(gs);
}

static void init_rooms(void) {
    // random text for rooms 4,
    // special code for room 5 QU=2, SC=50, room 13 CH=CH-1, room 29 QU=3.5, room 30 SC=10, QU=3.4, room 31 sc=20, QU=3
    // room 32 counts down from 10 to 1 as you die from a spider bite, SC=3, QU=5, room 37 SC=0  QU=3
}


static struct ObjectData {
    Object data[20];
} get_object_data(void) {
    return (struct ObjectData){
        .data = {
            {.id = 1, .name = "axe"},
            {.id = 2, .name = "sword"},
            {.id = 3, .name = "dagger"},
            {.id = 4, .name = "mace"},
            {.id = 5, .name = "quarterstaff"},
            {.id = 6, .name = "morning star"},
            {.id = 7, .name = "falchion"},
            {.id = 8, .name = "crystal ball", .value = 99},
            {.id = 9, .name = "amulet", .value = 247},
            {.id = 10, .name = "ebony ring", .value = 166},
            {.id = 11, .name = "gems", .value = 462},
            {.id = 12, .name = "mystic scroll", .value = 195},
            {.id = 13, .name = "healing potion", .value = 231},
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
    room_objects_init(20, get_object_data().data);
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

    set_char_sleep(0); // todo (rob) this is for debugging so we don't have to wait for text to display


    const CharBuffer *player_name = get_player_name();

    GameState gs = {.player_name = player_name};
    initialize();
    reset(&gs, DEBUG_RAND_SEED);

    room_objects_repr();
    monsters_repr();
    room_rooms_repr();

    bool continue_loop;
    do {
        continue_loop = main_game_loop(&gs);
    } while (continue_loop);


    display_conclusion(&gs);
    display_score(&gs);
    cleanup(&gs);
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
