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
    const int rgindex1 = ROOM_GRAPH[gs->room][RGINDEX_1];
    const int rgindex2 = ROOM_GRAPH[gs->room][RGINDEX_2];

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

//return false if fight action could not be completed, otherwise return true
bool fight_action(GameState * gs, int weapon, enum StatIndex stat1, enum StatIndex stat2) {
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


bool perform_action(GameState *gs, char action, int arg1, int arg2, int arg3) {
    return true;
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
    display_line(m.name);
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
    return perform_action(gs, 'F', weapon_choice, first_skill, second_skill);
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

static bool main_game_loop(GameState * gs) {
    uint32_t saved_sleep_duration = GLOBAL_char_sleep_duration;
    if ( gs->rooms_visited[gs->room] ) {
        // if we've already seen this room, speed up output display
        set_char_sleep(1'000);  // 1ms
    }

    gs->rooms_visited[gs->room] = true;
    printf("---------------------------------------------------------------------- %d\n", gs->turns);

    // display_status(gs);
    display_line("");
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
        // KW = 1: GOSUB 1400: REM FIGHT ROUTINE
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
    char cmd = get_command_char("\nWhat do you want to do? ", VALID_COMMANDS, nullptr);
    display("You chose ");
    printf("'%c'\n",cmd);


    set_char_sleep(saved_sleep_duration);

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
        ROOM_GRAPH[room_index][RGINDEX_1] = 0;
        ROOM_GRAPH[room_index][RGINDEX_2] = 0;
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
    const int saved_sleep = GLOBAL_char_sleep_duration;
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