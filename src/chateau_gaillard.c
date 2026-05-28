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
    -std=c23 -o chateau_gaillard.out chateau_gaillard.c mersenne_twister.c common/console_utils.c common/string.c

*/
#include "chateau_gaillard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


//// ------------------------------------------------------------
////
///     RANDOM
////    PRNG - Mersenne Twister
////
//// ------------------------------------------------------------

// return random int in range [min_inclusive, max_exclusive)
static int rnd_range(GameState * gs, int min_inclusive, int max_exclusive) {
    return (int)mt_rand_range(&gs->mt_state, min_inclusive, max_exclusive);
}

// return random double in range [0,1)
static double rnd_d(GameState * gs) {
    return mt_random_double(&gs->mt_state);
}


static int  roll_d6(GameState * gs, const int num_dice) {
    int result = 0;
    for (int i = 0; i < num_dice; ++i ) {
        result += rnd_range(gs, 1, 7);
    }

    return result;
}

static CharStats random_hero_stats(GameState * gs) {
    CharStats stats;
    stats.null_stat       = 0;
    stats.strength        = roll_d6(gs,3);
    stats.charisma        = roll_d6(gs,3);
    stats.dexterity       = roll_d6(gs,3);
    stats.intelligence    = roll_d6(gs,3);
    stats.wisdom          = roll_d6(gs,3);
    stats.constitution    = roll_d6(gs,3);
    return stats;
}

static CharStats random_monster_stats(GameState * gs) {
    CharStats stats;
    stats.null_stat       = 0;
    stats.strength        = rnd_range(gs, 3, 18 + 1);
    stats.charisma        = rnd_range(gs, 3, 18 + 1);
    stats.dexterity       = rnd_range(gs, 3, 18 + 1);
    stats.intelligence    = rnd_range(gs, 3, 18 + 1);
    stats.wisdom          = rnd_range(gs, 3, 18 + 1);
    stats.constitution    = rnd_range(gs, 3, 18 + 1);
    return stats;
}



static int count_rooms_visited(const GameState * gs) {
    int result = 0;
    for (int i = 0; i < NUM_ROOMS; ++i ) {
        result += gs->rooms_visited[i];
    }
    return result;
}

static int calc_score(const GameState * gs) {
    int sum_attributes = gs->stats.strength + gs->stats.charisma + gs->stats.dexterity +
        gs->stats.intelligence + gs->stats.wisdom + gs->stats.constitution;
    return 3 * gs->cash +  30 * gs->monsters_killed + 3 * sum_attributes + gs->turns  ;
}

//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------

static void display_score(const GameState * gs) {
    if (GLOBAL_silent_mode) return;

    display("\nSCORE: ");
    printf("%d\n", calc_score(gs));
    const int rooms_visited = count_rooms_visited(gs);
    printf("\nturns: %d, cash: %d, monsters fought: %d, killed: %d, rooms: %d\n",
        gs->turns, gs->cash, gs->monsters_fought, gs->monsters_killed, rooms_visited);
    printf("You completed %3.0f%% of the quest.\n", (double)rooms_visited * 100.0 / (NUM_ROOMS - NUM_DEATH_ROOMS - 1 ) );

}

static void display_conclusion(const GameState * gs) {
    if (GLOBAL_silent_mode) return;

    set_char_sleep(_30ms);  // so final text display is slowed down

    if (gs->completed && !gs->is_dead) {
        display("\nYou have succeeded, ");
        display_line(gs->player_name->buffer);
        display_line("You have escaped the Citadel of Pershu.");
        display_line("\nWell done!");
    } else if (gs->is_dead) {
        display_line("You have died.........");
    }
}

static void display_random_room_text(GameState * gs, const RandomTextArray *rta) {
    if (GLOBAL_silent_mode ) return;
    for (int i=0; i< rta->length; ++i) {
        const RandomText rt = rta->lines[i];
        const double random = mt_random_double(&gs->mt_state); // random double in [0,1)
        if (random < rt.chance_percent) {
            display_line(rt.text);
        } else if (rt.else_text) {
            display_line(rt.else_text);
        }
    }
}


static void display_room_desc(GameState * gs) {
    if (GLOBAL_silent_mode) return;

    display_line("");
    if (!gs->has_torch && ROOM_GRAPH[gs->room][RGINDEX_TREASURE] != 1 ) {
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

static void display_room_monster(GameState * gs) {
    if (GLOBAL_silent_mode) return;

    const int monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if ( monster_index == 0 ) {
        return;
    }
    Room room =  ROOMS[gs->room];
    display_line("\nLOOK OUT!");
    display("There is a ");
    display(room.monster.name);
    display_line(" here!");
}

static void display_room_treasure(const GameState * gs) {
    if (GLOBAL_silent_mode) return;
    const int treasure_index = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    const int rgindex1 = ROOM_GRAPH[gs->room][RGINDEX_TREASURE2];
    const int rgindex2 = ROOM_GRAPH[gs->room][RGINDEX_TREASURE3];

    if (treasure_index > 98 && rgindex1 == 0 && rgindex2 == 0) {
        return; // todo (rob) these are currently magic numbers until we figure out what they do
    }


    if ( (treasure_index > 0 && treasure_index <= 98 ) ||
        (rgindex1 > 0 && rgindex1 <= 98) ||
        (rgindex2 > 0 && rgindex2 <= 98) ) {
        display("\nYou can see ");
    } else {
        return;
    }

    const Room room =ROOMS[gs->room];
    if (treasure_index > 0 && treasure_index <= 98 ) {
        display_line(room.treasure.name);
    }
    if ( rgindex1 > 0 && rgindex1 <  NUM_OBJECTS ) {
        display_line(OBJECTS[rgindex1].name);
    }
    if ( rgindex2 > 0 && rgindex2 < NUM_OBJECTS ) {
        display_line(OBJECTS[rgindex2].name);
    }

}

static void display_room_content(GameState * gs) {
    if (GLOBAL_silent_mode) return;

    display_room_treasure(gs);

    const int treasure_index = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    if (treasure_index > 98 ) {
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

static void display_inventory(const GameState * gs) {
    if (GLOBAL_silent_mode) return;

    bool has_items = false;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i] != 0 ) {
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
    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index ) {
        if (gs->items[bag_index]) {
            printf("%d. ", bag_index);
            display(OBJECTS[gs->items[bag_index]].name);
            display("  ");
            item_count++;
            cash += OBJECTS[gs->items[bag_index]].value;
            if ( ! (item_count % 3) ) {
                display_line("");  // display 3 items per line
            }
        }
    }

    if ( item_count % 3) {
        display_line("");
    }

    if (cash > 0 ) {
        display("Total value - $");
        printf("%d\n", cash);
    }
}



// clear the monster in the current room and its entry in the ROOMS array
static void clear_monster(const GameState * gs) {
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
    ROOMS[gs->room].monster = (Monster){};
}

// Returns true if the user can move from the current room via the direction
bool check_can_move(GameState * gs, int const direction, const bool verbose) {
    if (!strchr(VALID_DIRECTIONS, direction)) {
        if (verbose) vdisplay_line("I don't know how to go %c.", direction);
        return false;
    }

    const int direction_index = calc_direction_index((char)direction);
    if (direction_index == DIRECTION_ERR) {
         if (verbose) vdisplay_line("Bad direction_index, first_letter='%c'", direction);
        return false;
    }

    const int room_index = gs->room;
    const int next_room_index = ROOM_GRAPH[room_index][direction_index];
    if ( next_room_index == 0 ) {
        display_line("");
        display_line(BAD_MOVE_DESC[direction_index]);
        return false;
    }

    if ( next_room_index < 0 || next_room_index >= NUM_ROOMS) {
        vdisplay_line("runtime error: next_room_index is out of bounds: %d, expected range [0, %d]" ,
            next_room_index, NUM_ROOMS - 1);
        return false;
    }

    // check for transition guards, like locks on doors, puzzles solved, equipment carried, etc.
    // i.e., if room_index == ROOM_YELLOW && (dwarf_alive) print("Dwarf stops you"); return false;


    return true;
}

// ML/engine path
static bool move_action(GameState * gs, int const first_letter) {
    if (!check_can_move(gs, first_letter, false)) {
        return false;
    }
    const int direction_index = calc_direction_index(first_letter);
    gs->room = ROOM_GRAPH[gs->room][direction_index];
    return true;

}

// Human user path
// Expect the first letter of the object[] to be in "NSEWUD".
// Return true if the command was successfully processed. If false, the move is not allowed and an error message
// will have been displayed.
static bool cmd_move(GameState * gs, struct ParsedCommand pc) {
    const int first_letter = toupper(pc.object[0]);
    if (!check_can_move(gs, first_letter, true)) {
        return false;
    }
    return perform_action( gs, CMD_MOVE, first_letter, 0, 0);
}

//return false if fight action could not be completed, otherwise return true
bool fight_action( GameState * gs, int weapon, enum StatIndex stat1, enum StatIndex stat2) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        return false;  // nothing to fight
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
    int ferocity_factor =  m.ferocity_factor * 2.0 / weapon;
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
            display(hero_tally > monster_tally ? "you" : m.name);
            display_line(".");
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
        if ( !(hero_tally > 0 && monster_tally > 0 && rnd_d(gs) < .75 )) {
            break;
        }
    }

    if (hero_tally > monster_tally ) {
        display_line("You have slain the beast.");
        gs->monsters_killed++;
    } else {
        display("The ");
        display(m.name);
        display_line(" got the better of you that time.");
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
            gs->stats.constitution =  gs->stats.constitution / 2;
        }
    }

    clear_monster(gs);
    //normalize any negative stats to 0
    for (int i = 0; i < STAT_COUNT; ++i ) {
        if (gs->stats.as_array[i] < 0 ) {
            gs->stats.as_array[i] = 0;
        }
    }
    return true;
}


// return true if the user is carrying this item
static bool clear_user_item(GameState * gs, const int object_index) {
    if ( object_index < 1 || object_index >= NUM_OBJECTS ) {
        return false;
    }

    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index ) {
        if ( gs->items[bag_index] == object_index ) {
            gs->items[bag_index] = 0;
            return true;
        }
    }
    return false;
}

static int user_item_count(const GameState * gs ) {
    int count = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i]) ++count;
    }

    return count;
}

static char const * object_name_for_index(const int object_index) {
    const int i = ( object_index < 0 || object_index > NUM_OBJECTS ) ? 0 : object_index;
    return OBJECTS[i].name;
}

static int object_index_for_name(char const * item_name) {
    if (!item_name) return 0; // null object

    for (int i = 1; i < NUM_OBJECTS; ++i) {
        if (strcmp(OBJECTS[i].name, item_name) == 0 ) {
            return i;
        }
    }
    return 0;
}


// return true if the user is carrying this item
static bool has_item_named(const GameState * gs, char const * item_name) {
    if (!item_name) return false;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i]) {
            if ( strcmp(OBJECTS[gs->items[i]].name, item_name ) == 0 ) {
                return true;
            }
        }
    }
    return false;
}

// return true if the user is carrying this item
static bool has_item(const GameState * gs, const int object_index) {
    if ( object_index < 1 || object_index >= NUM_OBJECTS ) {
        return false;
    }
    for (int bag_index = 1; bag_index < MAX_ITEMS; ++bag_index ) {
        if ( gs->items[bag_index] == object_index ) {
            return true;
        }
    }
    return false;
}

// return true if carrying any items

static bool has_items(const GameState * gs)
{
    for (int bag_index = 1; bag_index < MAX_ITEMS; ++bag_index ) {
        if (! gs->items[bag_index] ) {
            return true;
        }
    }
    return false;
}

/**
 * Shared Validation: Can an item be dropped here?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_drop_item(const GameState *gs, const int object_index, bool verbose) {
    if (!object_index || !has_item(gs, object_index)) {
        if (verbose) display_line("You are not carrying that item.");
        return false;
    }
    if ( ROOM_GRAPH[gs->room][RGINDEX_TREASURE] &&
         ROOM_GRAPH[gs->room][RGINDEX_TREASURE2] &&
         ROOM_GRAPH[gs->room][RGINDEX_TREASURE3]) {
            if (verbose) {
                display_line("This room already holds its maximum number of objects.");
            }
            return false;
        }

    return true;
}

/** Logic Entry Point: ML and Human both end up here */
bool drop_action(GameState *gs, int object_index) {
    if ( !can_drop_item(gs, object_index, false)) return false;

    if ( !clear_user_item( gs, object_index)) {
        return false;
    }

    if ( !ROOM_GRAPH[gs->room][RGINDEX_TREASURE] ) {
        ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = object_index;
    } else if ( !ROOM_GRAPH[gs->room][RGINDEX_TREASURE2] ) {
        ROOM_GRAPH[gs->room][RGINDEX_TREASURE2] = object_index;
    } else if ( !ROOM_GRAPH[gs->room][RGINDEX_TREASURE3] ) {
        ROOM_GRAPH[gs->room][RGINDEX_TREASURE3] = object_index;
    }

    return true;
}

// Entry point for the human user path.
// This displays some information, prompts the user for some choices, and passes those to
// drop_action(), the ML entry point for the drop action.
static bool drop_item(GameState * gs, const int object_index) {
    // Pre-check: If the room is already full, don't even start the loop
    if (!can_drop_item(gs, object_index, true)) return false;

    return perform_action( gs, CMD_DROP, object_index, 0, 0);
}


/**
 * Shared Validation: Can we take the object? Make sure the object_index is valid and exists in the room
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_take_item(const GameState *gs, const int object_index, const bool verbose) {
    if ( object_index < 0 || object_index >= NUM_OBJECTS ) {
        if (verbose) display_line("Unknown object.");
        return false;
    }

    if ( ROOM_GRAPH[gs->room][RGINDEX_TREASURE]  != object_index &&
         ROOM_GRAPH[gs->room][RGINDEX_TREASURE2] != object_index &&
         ROOM_GRAPH[gs->room][RGINDEX_TREASURE3] != object_index) {
        if (verbose) {
            display_line("That object is not here.");
        }
        return false;
    }

    if (user_item_count(gs) >= MAX_ITEMS ) {
        if (verbose) vdisplay_line("You are already carrying your maximum of %d objects.", MAX_ITEMS);
        return false;
    }

    if (object_index == OBJECT_STONE_CHEST || object_index == OBJECT_IRON_CHEST ) {
        if (verbose) display_line("It is far too heavy to lift.");
        return false;
    }

    return true;
}

/** Logic Entry Point: ML and Human both end up here */
bool take_action(GameState *gs, int object_index) {
    if (!can_take_item(gs, object_index, false)) return false;

    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (!gs->items[i]) {
            int room_index = gs->room;
            gs->items[i] = object_index;
            if ( ROOM_GRAPH[room_index][RGINDEX_TREASURE] == object_index ) {
                ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
            } else if ( ROOM_GRAPH[room_index][RGINDEX_TREASURE2] == object_index ) {
                ROOM_GRAPH[room_index][RGINDEX_TREASURE2] = 0;
            } else if ( ROOM_GRAPH[room_index][RGINDEX_TREASURE3] == object_index ) {
                ROOM_GRAPH[room_index][RGINDEX_TREASURE3] = 0;
            }

            return true;
        }
    }
    return false;
}

static bool take_object(GameState * gs, const int object_index) {
    // Pre-check: make sure the object_index is valid and exists in the room
    if (!can_take_item(gs, object_index, true)) return false;

    bool success = perform_action( gs, CMD_TAKE, object_index, 0, 0);

    if (success) {
        vdisplay_line("\nYou now have the object_index:%d, %s", object_index, object_name_for_index(object_index));
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
            result = fight_action(gs, arg1, (enum StatIndex)arg2, (enum StatIndex)arg3);
            break;
        case CMD_MOVE:
            result = move_action(gs, arg1);
            break;
        case CMD_DROP:
            result =  drop_action(gs, arg1);
            break;
        case CMD_TAKE:
            result =  take_action(gs, arg1);
            break;
        default:
            result = false;
            break;
    }

    return result;
}

// Entry point for the human user path. This displays some information, prompts the user for some choices,
// and passes those to perform_action(), the ML entry point for the fight action.
static bool process_fight(GameState * gs) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        display_line("There is nothing to fight.");
        return false;
    }
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == MONSTER_DWARF ) {
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
                display_line("s.");  //pluralization
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
                display_line(".");  //pluralization
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
    if (weapon_count == 0 ) {
        display("You must fight the ");
        display(m.name);
        display_line(" with your bare hands.");
    } else if ( weapon_count == 1 ) {
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
        display(OBJECTS[ weapon_choice ].name);
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
        ( gs->room >= ROOM_TRAPPED &&  gs->room <= ROOM_SPIDER ) ||
        gs->room >= ROOM_GARGOYLE )  {
        if (gs->room != ROOM_END ) gs->is_dead = true;
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
bool adjust_stats(GameState * gs) {
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

void check_dwarf(GameState * gs) {
    // process dwarf
    if (gs->room == ROOM_YELLOW &&
        (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == MONSTER_DWARF) &&
        rnd_d(gs) < .16 ) {
        display_paginated("You hear a whispered voice warning you: 'You must do something about the dwarf.'",
            80);
        }
}


bool str_in_array(char const * str, int len, char const  * array[static len]) {
    size_t str_len = strlen(str);
    char upper[str_len + 1] = {};
    for (int i = 0; i < str_len; ++i) {
        upper[i] = (char)toupper(str[i]);
    }
    upper[str_len] = '\0';
    for (int i = 0; i < len; ++i ) {
        if (strcmp(upper, array[i]) == 0 ) {
            return true;
        }
    }
    return false;
}

struct ParsedCommand  parse_user_command(char const * prompt, char const * err_msg) {
    struct ParsedCommand pc = {};

    for (;;) {
        char line[1024]       = {};
    }
    return pc;
}

struct ParsedCommand  parse_user_command_prev(char const * prompt, char const * err_msg) {

    // todo (rob) at what point is it more efficient to use regexpr here? Keeping in mind we
    // eventually want the grammar to be data driven from an external file

    for (;;) {

        char line[1024]       = {};
        char verb[1024]       = {};
        char verb_upper[1024] = {};
        char object[1024]     = {};

        struct ParsedCommand pc;

        display(prompt);
        if (!fgets(line, sizeof(line), stdin)) {
            continue;
        }

        // Remove newline if present
        line[strcspn(line, "\n")] = 0;

        int num_words = sscanf(line, "%s %s", verb, object);
        if (num_words < 1) {
            continue;
        }
        //defensive measure to ensure these are properly terminated within allocated bounds
        verb[1023]   = '\0';
        object[1023] = '\0';
        const size_t verb_len = strlen(verb);
        for (int i = 0; i < 1024 && verb[i]; ++i) {
            verb_upper[i] = (char)toupper((unsigned char)verb[i]);
        }

        // -----------------------------------------------------------------
        //         One word commands
        // -----------------------------------------------------------------


        // Check for help or quit (single word sufficient)
        if (strcmp(verb_upper, "HELP") == 0) {
            pc.command = CMD_HELP;
            memcpy(pc.verb, verb, 1024);
            pc.object[0] = '\0';
            return pc;
        }
        if (strcmp(verb_upper, "QUIT") == 0) {
            pc.command = CMD_QUIT;
            memcpy(pc.verb, verb, 1024);
            pc.object[0] = '\0';
            return pc;
        }

        // allow navigation with single word or direction character
        if (verb_len == 1 && strchr(VALID_DIRECTIONS, verb_upper[0])) {
            pc.command = CMD_MOVE;
            strncpy(pc.object, verb, sizeof(pc.object) - 1);
            strncpy(pc.verb, "go", strlen("go"));
            return pc;
        }
        const char *direction_verbs[] = {"NORTH", "SOUTH", "EAST", "WEST", "UP", "DOWN"};
        // printf("checking direction verbs: verb_upper:%s\n" ,verb_upper);

        for (int i = 0; i < 6; i++) {
            if (strcmp(verb_upper, direction_verbs[i]) == 0) {
                pc.command = CMD_MOVE;
                strncpy(pc.object, verb, sizeof(pc.object) - 1);
                strncpy(pc.verb, "go", strlen("go"));
                return pc;
            }
        }

        enum Command cmd = CMD_NONE;
        // Fight commands todo (rob) these are one-word commands here because there can only be one monster to fight
        // in a room, but in future versions, there may be more than one monster to fight so the second string is
        // required
        const char *fight_verbs[] = {"STAB", "KILL", "FIGHT", "KICK", "PUNCH", "SLAY", "ATTACK" };
        for (int i = 0; i < 7; i++) {
            if (strcmp(verb_upper, fight_verbs[i]) == 0) {
                pc.command = CMD_FIGHT;
                return pc;
            }
        }

        // -----------------------------------------------------------------
        //         Two word commands
        // -----------------------------------------------------------------

        // Movement commands
        const char *movement_verbs[] = {"GO", "MOVE", "CLIMB", "RUN", "WALK"};
        bool continue_outer = false;
        // printf("checking movement verbs:\n   verb:%s, verb_upper:%s, object:%s \n" ,verb, verb_upper, object);

        for (int i = 0; i < 5; i++) {
            if (strcmp(verb_upper, movement_verbs[i]) == 0) {
                if (!str_in_array(object, 6, direction_verbs)) {
                    // printf("'%s' not in direction_verbs.\n", object);
                    if ( strlen(object) == 0 ) {
                        vdisplay_line("%s where?", verb);
                    } else {
                        vdisplay_line("I don't know how to %s %s", verb, object);
                    }
                    continue_outer = true;
                    break;
                }
                cmd = CMD_MOVE;
                break;
            }
        }
        if (continue_outer) {
            continue;
        }

        // Drop commands
        const char *drop_verbs[] = {"DROP", "PUT", "THROW", "BREAK" };
        for (int i = 0; i < 4; i++) {
            if (strcmp(verb_upper, drop_verbs[i]) == 0) {
                cmd = CMD_DROP;
                break;
            }
        }

        // Take commands
        const char *take_verbs[] = {"TAKE", "GET", "STEAL", "LIFT" };
        for (int i = 0; i < 4; i++) {
            if (strcmp(verb_upper, take_verbs[i]) == 0) {
                cmd = CMD_TAKE;
                break;
            }
        }


        if (cmd) {
            if (num_words == 2) {
                pc.command = cmd;
                strncpy(pc.object, object, sizeof(pc.object) - 1);
                pc.object[sizeof(pc.object) - 1] = '\0';
                return pc;
            } else {
                char const * question;
                switch (cmd) {
                    case CMD_MOVE:
                        question = "where"; break;
                    case CMD_DROP:
                    case CMD_TAKE:
                    case CMD_FIGHT:
                        question = "what"; break;
                    default:
                        question = "how"; break;
                }
                char err[1024];
                snprintf(err, sizeof(err), "%s %s?", verb, question);
                display_line(err);
                continue;
            }
        }

        display_line(err_msg);
    }
}

static bool process_quit(const GameState * gs) {
    display_line("COWARD...QUITTER....TURNCOAT.....");
    // todo (rob) ask for confirmation?
    return END_GAME;
}

static bool main_game_loop(GameState * gs) {
    uint32_t saved_sleep_duration = GLOBAL_char_sleep_duration;
    if ( gs->rooms_visited[gs->room] ) {
        // if we've already seen this room, speed up output display
        set_char_sleep(1'000);  // 1ms
    }

    gs->rooms_visited[gs->room] = true;
    printf("---------------------------------------------------------------------------- %d\n", gs->turns);

    // display_status(gs);
    // display_line("");
    display_room_desc(gs);

    if (check_game_over(gs)){
        set_char_sleep(saved_sleep_duration);
        return END_GAME;
    }

    display_room_content(gs);

    const int monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if (monster_index > MONSTER_DWARF && rnd_d(gs) < .3 ) {
        // forced fight, monster attacks first
        display("The ");
        display(MONSTER_NAMES[monster_index]);
        display_line(" attacks!");
    } else {
        // we just lose points randomly here for some reason.
        if (adjust_stats(gs)){
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
    enum Command cmd = pc.command;

    if ( cmd == CMD_QUIT) {
        set_char_sleep(saved_sleep_duration);
        return process_quit(gs);
    }
    if ( cmd == CMD_HELP) {
        display_paginated("No help for mortals in this game! Although, reading and drinking may help...", 80);
    }

    if ( cmd == CMD_FIGHT ) {
        //specialized code to prompt user and gather options to pass to perform_action()
        process_fight(gs);
    } else if (cmd == CMD_DROP){
        drop_item(gs, object_index_for_name(pc.object));
    } else if (cmd == CMD_TAKE){
        take_object(gs, object_index_for_name(pc.object));
    } else if (cmd == CMD_MOVE){
        cmd_move(gs, pc);
    } else {
        // Now the human call and the ML call use the exact same entry point
        perform_action(gs, cmd, 0,0, 0);
    }

    set_char_sleep(saved_sleep_duration);

    display_line("");

    return CONTINUE_GAME;
}



// called at the start of each new game
void reset(GameState * gs, const uint32_t seed) {
    // reset GameState
    *gs = (GameState){ .seed = seed, .player_name = gs->player_name,  .room = ROOM_START, .has_torch = true, .QU = 1  };

    mt_initialize_state(&gs->mt_state, seed);  // initialize the PRNG

    gs->stats = random_hero_stats(gs);


    //clear all monsters, treasure
    for ( int room_index = 0; room_index < NUM_ROOMS; ++room_index ) {
        // note: if we dynamically modify the edge graph, we'll need to reset those edges here
        ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
        ROOM_GRAPH[room_index][RGINDEX_MONSTER] = 0;
        ROOM_GRAPH[room_index][RGINDEX_TREASURE2] = 0;
        ROOM_GRAPH[room_index][RGINDEX_TREASURE3] = 0;
        ROOMS[room_index].monster =  (Monster){};
        memcpy(&ROOMS[room_index].treasure, & (Object){}, sizeof(Object));
        // ROOMS[room_index].treasure =  (Object){};
    }

    ROOM_GRAPH[ROOM_EERIE][RGINDEX_TREASURE]    = OBJECT_SILVER_KEY;
    ROOM_GRAPH[ROOM_WOODEN][RGINDEX_TREASURE]   = OBJECT_SWORD;
    ROOM_GRAPH[ROOM_L_SHAPED][RGINDEX_TREASURE] = OBJECT_AXE;
    ROOM_GRAPH[ROOM_KITCHEN][RGINDEX_TREASURE]  = 99; // locked door i  99??
    ROOM_GRAPH[ROOM_MIRROR][RGINDEX_TREASURE]   = OBJECT_STONE_CHEST;
    ROOM_GRAPH[ROOM_UNEVEN][RGINDEX_TREASURE]   = 100; // locked door ii  100?
    ROOM_GRAPH[ROOM_TROPHY][RGINDEX_TREASURE]   = OBJECT_IRON_CHEST;
    ROOM_GRAPH[ROOM_TURRET][RGINDEX_TREASURE]   = OBJECT_GOLD_KEY;

    ROOM_GRAPH[ROOM_YELLOW][RGINDEX_MONSTER]    = MONSTER_DWARF;

    // create room Object for the treasures
    for (int room = 1; room < NUM_ROOMS; ++room) {
        int treasure_index = ROOM_GRAPH[room][RGINDEX_TREASURE];
        if ( treasure_index > 0 && treasure_index < NUM_OBJECTS ) {
            ROOMS[room].treasure = OBJECTS[ treasure_index ];
        }
    }



    // allot treasure
    for (int treasure_index = OBJECT_AXE; treasure_index <= OBJECT_DIADEM; ++treasure_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, NUM_ROOMS);
            // todo (rob) this is an inefficient check. Put valid rooms in a list, shuffle the list, choose first N rooms
            if ( ! ( ROOM_GRAPH[rand_room][RGINDEX_TREASURE] ||
                rand_room == ROOM_START ||
                rand_room == ROOM_END ||
                rand_room == ROOM_STONE ||
                rand_room == ROOM_CRAMPED ||
                rand_room == ROOM_TRAPPED ||
                ( rand_room >= ROOM_TRAPPED && rand_room <= ROOM_SPIDER ) ||
                rand_room == ROOM_GARGOYLE ) )
            {
                ROOM_GRAPH[rand_room][RGINDEX_TREASURE] = treasure_index;
                memcpy(&ROOMS[rand_room].treasure, &OBJECTS[treasure_index], sizeof(Object));
                // ROOMS[rand_room].treasure = OBJECTS[treasure_index];
                break;
            }
        }
    }

    // allot monsters
    for (int monster_index= MONSTER_DWARF + 1; monster_index < NUM_MONSTERS; ++monster_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, NUM_ROOMS);
            if ( ! ( ROOM_GRAPH[rand_room][RGINDEX_MONSTER] ||
                     rand_room == ROOM_START ||
                     rand_room == ROOM_END ||
                     rand_room == ROOM_STONE ||
                     rand_room == ROOM_CRAMPED ||
                     rand_room == ROOM_TRAPPED ||
                     ( rand_room >= ROOM_TRAPPED && rand_room <= ROOM_SPIDER ) ||
                     rand_room == ROOM_GARGOYLE ) )
            {
                ROOM_GRAPH[rand_room][RGINDEX_MONSTER]  = monster_index;
                CharStats s = random_monster_stats(gs);
                int ff = s.strength * rnd_range(gs, 1, 6 + 1);
                ROOMS[rand_room].monster =
                    (Monster){
                        .name = MONSTER_NAMES[monster_index],
                        .monster_index = monster_index,
                        .ferocity_factor = ff,
                        .stats = s};
                break;
            }
        }
    }

    // update_perception(gs);
}

void init_rooms(void) {
    // random text for rooms 4,
    // special code for room 5 QU=2, SC=50, room 13 CH=CH-1, room 29 QU=3.5, room 30 SC=10, QU=3.4, room 31 sc=20, QU=3
    // room 32 counts down from 10 to 1 as you die from a spider bite, SC=3, QU=5, room 37 SC=0  QU=3
}

// once time inits. Per-game inits happen in reset()
void initialize() {
    // note: random data is initialized in reset()
    init_rooms();
}

static CharBuffer * get_player_name() {
    cls();
    CharBuffer *cb = get_char_buffer("What is your name, explorer? ");
    display("Hello, Explorer ");
    display(cb->buffer);
    display_line(".");
    display_line("Type '[H]elp' for a list of commands.");
    return cb;
}

constexpr int DEBUG_RAND_SEED = 67;

static void cleanup(GameState * gs);

int main_chateau_gaillard(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    set_silent_mode(false);

    set_char_sleep(0); // todo (rob) this is for debugging so we don't have to wait for text to display


    const CharBuffer * player_name = get_player_name();

    GameState gs = {.player_name = player_name};
    initialize();
    reset(&gs, DEBUG_RAND_SEED );

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

static void cleanup(GameState * gs) {
    destroy_rooms();
    void * free_ptr = (void *)gs->player_name;
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

    for (int i = 1; i < NUM_ROOMS; ++i ) {
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