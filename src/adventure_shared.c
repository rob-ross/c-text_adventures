// adventure_shared.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/04 22:33:55 PDT

//
// Created by Rob Ross on 6/4/26.
//

#include "adventure_shared.h"

#include <stdio.h>
#include <string.h>

#include "attribute_stats.h"
#include "common/console_utils.h"

int actor_calc_inventory_value(const GameState *gs) {
    int cash = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i] != 0) {
            const Object *o = obj_find_object(gs->items[i]);
            cash += o->value;
        }
    }
    return cash;
}

int actor_count_of_objects(const GameState *gs) {
    int count = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (gs->items[i]) ++count;
    }
    return count;
}

// clamp all stats to be withing min, max
void actor_clamp_stats(GameState *gs, int min, int max) {
    // clamp stats to min 0
    for (int i = STAT_STRENGTH; i < STAT_COUNT; ++i) {
        int value = gs->stats.as_array[i];
        if (value < min)      gs->stats.as_array[i] = min;
        else if (value > max) gs->stats.as_array[i] = max;
    }
}

void actor_display_inventory(const GameState * gs) {
    if (GLOBALS.silent_mode) return;
    if (!actor_has_any_items(gs)) {
        display_line("You are carrying nothing.");
        return;
    }

    display_line("You are carrying:");
    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index ) {
        if (gs->items[bag_index]) {
            vdisplay(" %s\n", obj_name_for_id(gs->items[bag_index]));
        }
    }
}

// return true if carrying any items
bool actor_has_any_items(const GameState * gs) {
    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index ) {
        if ( gs->items[bag_index] != 0) {
            // printf("actor_has_any_items: i=%d, item_id=%d\n",bag_index, gs->items[bag_index]);
            return true;
        }
    }
    return false;
}

// return true if the user is carrying this item
bool actor_has_item(const GameState *gs, const object_id id) {
    if (id < 1 || id >= obj_num_objects()) {
        return false;
    }
    for (int bag_index = 0; bag_index < MAX_ITEMS; ++bag_index) {
        if (gs->items[bag_index] == id) {
            return true;
        }
    }
    return false;
}

// return true if the user is carrying this item
bool actor_has_item_named(const GameState *gs, char const *item_name) {
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

// clear the monster in the current room and its entry in the ROOMS array
void clear_monster(const GameState *gs) {
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
    room_clear_monster(gs->room);
}


void display_char_attributes(const CharStats stats) {
    if (GLOBALS.silent_mode) return;
    vdisplay_line("Strength:  %2d  Charisma:     %2d",
        stats.strength, stats.charisma);

    vdisplay_line("Dexterity: %2d  Intelligence: %2d",
        stats.dexterity, stats.intelligence );

    vdisplay_line( "Wisdom:    %2d  Constitution: %2d",
        stats.wisdom, stats.constitution);
}

void display_game_state(const GameState *gs) {
    printf("\n(GameState){ player_name=%s, room=%d, turns=%d, cash=%d, killed=%d, fought=%d, magic=%d, "
           "has_torch=%d, is_dead=%d, completed=%d, must_fight=%d }\n",
        gs->player_name->buffer, gs->room,  gs->turns, gs->cash, gs->monsters_killed,gs->monsters_fought, gs->magic,
        gs->has_torch, gs->is_dead, gs->completed, gs->must_fight);
    struct ObservationSpace os = gs->perception;
    printf("(ObservationSpace){ .monster_is_visible=%d, .treasure_is_visible=%d, .must_fight=%d, .current_monster.id=%d, .current_treasure.id=%d }\n",
        os.monster_is_visible, os.treasure_is_visible, os.must_fight, os.current_monster.id, os.current_treasure.id);
    room_repr(room_find_room(gs->room));
    room_graph_entry_repr(gs->room);
    display_char_attributes(gs->stats);
    actor_display_inventory(gs);
    printf("\n");
    printf("Rooms visited:\n");

    const int num_rooms = room_num_rooms();
    for (int room=0; room < num_rooms; ++room ) {
        if (room_find_room(room)->is_visited_bit) {
            printf("%d, ", room);

        }
    }
    printf("\n");
}

void display_random_room_text(GameState * gs, const RandomTextArray *rta) {
    if (GLOBALS.silent_mode ) return;
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

void display_room_content(GameState * gs) {
    if (GLOBALS.silent_mode) return;
    display_room_treasure(gs);
    display_room_monster(gs);
}

void display_room_desc(GameState * gs) {
    if (GLOBALS.silent_mode) return;

    if (!gs->has_torch && ROOM_GRAPH[gs->room][RGINDEX_TREASURE] != 1 ) {
        display_line("It is too dark to see anything.");
    } else {
        const Room *r = room_find_room(gs->room);
        display_line(r->name);
        if (r->preamble) {
            display_random_room_text(gs, r->preamble);
        }

        display_paginated(r->desc, 80);

        if (r->epilog) {
            display_random_room_text(gs, r->epilog);
        }
    }
}

void display_room_monster(GameState * gs) {
    if (GLOBALS.silent_mode) return;

    const monster_id id = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if ( id == 0 ) {
        return;
    }
    double chance = rnd_d(gs);
    display_line("");
    if (gs->has_torch ) {
        const Room *room =  room_find_room(gs->room);
        char const *m_name = (room == nullptr) ? "null" : monsters_name_for_id(room->monster);
        if ( chance < .33) {
            vdisplay_line("You come face to face with a %s.", m_name);
        } else if ( chance < .66){
            vdisplay_line("The room contains a %s.", m_name);
        } else {
            vdisplay_line("LOOK OUT! There is a %s here!", m_name);
        }
    } else {
        display_line("You feel a dangerous presence!");
    }
}

void display_room_treasure(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    const int treasure_index = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    if ( treasure_index == 0 || (!gs->has_torch && treasure_index != ITEM_TORCH )) {
        return;
    }
    const Room *room = room_find_room(gs->room);

    if (room_count_of_objects(room) == 0) return;

    display("\nYou can see ");
    for (int i = 0; i < 10; ++i) {
        if (room->objects[i]) {
            const Object *o = obj_find_object(room->objects[i]);
            if ( o->value == 0 ) display_line(o->name);
            else vdisplay_line( "%s worth $%d", o->name, o->value);
        }
    }
}


//// ------------------------------------------------------------
////
///     RANDOM
////    PRNG - Mersenne Twister
////
//// ------------------------------------------------------------

// return random int in range [min_inclusive, max_exclusive)
int rnd_range(GameState * gs, int min_inclusive, int max_exclusive) {
    return (int)mt_rand_range(&gs->mt_state, min_inclusive, max_exclusive);
}

// return random double in range [0,1)
double rnd_d(GameState * gs) {
    return mt_random_double(&gs->mt_state);
}

int roll_d6(GameState * gs, const int num_dice) {
    int result = 0;
    for (int i = 0; i < num_dice; ++i ) {
        result += rnd_range(gs, 1, 7);
    }

    return result;
}

CharStats random_hero_stats( GameState * gs) {
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

CharStats random_monster_stats( GameState * gs) {
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

Object generate_treasure( GameState * gs, object_id id, int min_value, int max_value) {
    char const * name = obj_name_for_id(id);
    return (Object){
        .name  = name,
        .id    = id,
        .value = rnd_range( gs, min_value, max_value + 1 )
    };
}





void room_graph_entry_repr(room_id id) {
    printf("ROOM_GRAPH[%d][%d]{ ", id, RGINDEX_COUNT);
    for (int i = 0; i < RGINDEX_COUNT; ++i) {
        const char *label = room_rgindex_label_short(i);
        printf("%.1s:%d, ", label, ROOM_GRAPH[id][i] );
    }
    printf("}\n");
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
////    INPUT
////
//// ------------------------------------------------------------


CharBuffer *get_player_name() {
    cls();
    CharBuffer *cb = get_char_buffer("What is your name, explorer? ");
    display("Hello, Explorer ");
    display(cb->buffer);
    display_line(".");
    display_line("Type '[H]elp' for a list of commands.");
    return cb;
}

