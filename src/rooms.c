// rooms.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/28 21:38:52 PDT


#include "rooms.h"



// Returns ROOM_ERR_OBJECT_NOT_FOUND if object_id is not present in the room.
// Otherwise, returns the array index into the Room's objects[]
// at which this object is an element.
int room_index_for_object(const Room *r, const int object_id ) {
    for (int i = 0; i < 10; ++i) {
        if ( r->objects[i] == object_id) {
            return i;
        }
    }
    return ROOM_ERR_OBJECT_NOT_FOUND;
}

// Returns the object id of the first object in the room, or ROOM_OBJECT_NOT_FOUND if there are no items
int room_first_object_index(const Room *r) {
    for (int i = 0; i < 10; ++i) {
        if (r->objects[i] != 0 ) {
            return r->objects[i];
        }
    }
    return ROOM_ERR_OBJECT_NOT_FOUND;
}

// Returns true if no more objects can be placed in this Room,
// otherwise returns false.
bool room_is_full(const Room *r ) {
    for (int i = 0; i < 10; ++i) {
        if ( r->objects[i] == 0) {
            return false;
        }
    }
    return true;
}

// Returns the number of objects currently in this room
int room_count_of_objects(const Room *r) {
    int count = 0;
    for (int i = 0; i < 10; ++i) {
        if ( r->objects[i] != 0) {
            count++;
        }
    }
    return count;
}

int room_remove_object(Room *r, const int object_id) {
    for (int i = 0; i < 10; ++i) {
        if ( r->objects[i] == object_id ) {
            r->objects[i] = 0;
            room_objects_relocate_object(object_id, 0);
            return ROOM_SUCCESS;
        }
    }
    return ROOM_ERR_OBJECT_NOT_FOUND;
}

int room_add_object(Room *r, const int object_id) {
    if (room_index_for_object(r, object_id) != ROOM_ERR_OBJECT_NOT_FOUND ) {
        return ROOM_ERR_ALREADY_GOT_ONE_YOU_SEE_ITS_VERY_NICE;
    }

    for (int i = 0; i < 10; ++i) {
        if ( r->objects[i] == 0) {
            room_objects_relocate_object(object_id, r->id);
            r->objects[i] = object_id;
            return ROOM_SUCCESS;
        }
    }
    return ROOM_ERR_ROOM_FULL;
}