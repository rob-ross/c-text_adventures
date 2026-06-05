// adventure_shared.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/04 22:33:55 PDT

//
// Created by Rob Ross on 6/4/26.
//

#ifndef TEXT_ADVENTURES_ADVENTURE_SHARED_H
#define TEXT_ADVENTURES_ADVENTURE_SHARED_H
#include <stdint.h>

#include "attribute_stats.h"
#include "mersenne_twister.h"
#include "monsters.h"
#include "objects.h"
#include "common/string.h"

//// ------------------------------------------------------------
////
////    GLOBALS
////
//// ------------------------------------------------------------


struct GlobalState {
    const char * player_name;
    bool         silent_mode;
    uint32_t     char_sleep_duration;
    uint32_t     char_sleep_visited_duration;
    uint32_t     debug_normal_sleep;
    uint32_t     debug_visited_sleep;
    bool         debug_mode;
};

extern int ROOM_GRAPH[][RGINDEX_COUNT];
extern struct GlobalState GLOBALS;
extern const int MAX_ITEMS; // max number of items that can be carried

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
    room_id room_prev; // room user was in before this one
    room_id room_last_turn; // updates every turn, if user in same room as last turn, will be same as `room`

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
    bool completed; // true if reached final room
    bool game_over;
    bool is_dead;
    bool ended_by_quitting;

    bool must_fight; // true if user previously tried to retreat from monster and failed

    object_id  items[ITEM_COUNT];  // first 9 items of Treasure have a slot here with the same index

    struct ObservationSpace {
        // what the player can currently "see" in the environment that is not part of the game state model
        bool     monster_is_visible;
        bool     treasure_is_visible;
        bool     must_fight; // Explicitly tell ML that movement/retreat is blocked
        Monster  current_monster;
        Object   current_treasure;
        uint32_t legal_actions_mask; // Bitmask where each bit corresponds to VALID_COMMANDS
    } perception;

    double QU;  // end-of-game flag? Quit flag, used in final scoring
    int SC;  // score bonus, depending on how game ends.
    int BOX; // chest flag?

} GameState;

// extern int ROOM_GRAPH[][RGINDEX_COUNT];
// extern struct GlobalState GLOBALS;
// extern const int MAX_ITEMS; // max number of items that can be carried

int  actor_calc_inventory_value(const GameState *gs);
void actor_clamp_stats(GameState *gs, int min, int max);
int  actor_count_of_objects(const GameState *gs);
void actor_display_inventory(const GameState * gs);
bool actor_has_any_items(const GameState * gs);
bool actor_has_item(const GameState *gs, object_id id);
bool actor_has_item_named(const GameState *gs, char const *item_name);

void clear_monster(const GameState *gs);

void display_char_attributes( CharStats stats );
void display_game_state(const GameState *gs);
void display_random_room_text(GameState * gs, const RandomTextArray *rta);
void display_room_content(GameState * gs);
void display_room_desc(GameState * gs);
void display_room_monster(GameState * gs);
void display_room_treasure(const GameState * gs);

int rnd_range(GameState * gs, int min_inclusive, int max_exclusive);
double rnd_d(GameState * gs);
int roll_d6(GameState * gs, int num_dice);
CharStats random_hero_stats( GameState * gs );
CharStats random_monster_stats( GameState * gs );
Object generate_treasure( GameState * gs, object_id id, int min_value, int max_value);

void room_graph_entry_repr(room_id id);

int sum_character_stats(const CharStats *s);

CharBuffer *get_player_name();



#endif //TEXT_ADVENTURES_ADVENTURE_SHARED_H
