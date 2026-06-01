// rooms.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/28 21:38:52 PDT


#include "rooms.h"

#include <stdio.h>

/*
 *  Word wrap notes:
 *      We don't break words. Only wrap whole words. If word ends with punctuation like period or comma or semicolon,
 *      and punctuation makes the word long enough to wrap, keep it on the same line even if it exceeds max length by 3 chars max for an ellipses.
 *      If first characters of a new line is whitespace, eat it.
 *      A newline will force a new line with an extra line before the new line. Think of newline as new paragraph
 *
 */

Room ROOMS[NUM_ROOMS] = {
{.id =  0,  .name= "NULL ROOM",  .desc = "" },
{.id =  1,  .name= "ROOM 1",     .desc = "You are out on the battlements of the Chateau. There is only one way back." },
{.id =  2,  .name= "ROOM 2",     .desc = "This is an eerie room, where once magicians consorted with evil sprites and werebeasts. Exits lead in three directions. An evil smell comes from the south." },
{.id =  3,  .name= "ROOM 3",     .desc = "An old straw mattress lies in one corner. It has been ripped apart to find any treasure which was hidden in it. Light comes fitfully from a window to the north, and around the doors to south, east, and west." },
{.id =  4,  .name= "ROOM 4",     .desc = "This wooden-panelled room makes you feel damp and uncomfortable. There are three doors leading from this room, one made of iron. Your sixth sense warns you to choose carefully..." },
{.id =  5,  .name= "ROOM 5",     .desc = "You ignore your intuition... A Spell of Living Stone, primed to trap the first intruder has been set on you. With your last seconds of life you have time only to feel profound regret..." },
{.id =  6,  .name= "ROOM 6",     .desc = "You are in an L-shaped room. Heavy parchment lines the walls. You can see through an archway to the east, but that is not the only exit from this room." },
{.id =  7,  .name= "ROOM 7",     .desc = "There is an archway to the west, leading to an L-shaped room. A door leads in the opposite direction." },
{.id =  8,  .name= "ROOM 8",     .desc = "This must be the Chateau's main kitchen, but any food left here has long rotted away. A door leads to the north, and there is one to the west." },
{.id =  9,  .name= "ROOM 9",     .desc = "You find yourself in a small room, which makes you feel claustrophobic. There is a picture of a black dragon painted on the north wall, above the door." },
{.id = 10,  .name= "ROOM 10",    .desc = "A stairwell ends in this 'room', which is more of a landing than an actual room. The door to the north is made of iron, which has rusted over the centuries." },
{.id = 11,  .name= "ROOM 11",    .desc = "There is a stone archway to the north. You are in a very long room. You are in a very long room.\nFresh air blows down some stairs and rich red drapes cover the walls. You can see doors to the south and east." },
{.id = 12,  .name= "ROOM 12",    .desc = "You have entered a room filled with swirling, choking smoke. You must leave quickly to remain healthy enough to continue your chosen quest." },
{.id = 13,  .name= "ROOM 13",    .desc = "There is a mirror in the corner. You glance at it, and feel suddenly very ill.\nYou realize the looking-glass has been infused with a Spell of Charisma Reduction... oh dear...." },
{.id = 14,  .name= "ROOM 14",    .desc = "This room is richly finished with a white marble floor. Strange footprints lead to the two doors from this room. Dare you follow them?" },
{.id = 15,  .name= "ROOM 15",    .desc = "You are in a long, long hallway, lined on each side with rich, red drapes.\nThey are parted halfway down the east wall where there is a door." },
{.id = 16,  .name= "ROOM 16",    .desc = "Someone has spent a long time painting this room a bright yellow.\nYou remember reading that yellow is the Ancient Oracle's Color of Warning..." },
{.id = 17,  .name= "ROOM 17",    .desc = "As you stumble down the ladder you fall into the room. The ladder crashes down behind you. There is now no way back.\nA small door leads east from this very cramped room." },
{.id = 18,  .name= "ROOM 18",    .desc = "You find yourself in the Hall of Mirrors, and see yourself reflected a hundred times or more. Through the bright glare you can make out doors in all directions. You notice the mirrors around the east door are heavily tarnished." },
{.id = 19,  .name= "ROOM 19",    .desc = "You find yourself in a long corridor... Your footsteps echo as you walk." },
{.id = 20,  .name= "ROOM 20",    .desc = "You feel as if you've been wandering around this Chateau forever, and you begin to despair of ever escaping.\nStill, you can't get too depressed but must struggle on. Looking around, you see that you are in a room which has a heavy timbered ceiling and white roughly-finished walls.\nThere are two doors..." },
{.id = 21,  .name= "ROOM 21",    .desc = "You are in a small alcove. You look around, but can see nothing in the gloom. Perhaps if you wait a while your eyes will adjust to the murky dark of this alcove." },
{.id = 22,  .name= "ROOM 22",    .desc = "A dried-up fountain stands in the center of this courtyard, which once held beautiful flowers but have have long since died." },
{.id = 23,  .name= "ROOM 23",    .desc = "The scent of dying flowers fills this brightly-lit room.\nThere are two exits from it." },
{.id = 24,  .name= "ROOM 24",    .desc = "This is a round stone cavern off the side of the alcove to your north." },
{.id = 25,  .name= "ROOM 25",    .desc = "You are in an enormous circular room, which looks as if it was used as a games room. Rubble covers the floor, partially blocking the only exit." },
{.id = 26,  .name= "ROOM 26",    .desc = "Through the dim mustiness of this small potting shed you can see a stairwell." },
{.id = 27,  .name= "ROOM 27",    .desc = "You begin this Adventure in a small wood outside the Chateau.\nWhile out walking one day, you come across a small, ramshackle shed in the woods. Entering it, you see a hole in one corner. An old ladder leads down from the hole." },
{.id = 28,  .name= "ROOM 28",    .desc = "How wonderful! Fresh air, sunlight, birds are singing. You are free at last." },
{.id = 29,  .name= "ROOM 29",    .desc = "The smell came from bodies rotting in huge traps. One springs shut on you, trapping you forever!" },
{.id = 30,  .name= "ROOM 30",    .desc = "You fall into a pit of flames." },
{.id = 31,  .name= "ROOM 31",    .desc = "Aaaaahhh... you have fallen into a pool of acid. Now you know - too late - why the mirrors were so badly tarnished." },
{.id = 32,  .name= "ROOM 32",    .desc = "It's too bad you chose that exit from the alcove. A giant funnel-web spider leaps on you, and before you can react, bites you on the neck. You have 10 seconds to live." },
{.id = 33,  .name= "ROOM 33",    .desc = "A stairwell leads into this room, a poor and common hovel with many doors and exits." },
{.id = 34,  .name= "ROOM 34",    .desc = "It is hard to see in this room and you slip slightly on the uneven, rocky floor." },
{.id = 35,  .name= "ROOM 35",    .desc = "Horrors! This room was once the torture chamber of the Chateau.\nSkeletons lie on the floor, still with chains around their bones." },
{.id = 36,  .name= "ROOM 36",    .desc = "Another room with very unpleasant memories.\nThis foul hole was used as the Chateau dungeon." },
{.id = 37,  .name= "ROOM 37",    .desc = "Oh no, this is a gargoyle's lair. It has been held prisoner here for three hundred years.\nIn his frenzy he thrashes out at you and... breaks your neck!!" },
{.id = 38,  .name= "ROOM 38",    .desc = "This was the Lower Dancing Hall. With doors to the north, the east, and to the west, you would seem to be able to flee any danger." },
{.id = 39,  .name= "ROOM 39",    .desc = "This is a dingy pit at the foot of some extremely dubious-looking stairs. A door leads to the east." },
{.id = 40,  .name= "ROOM 40",    .desc = "Doors open to each compass point from the Trophy Room of the Chateau.\nThe heads of strange creatures shot by the ancestral owners are mounted high up on each wall." },
{.id = 41,  .name= "ROOM 41",    .desc = "You have stumbled on to a secret room.\nDown here, eons ago, the ancient Necromancers of Thorin plied their evil craft... and the remnant of their spells hangs heavy on the air." },
{.id = 42,  .name= "ROOM 42",    .desc = "Cobwebs brush your face as you make your way through the gloom of this room of shadows." },
{.id = 43,  .name= "ROOM 43",    .desc = "This gloomy passage lies at the intersection of three rooms." },
{.id = 44,  .name= "ROOM 44",    .desc = "You are in the rear turret room, below the extreme western wall of the ancient Chateau." },
};


const Room * room_find_room(const room_id id) {
    return &ROOMS[id];
}

Room * pvt_room_find_room(const room_id id) {
    return &ROOMS[id];
}

// Returns true if the object in the argument is located in the room, otherwise returns false.
bool room_contains_object(const Room *r, const object_id id) {
    for (int i = 0; i < 10; ++i) {
        if ( r->objects[i] == id) {
            return true;
        }
    }
    return false;
}

void room_set_visited_flag(const Room *r) {
    Room *mutable_room = pvt_room_find_room(r->id);
    mutable_room->is_visited_bit = true;
}

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
int room_first_object_id(const Room *r) {
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
            obj_relocate_object(object_id, 0);
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
            obj_relocate_object(object_id, r->id);
            r->objects[i] = object_id;
            return ROOM_SUCCESS;
        }
    }
    return ROOM_ERR_ROOM_FULL;
}

void room_repr(const Room *r) {
    printf("(Room){ .id=%d, .name='%s', .desc='%s'", r->id, r->name, r->desc);
    printf("  (Monster){ .id=%d, .name='%s' } }\n", r->monster, monsters_name_for_id(r->monster));
}

void room_rooms_repr() {
    printf("ROOMS[%d] = {\n", NUM_ROOMS);
    for (int i = 0; i < NUM_ROOMS; ++i) {
        room_repr(&ROOMS[i]);
    }
    printf("};\n");
}