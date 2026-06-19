// rooms.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/28 21:38:52 PDT


#include "rooms.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "objects.h"
#include "common/cu_string.h"

typedef struct RoomStore {
    size_t capacity;
    size_t size;
    Room rooms[]; // flexible array
} RoomStore;

// This store manages each object as a unique identity.
// E.g., the "chest of stone" is a single unique object in the global environment.
// If we want to populate many "chest of stone" objects in multiple locations, we need a higher-level data structure
// like a Class or a prototype for each type of such objects.
static RoomStore * pvt_rooms = {};

extern const int MAX_ROOM_OBJECTS;

Room * pvt_room_find_room(const room_id id) {
    return &pvt_rooms->rooms[id];
}



int room_init(size_t size, Room data[static size]) {
    // we add one extra Object element for the null object element, id = 0.
    const size_t capacity = size + 1;
    pvt_rooms  = calloc( 1, sizeof( RoomStore) +  ( sizeof(Room) * capacity ) );
    if (! pvt_rooms ) {
        return - 1;
    }

    pvt_rooms->capacity = capacity;

    int room_index = 0;
    pvt_rooms->rooms[room_index++] = (Room){ .id =  0, .name="NULL ROOM" };
    for (size_t data_index = 0 ; data_index < size; ++data_index) {
        if (data[data_index].id < 1) {
            continue;  // only copy ids > 0
        }
        pvt_rooms->size = room_index;
        pvt_rooms->rooms[room_index++] = data[data_index];
    }
    pvt_rooms->size = room_index;

    return 0;
}

static void pvt_destroy_rooms() {
    const int num_rooms = room_num_rooms();
    for (int room_index = 0; room_index < num_rooms; ++room_index) {
        const Room *r = room_find_room(room_index);
        free(r->preamble);
        free(r->epilog);
    }
}

void room_destroy() {
    pvt_destroy_rooms();
    void * saved = pvt_rooms;
    pvt_rooms = nullptr;
    free(saved);
}

int room_add_object(const Room *room, const object_id id) {
    if ( room_contains_object(room, id) ) {
        printf("room_add_object; object_id: %d is already here.\n", id);
        return ROOM_ERR_ALREADY_GOT_ONE_YOU_SEE_ITS_VERY_NICE;
    }
    if (id < 1 || id > obj_num_objects() - 1 ) return ROOM_ERR_OBJECT_ID_OUT_OF_BOUNDS;
    const int objects_len = room->objects_len;
    if ( objects_len == MAX_ROOM_OBJECTS ) return ROOM_ERR_ROOM_FULL;
    Room *r = ((Room*)room);
    r->objects[objects_len] = id;
    r->objects_len++;
    obj_relocate_object(id, room->id);
    return ROOM_SUCCESS;
}


int room_remove_object(const Room *room, const int object_id) {
    const int objects_len = room->objects_len;
    Room *r = (Room*)room;
    for (int i = 0; i < objects_len; ++i) {
        if ( r->objects[i] == object_id ) {
                r->objects[i] = r->objects[objects_len - 1];  // swap
                r->objects[objects_len - 1] = 0; // pop
                r->objects_len--;
                obj_clear_location(object_id);
                return ROOM_SUCCESS;
        }
    }
    return ROOM_ERR_OBJECT_NOT_FOUND;
}

void room_remove_all_objects(room_id id) {
    Room *r = pvt_room_find_room(id);
    const int objects_len = r->objects_len;
    for (int i = 0; i < objects_len; ++i) {
        obj_clear_location(r->objects[i]);
    }
    memset(r->objects, 0, sizeof(object_id[ROOM_OBJECTS_CAPACITY]));
}

// Clear the monster in the current room and its entry in the ROOMS array
bool room_clear_monster(const Room *r) {
    ROOM_GRAPH[r->id][RGINDEX_MONSTER] = 0;
    pvt_rooms->rooms[r->id].monster = 0;
    return true;
}

bool room_contains_lit_object(const Room *r ) {
    const int num_objects = r->objects_len;
    for (int i = 0; i < num_objects; ++i) {
        const Object *o = obj_find_object(r->objects[i]);
        if (o) {
            if (o->is_light_source_bit && o->is_lit_bit) {
                return true;
            }
        }
    }
    return false;
}

bool room_contains_monster_named( const Room *r, const char *monster_name  ) {
    if (!r || !monster_name || r->monster == 0 ) return false;
    const monster_id id = r->monster;
    const char *room_monster_name = monsters_name_for_id(id);
    if (! room_monster_name) return false;
    return string_starts_with_ignore_case(monster_name, room_monster_name);
}

bool room_contains_object(const Room *r, const object_id id) {
    const int len = r->objects_len;
    for (int i = 0; i < len; ++i) {
        if ( r->objects[i] == id) {
            return true;
        }
    }
    return false;
}

// Returns the number of objects currently in this room
int room_count_of_objects(const Room *r) {
    return r->objects_len;
}

int room_count_visited() {
    int count = 0;
    const int num_rooms = (int)pvt_rooms->size;
    for (int i = 0; i < num_rooms; ++i) {
        if ( pvt_rooms->rooms[i].is_visited_bit ) count++;
    }
    return count;
}

RandomTextArray * create_rta(int length) {
    const size_t mem_size = sizeof(RandomTextArray) + sizeof(RandomText) * length;
    RandomTextArray * result = calloc(1, mem_size);
    if ( !result ) {
        return nullptr;
    }
    result->length = length;
    return result;
}



const Room * room_find_room(const room_id id) {
    return &pvt_rooms->rooms[id];
}

// Searches the Room for an object whose name starts with `starts_with`, ignoring case.
// Returns the Object if found, or nullptr if not found.
const Object * room_find_object_named(const Room *r, char const partial_name[static 1]) {
    const int objects_len = r->objects_len;
    for (int i = 0; i < objects_len; ++i) {
        const Object *o  = obj_find_object(r->objects[i]);
        if (string_starts_with_ignore_case(partial_name, o->name) ){
            return o;
        }
    }
    return nullptr;
}

// Returns the object id of the first object in the room, or ROOM_OBJECT_NOT_FOUND if there are no items
int room_first_object_id(const Room *r) {
    const int len = r->objects_len;
    if ( len > 0 ) {
        return r->objects[0];
    }
    return ROOM_ERR_OBJECT_NOT_FOUND;
}

// Returns ROOM_ERR_OBJECT_NOT_FOUND if object_id is not present in the room.
// Otherwise, returns the array index into the Room's objects[]
// at which this object is an element.
int room_index_for_object(const Room *r, const int object_id ) {
    const int len = r->objects_len;
    for (int i = 0; i < len; ++i) {
        if ( r->objects[i] == object_id) {
            return i;
        }
    }
    return ROOM_ERR_OBJECT_NOT_FOUND;
}

// Returns true if no more objects can be placed in this Room,
// otherwise returns false.
bool room_is_full(const Room *r ) {
    return r->objects_len == MAX_ROOM_OBJECTS;
}

// Returns true if there are no objects in this room
bool room_is_empty(const Room *r) {
    return r->objects_len == 0;
}

// Returns the number of rooms
int room_num_rooms(void) {
    return (int)pvt_rooms->size;
}

int room_num_nice_rooms(void) {
    // todo (rob) we need to add a flag like "is_death_room" to the Room struct so we can compute this
    // dynamically. right now we hard-code this in the .h file for the main file.

    return (int)pvt_rooms->size; // - NUM_DEATH_ROOMS
}


void room_repr(const Room *r) {
    printf("(Room){ .id=%d, .name='%s', .desc='%.20s...'", r->id, r->name, r->desc);
    const int len = r->objects_len;

    printf("(objects)[%d]{ ", len);
    for (int i = 0; i < len; ++i) {
        printf("%d, ", r->objects[i]);
    }
    printf("}, ");
    printf("  (Monster){ .id=%d, .name='%s' } }\n", r->monster, monsters_name_for_id(r->monster));
}

void room_all_rooms_repr() {
    int num_rooms = (int)pvt_rooms->size;
    printf("ROOMS[%d] = {\n", num_rooms);
    for (int i = 0; i < num_rooms; ++i) {
        room_repr(&pvt_rooms->rooms[i]);
    }
    printf("};\n");
}

const char * room_rgindex_label(const enum RoomGraphIndex rg_index) {
    switch (rg_index) {
        case RGINDEX_NORTH:         return "North";
        case RGINDEX_SOUTH:         return "South";
        case RGINDEX_EAST:          return "East";
        case RGINDEX_WEST:          return "West";
        case RGINDEX_UP:            return "Up";
        case RGINDEX_DOWN:          return "Down";
        case RGINDEX_TREASURE:      return "Treasure";
        case RGINDEX_MONSTER:       return "Monster";
        case RGINDEX_REQUIRED_KEY:  return "Required Key";
        case RGINDEX_UNUSED:        return "Unused";
        default:                    return "Unknown";
    }
}

const char * room_rgindex_label_short(const enum RoomGraphIndex rg_index) {
    switch (rg_index) {
        case RGINDEX_NORTH:         return "N";
        case RGINDEX_SOUTH:         return "S";
        case RGINDEX_EAST:          return "E";
        case RGINDEX_WEST:          return "W";
        case RGINDEX_UP:            return "U";
        case RGINDEX_DOWN:          return "D";
        case RGINDEX_TREASURE:      return "T";
        case RGINDEX_MONSTER:       return "M";
        case RGINDEX_REQUIRED_KEY:  return "K";
        case RGINDEX_UNUSED:        return "0";
        default:                    return "?";
    }
}

// Takes ownership of the RandomTextArray and frees it in room_destroy().
bool room_set_epilog(room_id id, RandomTextArray *rta) {
    pvt_rooms->rooms[id].epilog = rta;
    return true;
}

// Takes ownership of the RandomTextArray and frees it in room_destroy().
bool room_set_preamble(room_id id, RandomTextArray *rta) {
    pvt_rooms->rooms[id].preamble = rta;
    return true;
}

bool room_set_monster(const Room *r, monster_id id) {
    ((Room *)r)->monster = id;
    return true;
}

void room_set_visited_flag(const Room *r) {
    Room *mutable_room = pvt_room_find_room(r->id);
    mutable_room->is_visited_bit = true;
}

void room_set_visit_started_flag(const Room *r) {
    Room *mutable_room = pvt_room_find_room(r->id);
    mutable_room->is_visit_started_bit = true;
}
