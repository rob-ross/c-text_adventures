// objects.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/22 23:27:11 PDT
//
// todo (rob) we need a less generic name than "object" here.
// these objects are game objects that a player can interact with, like weapons, treasure, scrolls,
// potions, torches, etc.

#pragma once

//// ------------------------------------------------------------
////
////    ROOM OBJECTS
////      weapons, treasures, things in a room you can interact with
////      or pick up
////
//// ------------------------------------------------------------

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

typedef int object_id;

// todo (rob) need a better name than Item or Object. It's a thingee you can pick up and carry.
// at least 3 categories; weapon, treasure, usable item: e.g., a key, rope, things you use to advance game state.
// UserItem vs RoomItem? UserItem would be things a user can pick up, like a weapon or treasure, key.
// RoomItem would be something attached or part of the room, that can have its own state, and be manipulated by user.
// a user probably can't take a RoomItem but maybe move it or destroy it?  Maybe if it's a gold statue attached to the
// floor it's a RoomItem but if the user is somehow able to pry it loose it can change into a UserItem?

typedef struct Object {
    char const *       name;
    object_id          id;
    int                value;
    int                location; // 0 == nowhere, >0 == Room id, -1 == In player's bag.

    // flags. eventually can be bit flags for efficiency
    bool                is_open_bit; // set to true if the Object is "open", like a chest.

    // todo (rob): two issues.
    // 1. Can we consider drinkable/eatable as the same basic action, i.e., "consumable?"
    // 2. ZILF handles this different. Every object can be given an Action Routine, and that
    //     examines the parsed VO and VIO and handles the command. So we don't need a bit for
    //     every conceivable interaction possible with an object, we can customize what objects can
    //     do. Maybe a statue becomes edible?
    //      Then again, eating and drinking are probably ubiquitous enough in these games to deserve
    //      top-level handling. It automates and standardizes the responses to these actions.
    bool                is_drinkable_bit; // true if the object can be drunk/swallowed
    bool                is_eatable_bit;   // true if the object can be eaten



    bool                is_light_source_bit; // is a light source and can be turned on/off
    bool                is_lit_bit; // if this is a light source, weather it is lit or not
    bool                is_flame_bit; // true if the object is a source of fire, like a torch
    bool                is_flammable_bit;  // true if fire can consume the object
    // true if an actor has interacted with this object, i.e., picked it up, manipulated it
    bool                is_touched_bit;

    // Usually, if an object is takeable, then it is also droppable. But we may have special circumstances like
    // "cursed" gear that cannot be dropped normally and require some special action to get rid of.
    bool                is_takeable_bit; // the object can be taken, i.e., picked up
    bool                is_droppable_bit; // the object can be dropped.



} Object;

constexpr int ROOM_OBJECT_NOT_FOUND = -1;
constexpr int ROOM_OBJECT_NULL_OBJECT_NAME = 0;

int room_objects_init(size_t size, Object data[static size]);
void room_objects_free(void);
void room_objects_repr(void);
int room_objects_relocate_object( int id,  int new_location);
int room_objects_id_for_partial_name(char const item_name[static 1]);
char const * room_objects_name_for_object_id(object_id id);
const Object * find_object(object_id id);
bool room_objects_set_open_flag(object_id id);