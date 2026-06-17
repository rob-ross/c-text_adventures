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


StringAssets global_string_assets = {};

int actor_calc_inventory_value(const GameState *gs) {
    int cash = 0;
    const int items_len = gs->items_len;
    for (int i = 0; i < items_len; ++i) {
        if (gs->items[i] != 0) {
            const Object *o = obj_find_object(gs->items[i]);
            cash += o->value;
        }
    }
    return cash;
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

int actor_count_of_objects(const GameState *gs) {
    return gs->items_len;
}

void actor_display_inventory(const GameState * gs, bool show_item_index, bool show_item_value ) {
    if (GLOBALS.silent_mode) return;
    if (!actor_has_any_items(gs)) {
        display_line("You are carrying nothing.");
        return;
    }

    display_line("You are carrying:");
    const int items_len = gs->items_len;
    for (int bag_index = 0; bag_index < items_len; ++bag_index ) {
        if (gs->items[bag_index]) {
            char const * fmt;
            const Object *o = obj_find_object(gs->items[bag_index]);
            if (show_item_index && show_item_value && o->value != 0) {
                fmt = " % 3d. %s worth $%d";
                display_linef(fmt, bag_index + 1, o->name, o->value );
            } else if ( !show_item_index && !show_item_value) {
                fmt = " %s";
                display_linef(fmt, o->name );
            } else if (show_item_index) {
                fmt = " % 3d. %s";
                display_linef(fmt, bag_index + 1, o->name );
            } else if (o->value != 0) {
                fmt = " %s worth $%d";
                display_linef(fmt, o->name, o->value );
            } else {
                fmt = " %s";
                display_linef(fmt, o->name );
            }
        }
    }
}

// return true if carrying any items
bool actor_has_any_items(const GameState * gs) {
    return gs->items_len > 0;
}

// return true if the user is carrying this item
bool actor_has_item(const GameState *gs, const object_id id) {
    if (id < 1 || id >= obj_num_objects()) {
        return false;
    }
    const int items_len = gs->items_len;
    for (int bag_index = 0; bag_index < items_len; ++bag_index) {
        if (gs->items[bag_index] == id) {
            return true;
        }
    }
    return false;
}

// return true if the user is carrying this item
bool actor_has_item_named(const GameState *gs, char const *item_name) {
    if (!item_name) return false;
    const int items_len = gs->items_len;
    for (int i = 0; i < items_len; ++i) {
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

// add the object id to the actor's items[] array.
// Returns true if the object was successfully added.
// Returns false if items[] is full, if the object_id is out of bounds, or if the object is already in items[]
bool actor_add_object(GameState *gs, const object_id id) {
    // todo (rob) we need better error reporting via returning a ErrOrResult struct
    if (id < 1 || id > obj_num_objects() - 1 ) return false;  // id out of bounds.
    int item_len = gs->items_len;
    if ( item_len == MAX_PLAYER_OBJECTS ) return false;
    for (int i = 0; i < item_len; ++i) {
        if (gs->items[i] == id) return false;  // already carrying this item
    }
    gs->items[item_len] = id;  // add to end of array
    gs->items_len++;
    obj_relocate_object(id, PLAYER_LOCATION);
    return true;
}

// Removes the object id from the user's list of items.
// Returns true if the user was carrying this item, or false if the item was not present.
// after this method completes, the object id will no longer be in the agent's item list.
// items[] is a swap-and-pop list, so the deleted element position is replaced by the last element
// in the list.
bool actor_remove_object(GameState *gs, const object_id id) {
    if (id < 1 || id >= obj_num_objects()) {
        return false;
    }
    const int items_len = gs->items_len;
    for (int bag_index = 0; bag_index < items_len; ++bag_index) {
        if (gs->items[bag_index] == id) {
            gs->items[bag_index] = gs->items[items_len - 1];  // 'swap'
            gs->items[items_len - 1] = 0; // 'pop'
            gs->items_len--;
            obj_clear_location(id);
            return true;
        }
    }
    return false;
}

void actor_remove_all_objects( GameState * gs) {
    const int items_len = gs->items_len;
    for (int i = 0; i < items_len; ++i) {
        obj_clear_location(gs->items[i]);
        gs->items[i] = 0;
    }
}

bool cmd_look(GameState *gs) {
    // This is a presentation-layer-only command. It just displays text to the user they have already seen.
    display_room_desc(gs);
    display_room_content(gs);
    return true;
}

bool cmd_quit(GameState * gs) {
    if ( global_string_assets.conclusion_quit ) {
        display_line(global_string_assets.conclusion_quit );
    }
    gs->game_over = true;
    gs->ended_by_quitting = true;
    // todo (rob) ask for confirmation?
    return END_GAME;
}

void display_char_attributes(const CharStats stats) {
    if (GLOBALS.silent_mode) return;
    display_linef("Strength:  %3d  Charisma:     %3d",
        stats.strength, stats.charisma);

    display_linef("Dexterity: %3d  Intelligence: %3d",
        stats.dexterity, stats.intelligence );

    display_linef( "Wisdom:    %3d  Constitution: %3d",
        stats.wisdom, stats.constitution);
}


void display_conclusion(const GameState *gs) {
    if (GLOBALS.silent_mode) return;

    set_char_sleep(_30ms); // so final text display is slowed down
    display_line("");

    if (gs->completed && !gs->is_dead) {
        displayf("%s, ",gs->player_name->buffer);
        display_line(global_string_assets.conclusion_completed);
    } else if (gs->is_dead) {
        display_line(global_string_assets.conclusion_died);
    }
}



void display_game_state(const GameState *gs) {
    printf("\n(GameState){ player_name=%s, room=%d, turns=%d, cash=%d, killed=%d, fought=%d, magic=%d, "
           "has_torch=%d, is_dead=%d, completed=%d, must_fight=%d }\n",
        gs->player_name->buffer, gs->room,  gs->turns, gs->cash, gs->monsters_killed,gs->monsters_fought, gs->magic,
        gs->has_torch, gs->is_dead, gs->completed, gs->must_fight);

    printf("(items[%d]){ ", gs->items_len);
    for (int i = 0; i < MAX_ITEMS_CAPACITY; ++i) {
        printf("%d, ", gs->items[i]);
    }
    printf("}\n");

    struct ObservationSpace os = gs->perception;
    printf("(ObservationSpace){ .monster_is_visible=%d, .treasure_is_visible=%d, .must_fight=%d, .current_monster.id=%d, .current_treasure.id=%d }\n",
        os.monster_is_visible, os.treasure_is_visible, os.must_fight, os.current_monster.id, os.current_treasure.id);
    room_repr(room_find_room(gs->room));
    room_graph_entry_repr(gs->room);
    display_char_attributes(gs->stats);
    actor_display_inventory(gs, true, true);
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

void display_globals(void) {
    const u32 saved_sleep = set_char_sleep(0);
    char  buffer[1024] = {};
    int n = snprintf(buffer, sizeof buffer,
        "(GlobalState){ player_name=%s, silent_mode=%d, char_sleep_duration=%d, "
            "char_sleep_visited_duration%d, debug_normal_sleep=%d, "
            "debug_visited_sleep=%d, debug_mode=%d }\n",
            GLOBALS.player_name->buffer, GLOBALS.silent_mode, saved_sleep,
            GLOBALS.char_sleep_visited_duration, GLOBALS.debug_normal_sleep,
            GLOBALS.debug_visited_sleep, GLOBALS.debug_mode );
    if (n < 1 ) printf("display_globals: possible truncation of output.\n");
    display_paginated(buffer, 100);

    // printf("(GlobalState){ player_name=%s, silent_mode=%d, char_sleep_duration=%d, "
    //         "char_sleep_visited_duration%d, debug_normal_sleep=%d, "
    //         "debug_visited_sleep=%d, debug_mode=%d }\n",
    //         GLOBALS.player_name, GLOBALS.silent_mode,GLOBALS.char_sleep_duration,
    //         GLOBALS.char_sleep_visited_duration, GLOBALS.debug_normal_sleep,
    //         GLOBALS.debug_visited_sleep, GLOBALS.debug_mode);

    set_char_sleep( saved_sleep );
}

void display_random_room_text(GameState * gs, const RandomTextArray *rta) {
    if (GLOBALS.silent_mode ) return;
    for (size_t i=0; i < rta->length; ++i) {
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
    const Room *r = room_find_room(gs->room);

    if ( !r->is_lit_bit && !gs->has_torch && !room_contains_lit_object(r) ) {
        display_line("It is too dark to see anything.");
        return;
    }

    display_line(r->name);
    if (r->preamble) {
        display_random_room_text(gs, r->preamble);
    }

    display_paginated(r->desc, 80);

    if (r->epilog) {
        display_random_room_text(gs, r->epilog);
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
            display_linef("You come face to face with a %s.", m_name);
        } else if ( chance < .66){
            display_linef("The room contains a %s.", m_name);
        } else {
            display_linef("LOOK OUT! There is a %s here!", m_name);
        }
    } else {
        display_line("You feel a dangerous presence!");
    }
}

void display_room_treasure(const GameState * gs) {
    if (GLOBALS.silent_mode) return;
    const Room *room = room_find_room(gs->room);
    if (room_is_empty(room)) return;

    if ( !room->is_lit_bit && !gs->has_torch && !room_contains_lit_object(room) ) {
        return;
    }

    const int len = room->objects_len;

    char buffer[1024] = "\nYou can see";
    char *s = &buffer[0] + strlen(buffer);
    char const *fmt;
    int fmt_len;
    int written;
    for (int i = 0; i < len - 1 ; ++i) {
        if (room->objects[i]) {
            const Object *o = obj_find_object(room->objects[i]);
            if ( o->value == 0 ) {
                fmt = " a %s,";
                fmt_len = (int)strlen(fmt);
                written = snprintf(s, strlen(o->name) + fmt_len , fmt, o->name);
            } else {
                fmt = " a %s worth $%d,";
                fmt_len = (int)strlen(fmt);
                written = snprintf(s, strlen(o->name) + fmt_len , fmt, o->name, o->value);
            }
            s += written;
        }
    }
    // handle last object with the conjunction "and." Yup, that allotta code for a measly "and!"
    const Object *o = obj_find_object(room->objects[len - 1 ]);
    if ( o->value == 0 ) {
        if ( len > 1 ) {
            fmt = " and a %s";
        } else {
            fmt = " a %s";
        }
        fmt_len = (int)strlen(fmt);
        written = snprintf(s, strlen(o->name) + fmt_len , fmt, o->name);
    } else {
        if ( len > 1 ) {
            fmt = " and a %s worth $%d";
        } else {
            fmt = " a %s worth $%d";
        }
        fmt_len = (int)strlen(fmt);
        written = snprintf(s, strlen(o->name) + fmt_len , fmt, o->name, o->value);
    }
    s += written;
    display_paginated(buffer, 80);
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

bool room_transfer_obj_location(const Room *r, object_id id, int location) {
    if (!r) return false;
    if ( r->id == location ) return true;  // if relocating obj to same room, nothing to do

    room_remove_object(r, id);
    obj_relocate_object(id, location);
    ROOM_GRAPH[r->id][RGINDEX_TREASURE] = 0;  //todo need to deprecate use of RGINDEX_TREASURE and look at Room.objects

    return true;
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


CharBuffer *get_player_name(char const *greeting) {
    char const *g = greeting ? greeting : "Hello, Explorer ";
    cls();
    CharBuffer *cb = get_char_buffer("What is your name, explorer? ");
    display_linef("%s %s.", g, cb->buffer);
    return cb;
}

