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
    stats.strength        = 3  * rnd_range(gs, 0, 6) + 1;
    stats.charisma        = 3  * rnd_range(gs, 0, 6) + 1;
    stats.dexterity       = 3  * rnd_range(gs, 0, 6) + 1;
    stats.intelligence    = 3  * rnd_range(gs, 0, 6) + 1;
    stats.wisdom          = 3  * rnd_range(gs, 0, 6) + 1;
    stats.constitution    = 3  * rnd_range(gs, 0, 6) + 1;
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

    return END_GAME;
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
                ROOMS[rand_room].monster =
                    (Monster){
                        .name = MONSTER_NAMES[monster_index],
                        .monster_index = monster_index,
                        .stats = random_monster_stats(gs)};
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

    set_char_sleep(0);
    for (int i  = 0; i < NUM_ROOMS; ++i ) {
        display_line(ROOMS[i].name);
        display_line("-----------------------------------------------------------------------------");
        display_paginated(ROOMS[i].desc, 80);
        display_line("");
    }


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

// main() is defined when running this TU stand-alone and including -DCHATEAU_GAILLARD_MAIN compiler flag.
#ifdef CHATEAU_GAILLARD_MAIN
int main(void) {
    return main_chateau_gaillard();
}
#endif