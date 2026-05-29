// room_objects.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/28 22:10:52 PDT



#include <stdio.h>
#include <stdlib.h>
#include <sys/_types/_size_t.h>

#include "room_objects.h"

#include <string.h>

#include "rooms.h"

struct ObjectStore {
    size_t capacity;
    size_t size;
    Object objects[]; // flexible array
};

struct ObjectStore * pvt_objects = {};

constexpr int ROOM_OBJECT_NOT_FOUND = -1;
constexpr int ROOM_OBJECT_NULL_OBJECT_NAME = 0;

Object * find_object(const int object_id) {
    const size_t size = pvt_objects->size;
    for (int i = 0; i < size; ++i) {
        if (pvt_objects->objects[i].id == object_id) {
            return &pvt_objects->objects[i];
        }
    }
    return nullptr;
}


char const * room_objects_name_for_object_id(object_id object_id) {
    Object *o = find_object(object_id);
    if (!o) {
        return pvt_objects->objects[0].name;
    }
    return o->name;
}

int room_objects_index_for_name(char const item_name[static 1]) {
    if (!item_name) return ROOM_OBJECT_NULL_OBJECT_NAME;

    const size_t size = pvt_objects->size;
    for (int i = 1; i < size; ++i) {
        printf("room_objects_index_for_name: item_name: %s, objects[%d].name:%s, strlen:%zd\n",
            item_name, i, pvt_objects->objects[i].name, strlen(item_name));
        if (strncmp(item_name, pvt_objects->objects[i].name, strlen(item_name)) == 0 ) {
            return pvt_objects->objects[i].id;
        }
    }
    return ROOM_OBJECT_NOT_FOUND;
}

// Changes location of the object
// Returns the old location
int room_objects_relocate_object(const int object_id, const int new_location) {
    Object *o = find_object(object_id);
    int old_loc = o->location;
    o->location = new_location;
    return old_loc;
}

int room_objects_init(const size_t size, Object data[static size]) {
    // we add one extra Object element for the null object element, id = 0.
    const size_t capacity = size + 1;
    pvt_objects  = calloc( 1, sizeof(struct ObjectStore) +  ( sizeof(Object) * capacity ) );
    if (! pvt_objects ) {
        return - 1;
    }

    pvt_objects->capacity = capacity;

    int obj_index = 0;
    pvt_objects->objects[obj_index++] = (Object){ .id =  0, .name="NULL OBJECT" };

    for (int data_index = 0 ; data_index < size; ++data_index) {
        if (data[data_index].id < 1) {
            continue;  // only copy ids > 0
        }
        pvt_objects->size = obj_index;
        pvt_objects->objects[obj_index++] = data[data_index];
    }
    pvt_objects->size = obj_index;

    return 0;  // no errors
}


void room_objects_free(void) {
    void * saved = pvt_objects;
    pvt_objects = 0;
    free(saved);
}


// print info about the Objects managed in this unit
void room_objects_repr(void) {
    const size_t size = pvt_objects->size;
    printf("\n(ObjectStore){.capacity=%zd, .size=%zd, .objects[]=\n", pvt_objects->capacity, pvt_objects->size);
    for (int i = 0; i < size; ++i) {
        const Object *o = &pvt_objects->objects[i];
        printf("  (Object){ .id=%4d, .name=%-30s, .value=%4d, .location=%4d},\n", o->id, o->name, o->value, o->location);
    }
    printf("}\n");
}

#ifdef ROOM_OBJECTS_MAIN

int main(void) {
    Object data[20] = {
        {.id =  1, .name="axe" },
        {.id =  2, .name="sword" },
        {.id =  3, .name="dagger" },
        {.id =  4, .name="mace" },
        {.id =  5, .name="quarterstaff" },
        {.id =  6, .name="morning star" },
        {.id =  7, .name="falchion" },
        {.id =  8, .name="crystal ball", .value=99 },
        {.id =  9, .name="amulet", .value=247 },
        {.id = 10, .name="ebony ring", .value=166 },
        {.id = 11, .name="gems", .value=462 },
        {.id = 12, .name="mystic scroll", .value=195 },
        {.id = 13, .name="healing potion", .value=231 },
        {.id = 14, .name="dilithium crystals", .value=162 },
        {.id = 15, .name="copper pieces", .value=27 },
        {.id = 16, .name="diadem", .value=141 },
        {.id = 17, .name="silver key" },
        {.id = 18, .name="golden key" },
        {.id = 19, .name="chest of stone" },
        {.id = 20, .name="chest made of iron" },
    };



    int result = room_objects_init(20, data);
    printf("init result: %d\n", result);

    room_objects_repr();



    room_objects_free();
}
#endif