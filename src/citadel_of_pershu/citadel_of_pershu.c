// citadel_of_pershu.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/03 14:02:21 PDT

// citadel_of_pershu.c
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983
//
//
// Created 2026/05/15 01:35:01 PDT

// make :
// cd /Users/robross/Documents/Development/CLionProjects/text_adventures/src

/*
 * DEBUG:
clang -g -DCITADEL_OF_PERSHU_MAIN -fsanitize=address -fsanitize=leak -Wall -Werror \
    -Wno-unused-const-variable -Wno-unused-variable -Wno-unused-function \
    -std=c23 -o citadel_of_pershu.out citadel_of_pershu.c \
    ../mersenne_twister.c ../common/console_utils.c ../common/string.c ../parser.c ../objects.c ../rooms.c ../monsters.c


*/


#include "citadel_of_pershu.h"

constexpr int DEBUG_RAND_SEED = 67;
struct GlobalState GLOBALS = {.player_name = nullptr, .char_sleep_duration = _15ms };


// --------------------------------------------------------------
//      Forward references
// --------------------------------------------------------------
static void update_perception(GameState * gs);
static int calc_score(const GameState * gs) ;
bool perform_action(GameState *gs, char action, int arg1, int arg2, int arg3);


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


static int roll_d6(GameState * gs, const int num_dice) {
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



//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------

static void display_char_attributes(const CharStats stats) {
    if (GLOBALS.silent_mode) return;
    vdisplay_line("Strength:  %2d  Charisma:     %2d",
        stats.strength, stats.charisma);

    vdisplay_line("Dexterity: %2d  Intelligence: %2d",
        stats.dexterity, stats.intelligence );

    vdisplay_line( "Wisdom:    %2d  Constitution: %2d",
        stats.wisdom, stats.constitution);
}


static void actor_display_inventory(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    display_line("Items:");
    int item_count = 0;
    for (int bag_index = 1; bag_index < ITEM_COUNT; ++bag_index ) {
        if (gs->items[bag_index]) {
            vdisplay("%d. %s ", bag_index, obj_name_for_id(gs->items[bag_index]));
            item_count++;
            if ( ! (item_count % 3) ) {
                display_line("");  // display 3 items per line
            }
        }
    }
    if (item_count) {
        if ( item_count % 3) {
            display_line("");
        }
    } else {
        display_line("You have no items.");
    }

}


static void display_status(const GameState * gs) {
    if (GLOBALS.silent_mode ) return;
    vdisplay("magic spells: %d, ", gs->magic);

    if (!gs->cash) {
        display_line("You have no money.");
    } else {
        vdisplay_line("You have $%d.", gs->cash);
    }
}

static void display_inventory(const GameState * gs) {
    actor_display_inventory(gs);
    display_status(gs);
}

static void display_random_room_text(GameState * gs, const RandomTextArray *rta) {
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

static void display_room_desc(GameState * gs) {
    if (GLOBALS.silent_mode) return;

    if (!gs->has_torch && ROOM_GRAPH[gs->room][RGINDEX_TREASURE] != 1 ) {
        display_line("IT IS TOO DARK TO SEE ANYTHING!\n");
    } else {
        const Room *r = room_find_room(gs->room);
        if (r->preamble) {
            display_random_room_text(gs, r->preamble);
        }

        display_paginated(r->desc, 80);

        if (r->epilog) {
            display_random_room_text(gs, r->epilog);
        }
    }
}

static void display_room_monster(GameState * gs) {
    if (GLOBALS.silent_mode) return;

    const int monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    if ( monster_index == 0 ) {
        return;
    }
    display_line("");
    if (gs->has_torch ) {
        if (rnd_d(gs) < .5) {
            display("You come face to face with a ");
        } else {
            display("The room contains a ");
        }
        const Room *room =  room_find_room(gs->room);
        display(monsters_name_for_id(room->monster));
        display_line("");
    } else {
        display_line("YOU FEEL A DANGEROUS PRESENCE!");
    }
}

static void display_room_treasure(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    const int treasure_index = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    if ( treasure_index == 0 || (!gs->has_torch && treasure_index != ITEM_TORCH )) {
        return;
    }
    const Room *room = room_find_room(gs->room);

    if (room_count_of_objects(room) == 0) return;

    display("You can see ");
    for (int i = 0; i < 10; ++i) {
        if (room->objects[i]) {
            const Object *o = obj_find_object(room->objects[i]);
            if ( o->value == 0 ) display_line(o->name);
            else vdisplay_line( "%s worth $%d", o->name, o->value);
        }
    }
}

static void display_room_content(GameState * gs) {
    if (GLOBALS.silent_mode) return;

    display_room_treasure(gs);
    display_room_monster(gs);
}


static void display_conclusion(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

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



static void display_score(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    display("\nSCORE: ");
    printf("%d\n", calc_score(gs));
    const int rooms_visited = room_count_visited();
    printf("\nturns: %d, cash: %d, monsters fought: %d, killed: %d, rooms: %d\n",
        gs->turns, gs->cash, gs->monsters_fought, gs->monsters_killed, rooms_visited);
    printf("You completed %3.0f%% of the quest.\n", (double)rooms_visited * 100.0 / (room_num_rooms() - NUM_DEATH_ROOMS - 1 ) );

}

static void display_help_info(void) {
    if (GLOBALS.silent_mode) return;

    display_line("\nVALID COMMANDS ARE:\n");

    display_line("[H]elp       [I]nventory  [Q]uit");
    display_line("[A]ttributes [T]ally");
    display_line("[R]etreat    [F]ight");
    display_line("[P]ick up    [G]et rid of");
    display_line("[N]orth      [S]outh");
    display_line("[E]ast       [W]est");
    display_line("[U]p         [D]own");

    display_line("\nDEBUG:");
    display_line("g[L]obals  [M]agic  [1]GameState [2]Reset");
}

//debug methods
void display_globals(void) {
    printf("\nplayer_name=%s, char_sleep_duration=%d, silent_mode=%d\n", GLOBALS.player_name, GLOBALS.char_sleep_duration, GLOBALS.silent_mode);
}

void display_game_state(const GameState *gs) {
    printf("\nGameState:\n");
    printf("player_name=%s, room=%d, turns=%d, cash=%d, killed=%d, fought=%d, magic=%d, "
           "has_torch=%d, is_dead=%d, completed=%d, must_fight=%d\n",
        gs->player_name->buffer, gs->room, gs->turns, gs->cash, gs->monsters_killed,gs->monsters_fought, gs->magic,
        gs->has_torch, gs->is_dead, gs->completed, gs->must_fight);
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


//// ------------------------------------------------------------
////
////    INPUT
////
//// ------------------------------------------------------------


static bool stdin_has_data(void) {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    struct pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;
    return poll(&fds, 1, 0) > 0;
#endif
}




static CharBuffer *get_player_name() {
    cls();
    CharBuffer *cb = get_char_buffer("What is your name, explorer? ");
    display("Hello, Explorer ");
    display(cb->buffer);
    display_line(".");
    display_line("Type '[H]elp' for a list of commands.");
    return cb;
}



//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------



RandomTextArray * create_rta(int length) {
    const size_t mem_size = sizeof(RandomTextArray) + sizeof(RandomText) * length;
    RandomTextArray * result = calloc(1, mem_size);
    result->length = length;
    return result;
}


// one time inits of ROOM or ROOM_GRAPH data
static void  init_rooms() {
    RandomTextArray *rta;
    // randomized text in Rooms 1, 18, 37, 39
    // room 1
    rta = create_rta(2);
    // ROOMS[1].epilog = create_rta(2);
    rta->lines[0] = (RandomText){ .chance_percent = .5, .text="There is an exit to the west."};
    rta->lines[1] = (RandomText){ .chance_percent = .5, .text="A tunnel leads to the south."};
    room_set_epilog(1, rta);

    // room 18
    rta = create_rta(1);
    rta->lines[0] = (RandomText){ .chance_percent = .5, .text="A bat flies past you, shrieking."};
    room_set_epilog(18, rta);
    // room 37
    rta = create_rta(2);
    rta->lines[0] = (RandomText){ .chance_percent = .7, .text="But now it tells you there is\na hidden stairwell in the room->"};
    rta->lines[1] = (RandomText){ .chance_percent = .3, .text="The voice faintly murmurs of the door to the south."};
    room_set_epilog(37, rta);
    // room 39
    rta = create_rta(1);
    rta->lines[0] = (RandomText){ .chance_percent = .6, .text="A small door leads to the north\nand another to the east."};
    room_set_epilog(39, rta);
}

static Object generate_treasure( GameState * gs, int treasure_index) {
    char const * name = obj_name_for_id(treasure_index);
    return (Object){
        .name = name,
        .id = treasure_index,
        .value = rnd_range(gs, 0, 100 ) + 56};
}

int sum_character_stats(const CharStats *s) {
    int total = 0;
    for (int i = 1; i < STAT_COUNT; ++i) {
        total += s->as_array[i];
    }
    return total;
}

// called at the start of each new game
void reset(GameState * gs, const uint32_t seed) {
    // reset GameState
    *gs = (GameState){ .seed = seed, .player_name = gs->player_name, .room = ROOM_START, .cash = 100, .magic = 3 };
    
    mt_initialize_state(&gs->mt_state, seed);  // initialize the PRNG
    
    gs->stats = random_hero_stats(gs);

    //clear all monsters, treasure
    const int num_rooms = room_num_rooms();
    for ( int room_index = 0; room_index < num_rooms; ++room_index ) {
        // note: if we dynamically modify the edge graph we'll need to reset those edges here
        ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
        ROOM_GRAPH[room_index][RGINDEX_MONSTER] = 0;
        room_clear_monster(room_index);
        room_remove_all_objects(room_index);
    }
    monsters_clear_all();

    // special treasure items
    ROOM_GRAPH[ROOM_START][RGINDEX_TREASURE] = ITEM_TORCH;
    ROOM_GRAPH[LIBRARY_ROOM][RGINDEX_TREASURE] = ITEM_SILVER_KEY;
    ROOM_GRAPH[GLOVE_STOREROOM][RGINDEX_TREASURE] = ITEM_GOLD_KEY;

    room_add_object(room_find_room(ROOM_START), ITEM_TORCH);
    room_add_object(room_find_room(LIBRARY_ROOM), ITEM_SILVER_KEY);
    room_add_object(room_find_room(GLOVE_STOREROOM), ITEM_GOLD_KEY);

    const int num_monsters = monsters_num_monsters();
    // allot monsters
    for (int monster_index= 1; monster_index <= num_monsters; ++monster_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, 43 + 1);
            if ( ! ( ROOM_GRAPH[rand_room][RGINDEX_MONSTER] ||
                    rand_room == ROOM_START ||
                    rand_room == ROOM_END ||
                    rand_room == LIBRARY_ROOM ||
                    rand_room == GLOVE_STOREROOM)) {
                ROOM_GRAPH[rand_room][RGINDEX_MONSTER]  = monster_index;
                CharStats stats = random_monster_stats(gs);
                int ff = sum_character_stats(&stats);
                monsters_update_monster(
                    &(Monster){
                        .name = monsters_name_for_id(monster_index),
                        .id = monster_index,
                        .ferocity_factor = ff,
                        .stats = random_monster_stats(gs)
                    });
                break;
            }
        }
    }
    // allot treasure
    const int num_objects = obj_num_objects();
    for (int treasure_index = 4; treasure_index < num_objects; ++treasure_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, 43 + 1);
            if ( ! ( ROOM_GRAPH[rand_room][RGINDEX_TREASURE] || rand_room == ROOM_START || rand_room == ROOM_END  ) ) {
                ROOM_GRAPH[rand_room][RGINDEX_TREASURE] = treasure_index;
                room_add_object(room_find_room( rand_room ), treasure_index);
                break;
            }
        }
    }

    update_perception(gs);
}

static constexpr size_t num_roomz = 48;  // todo (temp) until room data is read from file
typedef struct RoomData {
    size_t size;
    Room data[num_roomz];
} RoomData;

static RoomData get_room_data(void) {
    return (RoomData){
        .size = num_roomz,
        .data = {
            {.id =  0,  .name= "NULL ROOM",   .desc = "NULL ROOM"},
            {.id =  1,  .name= "River",       .desc = "An underground river flows swiftly by."},
            {.id =  2,  .name= "Food Store",  .desc = "You are in the Citadel's food storage area.\nOld cheeses and black loaves of bread can be seen, as well as many sacks of supplies."},
            {.id =  3,  .name= "Kitchen",     .desc = "You are in the Citadel's kitchen. A huge joint of meat turns slowly over a raging fire. Doors lead into cupboards, as well as to the west and to the south."},
            {.id =  4,  .name= "Library",     .desc = "This is the Central Library. Leather-bound volumes line the walls, right up to the ornately carved ceiling."},
            {.id =  5,  .name= "Studio",      .desc = "This room is an awful mess. It used to be an artist's studio. Paint and old easels lie around the floor."},
            {.id =  6,  .name= "Entrance",    .desc = "This is the entrance to the Citadel of Pershu.\nTurn now, if you wish. Many stronger than you have taken fright at its menacing towers and dark portals. If you wish to proceed, move east towards the black gaping doorway."},
            {.id =  7,  .name= "Altar",       .desc = "A stone altar stands in the middle of the room with two dead candles on it. An old book lies on one part of the altar top, and a faded red parchment cloth covers the front of it."},
            {.id =  8,  .name= "Black Tower", .desc = "You stand high on the black tower, the Citadel stretches to the north, south and east of you.\nThere is only one way out."},
            {.id =  9,  .name= "North Cellar",.desc = "You are in the northern section of the Citadel's large wine cellar. Heavy barrels lie all around you in this end of the cellar. There is a door to the north and one to the south."},
            {.id = 10,  .name= "West Cellar", .desc = "You are in the west wing of the wine cellar. There is a door to the west and one to the east. The central circular part of the cellar lies beyond the east door."},
            {.id = 11,  .name= "Wine Cellar", .desc = "You are in the central circular area of the wine cellar. There is a door at each compass point."},
            {.id = 12,  .name= "East Cellar", .desc = "You are in the east section of the wine cellar. There is a door to the west and one - which you cannot use,as it only allows entrance to where you now stand - to the east."},
            {.id = 13,  .name= "South Cellar",.desc = "There are many, many wine bottles here lying on their sides in this southern section of the wine cellar. There is a dark, unfriendly-looking hole to the west and doors to the north and to the south."},
            {.id = 14,  .name= "Armory",      .desc = "This is the Citadel's armory. Row upon row of shiny suits of armor are stored here." },
            {.id = 15,  .name= "Bedchamber",    .desc = "You are in the ruler's bedchamber.\nA large fire burns in the south of the room, with a small door beside it. Other exits are to the north and to the west." },
            {.id = 16,  .name= "Sand",          .desc = "Sand covers the floor of this curious room, heaped into drifts.\nBy peeping over the 'dunes' you can see a golden passage way leads to the west, and there is a door to the south. You are not sure whether or not you have seen all the exits." },
            {.id = 17,  .name= "Gallery",       .desc = "You are in the picture gallery. Portraits of long-dead princes line all of the walls. The room is dominated by a huge landscape, hanging above the exit to the east which leads, via the gold passage way back to that curious room of sand." },
            {.id = 18,  .name= "Tower Balcony", .desc = "You are on a remote tower balcony.\nThere are stairs here." },
            {.id = 19,  .name= "Archway",       .desc = "You walk beneath a stone archway.\nYou can only walk north or south unless you decide to take the stairs." },
            {.id = 20,  .name= "Marble Hall",       .desc = "This vast hall has a marble floor, and the slightest sound echos violently.\nThere are purple drapes concealing the exits from this hall." },
            {.id = 21,  .name= "Glove Storeroom",   .desc = "You are in the glove storeroom->\nThe west door radiates heat.\nAnother door leads to the south." },
            {.id = 22,  .name= "Silver Storeroom",  .desc = "You are in the silver crosses storeroom->\nThere are only two exits." },
            {.id = 23,  .name= "Amulet Storeroom",  .desc = "You are in the amulet storeroom->\nDoors lead north and south." },
            {.id = 24,  .name= "Kazoom Storeroom",  .desc = "You are in the kazoo storeroom->\nThere are two exits." },
            {.id = 25,  .name= "Satchel Storeroom", .desc = "You are in the satchel storeroom->" },
            {.id = 26,  .name= "Wooden Storeroom",  .desc = "You are in the storeroom for wooden boxes... There are two exits." },
            {.id = 27,  .name= "Vase Storage",      .desc = "This is where printed vases are stored... As you can easily see." },
            {.id = 28,  .name= "Mine",              .desc = "The heavy air of this area seems to make your torch very dim-> You can hardly see that air is rushing up from somewhere.\nYou can just make out that this area must be a mine of some sort." },
            {.id = 29,  .name= "Tunnels", .desc = "You appear to be in an endless labyrinth,lined with paintings.........\nWhichever way you turn, there seems to be more tunnels, all lined with paintings." },
            {.id = 30,  .name= "South Tower", .desc = "This is the southern tower of the Citadel." },
            {.id = 31,  .name= "Exit", .desc = "Well done, you have managed to find the exit.\nTake a deep breath of good, clean air..........." },
            {.id = 32,  .name= "Meditation Room", .desc = "This room is filled with swirling smoke,so you cannot see... Air rushes past a statue of the goddess Diana. This must be the Citadel's meditation chamber." },
            {.id = 33,  .name= "Bridge", .desc = "A small forked bridge crosses a stream here.\nYou can move north, south, or west." },
            {.id = 34,  .name= "Cavern", .desc = "You are in a rough stone cavern. Stairs lead up from here.\nThere is also a single door which leads away from the cavern." },
            {.id = 35,  .name= "Stable", .desc = "This is the former Citadel underground stable. It smells terrible." },
            {.id = 36,  .name= "Courtyard", .desc = "You find yourself in an underground courtyard. Strange, twisted trees are around you, and a wind of incredible coldness blows from the east." },
            {.id = 37,  .name= "Oracle", .desc = "This is the Oracle Room, although the mystic voice has not spoken for many years." },
            {.id = 38,  .name= "Sacrifice Room", .desc = "Horrors. A cold shudder passes through you as you realize this is the priests' sacrifice room->\nDried-up blood is on the floor and a skull grins at you from high on the wall." },
            {.id = 39,  .name= "Dungeon", .desc = "Old straw mattresses and rings chained to the wall tell you this was the Citadel's dungeon. The dungeon seems to stretch forever, with many small partitioned areas...." },
            {.id = 40,  .name= "Alcove",  .desc = "You are in a small alcove, with a solid gray granite throne in the middle of it." },
            {.id = 41,  .name= "Orc Guardroom", .desc = "This is the orc's guardroom, way below the ground. A stairwell ends here and a door leads to the east." },
            {.id = 42,  .name= "Healing Pool",  .desc = "There is a healing pool here, with a dangerous, swirling area of water." },
            {.id = 43,  .name= "Hall of Odric", .desc = "The Underpriests of Odric used this tiny hall for their forbidden worship eons ago. It is an unpleasant area,so you are thrilled to see a set of stone stairs." },

            {.id = 44,  .name= "DEATH BY DROWNING",  .desc = "Water covers your head.\nYou are drowning.\nGLUG... GASP............" },
            {.id = 45,  .name= "DEATH BY BURNING",  .desc = "The flames strike at you...\nas you slowly burn to death..."},
            {.id = 46,  .name= "DEATH BY FREEZING", .desc = "You are hit by a freezing spell and turn into a block of perpetual living stone. This is the end."},
            {.id = 47,  .name= "BOTTOMLESS PIT",    .desc = "You tumble down a bottomless pit.\nDown, down, down..."},

        }
    };
}

constexpr size_t num_objectz = 20;
typedef struct ObjectData {
    size_t size;
    Object data[num_objectz];
} ObjectData;


// first 9 elements are items the user can use, carry, or drop (and pick up again.)
// From Emeralds and higher, these are treasure that are converted to a cash equivalent
static ObjectData get_object_data(void) {
    return (ObjectData){
        .size = num_objectz,
        .data = {
            { .id =  1, .name = "Flaming Torch",         } ,
            { .id =  2, .name = "Silver Key",            },
            { .id =  3, .name = "Gold Key",              },
            { .id =  4, .name = "Sword",                 },
            { .id =  5, .name = "War Hammer",            },
            { .id =  6, .name = "Chain Mail Armor",      },
            { .id =  7, .name = "Shield",                },
            { .id =  8, .name = "Cloak of Protection",   },
            { .id =  9, .name = "Wand of Fireballs",     },
            { .id = 10, .name = "Emeralds",              .value =  99 },
            { .id = 11, .name = "Silver Rings",          .value = 247 },
            { .id = 12, .name = "Elven Amethysts",       .value = 166 },
            { .id = 13, .name = "Diamond Dragon Eyes",   .value = 462 },
            { .id = 14, .name = "Crystal Ball",          .value = 195 },
            { .id = 15, .name = "Pieces of Eight",       .value = 231 },
            { .id = 16, .name = "Elemental Gems",        .value = 162 },
            { .id = 17, .name = "Shape-Shifting Stones", .value =  27 },
            { .id = 18, .name = "Gold Doubloons",        .value = 141 },
        }
    };
}

// once time inits. Per-game inits happen in reset()
void initialize() {
    // note: random data is initialized in reset()
    RoomData rd = get_room_data();
    room_init(rd.size,rd.data);
    
    monsters_init("monsters.txt");
    
    ObjectData od = get_object_data();
    obj_init(od.size, od.data);
    
    init_rooms();
}




//// ------------------------------------------------------------
////
////    CLEANUP
////
//// ------------------------------------------------------------


static void cleanup(GameState * gs) {
    room_destroy();
    void *free_ptr = (void *) gs->player_name;
    gs->player_name = nullptr;
    free( free_ptr);
    monsters_destroy();
    obj_destroy();
}





//// ------------------------------------------------------------
////
////    GAME FUNCTIONS
////
//// ------------------------------------------------------------


static int count_items_carried(const GameState * gs) {
    int result = 0;
    for (int bag_index = 1; bag_index < ITEM_COUNT; ++bag_index ) {
        if (! gs->items[bag_index] ) {
            result++;
        }
    }
    return result;
}

// return true if carrying any items
static bool actor_has_any_items(const GameState * gs) {
    for (int bag_index = 1; bag_index < ITEM_COUNT; ++bag_index ) {
        if (! gs->items[bag_index] ) {
            return true;
        }
    }
    return false;
}

static int calc_score(const GameState * gs) {
    int sum_attributes = gs->stats.strength + gs->stats.charisma + gs->stats.dexterity +
        gs->stats.intelligence + gs->stats.wisdom + gs->stats.constitution;
    return 3 * gs->cash +  30 * gs->monsters_killed + 3 * sum_attributes + gs->turns  ;
}

static bool cmd_quit(const GameState * gs) {
    display_line("COWARD...QUITTER....TURNCOAT.....");
    // todo (rob) ask for confirmation?
    return END_GAME;
}




// first_letter must be in "NSEWUD"
// return true if command was successfully processed. If false, the move is not allowed and an error message
// will have been displayed
static bool cmd_move(GameState * gs, char const first_letter) {
    const int location = gs->room;
    const int direction_index = calc_room_graph_direction_index(first_letter);
    if (direction_index == DIRECTION_ERR) {
        display("Bad direction_index, first_letter='");
        printf("%c'\n", first_letter);
        return false;
    }
    gs->room_prev = gs->room;

    if (ROOM_GRAPH[location][direction_index] > 0) {
        gs->room = ROOM_GRAPH[location][direction_index];
        return true;
    }

    display_line(BAD_MOVE_DESC[direction_index]);
    return false;
}

static bool pick_up_treasure(GameState * gs) {
    const int treasure_index = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    if ( !gs->has_torch && treasure_index != ITEM_TORCH ) {
        display_line("It is too dark to see anything.");
        return false;
    }

    if (!treasure_index) {
        display_line("There is nothing to pick up.");
        return false;
    }

    if ( treasure_index == ITEM_TORCH ) {
        gs->has_torch = true;
    }

    const Room *room = room_find_room(gs->room);

    if (treasure_index > ITEM_WAND) {
        const Object treasure = room->treasure;
        gs->cash += treasure.value;
    } else {
        gs->items[treasure_index] = treasure_index;
    }

    room_remove_object(room, treasure_index);
    display_line("Taken.");
    return true;
}

// clear the monster in the current room and its entry in the ROOM_GRAPH array
static void clear_monster(const GameState * gs) {
    room_clear_monster(gs->room);
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
}

/**
 * Shared Validation: Can an item be dropped here?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_drop_item(const GameState *gs, int item_index, bool verbose) {
    if (!actor_has_any_items(gs)) {
        if (verbose) display_line("You have nothing to get rid of.");
        return false;
    }
    if (ROOM_GRAPH[gs->room][RGINDEX_TREASURE]) {
        if (verbose) {
            vdisplay_line( "There is already a %s here.",  obj_name_for_id(gs->room));
        }
        return false;
    }
    if (item_index == 0) return true; // Cancel/No-op is valid coice
    if (item_index < 0 || item_index >= ITEM_COUNT || !gs->items[item_index]) {
        if (verbose) display_line("You are not carrying that item->");
        return false;
    }
    return true;
}

// Entry point for human user path. This displays some information, prompts user for some choices, and passes those to
// drop_action(), the ML entry point for the drop action.
static bool get_rid_of(GameState * gs) {
    // Pre-check: If the room is already full, don't even start the loop
    if (!can_drop_item(gs, 0, true)) return false;

    int item = 0;
    for (;;) {
        actor_display_inventory(gs);
        item = get_int("Enter number of object to drop (0 for none): ", 0, 9);
        if ( !item ) {
            return true;  // exit without dropping anything
        }

        if (gs->items[item]) {
            break;
        }
        display_line("You are not carrying that item->");
    }
    return perform_action(gs, 'G', item, 0, 0);
}



/** Logic Entry Point: ML and Human both end up here */
bool action_drop(GameState *gs, int item_index) {
    // Perform the check (protects against ML typos)
    if (!can_drop_item(gs, item_index, !GLOBALS.silent_mode)) {
        return false;
    }

    const Room *r = room_find_room(gs->room);
    if (item_index == 0) return true; // successful no-op
    object_id id = gs->items[item_index];
    room_add_object( r, id );

    gs->items[item_index] = 0;
    ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = item_index;

    if (item_index == ITEM_TORCH) {
        gs->has_torch = false;
    }

    return true;

}

//todo (rob) make `strategy` an enum
//return false if fight action could not be completed, otherwise return true
bool action_fight(GameState * gs, int strategy, enum StatIndex stat1, enum StatIndex stat2) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        return false;  // nothing to fight
    }    
    const Room *r = room_find_room(gs->room);
    Monster *m = monsters_find_monster(r->monster);
    
    if (strategy == 1 && gs->magic == 0 ) {
        // not enough magic
        //todo (rob) - create return code for this case and others.
        return false;
    } 
    
    gs->monsters_fought++;
    gs->must_fight = false;
    
    if (strategy == 1) {
        display_line("Your magic destroys it!");
        gs->magic--;
        gs->monsters_killed++;
        clear_monster(gs);
        return true;
    }
    
    int hero_tally = 0;
    int monster_tally = 0;
    
    // calc enhancements due to fighting items
    for (enum Item item = ITEM_SWORD; item <= ITEM_WAND; ++item ) {
        if (gs->items[item]) {
            hero_tally++;
        }
    }
    
    hero_tally += gs->stats.as_array[stat1];
    hero_tally += gs->stats.as_array[stat2];
    monster_tally += m->stats.as_array[stat1];
    monster_tally += m->stats.as_array[stat2];

    if (!gs->has_torch) {
        hero_tally -= 5;  // harder to see in the dark
    }
    
    if (!GLOBALS.silent_mode) {
        display("\nThe fight starts in favor of ");
        if (hero_tally > monster_tally ) {
            display_line("you.");
        } else {
            display_line(m->name);
        }
    
        display("The ");
        display(m->name);
        display(" - ");
        printf("%d\n",monster_tally);
        display(gs->player_name->buffer);
        display(" - ");
        printf("%d\n",hero_tally);
    }
    
       for (;;) {
        int attack = rnd_range(gs, 0,8 );
        switch (attack) {
            case 0: {
                display_line("You get in a glancing blow");
                monster_tally--;
            } break;
            case 1: {
                display("The ");
                display(m->name);
                display_line(" strikes out!");
                hero_tally -= 3;
                gs->stats.strength--;
                gs->stats.charisma--;
            } break;
            case 2: {
                display("You draw the ");
                display(m->name);
                display_line("'s blood!");
                monster_tally--;
            } break;
            case 3: {
                display_line("You are wounded!!");
                hero_tally -= rnd_range(gs, 1, 4);
                gs->dexterity--;  // annonymous union lets us do this!
            } break;
            case 4: {
                display("The ");
                display(m->name);
                display_line(" is tiring.");
                monster_tally--;
            } break;
            case 5: {
                display_line("You are bleeding....");
                hero_tally -= 2;
                gs->stats.wisdom--;
                gs->stats.constitution--;
            } break;
            case 6: {
                display("You wound the ");
                display(m->name);
                display_line("");
                monster_tally--;
            } break;
            case 7:
            default: {
                const int lost_cash = rnd_range(gs, 1, gs->cash/2 + 1);
                if (!GLOBALS.silent_mode) {
                    display("It knocks $");
                    printf("%d from your hand.\n",lost_cash);
                }
                gs->cash -= lost_cash;
            } break;
        }

        if (! (hero_tally > 0 && monster_tally > 0 && rnd_d(gs) < .75 ) ) {
            break;
        }
    }

    if (hero_tally > monster_tally ) {
        display_line("You bested the beast!");
        gs->monsters_killed++;
    } else {
        display("The ");
        display(m->name);
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
            gs->stats.constitution = 3 *  gs->stats.constitution / 6;
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

// Entry point for human user path. This displays some information, prompts user for some choices, and passes those to
// perform_action(), the ML entry point for the fight action.
static bool cmd_fight(GameState * gs) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        display_line("There is nothing to fight.");
        return false;
    }
    const Room *r = room_find_room(gs->room);
    Monster *m = monsters_find_monster(r->monster);

    if (gs->has_torch) {
        display("\nYour opponent is a ");
        display_line(m->name);
        display_line("With the following attributes:");
        display_char_attributes(m->stats);
    }
        display_line("\nYour attributes are:");
        display_char_attributes(gs->stats);


    if (gs->items[ITEM_SWORD]) {
        display_line("You have a sword");
    }
    if (gs->items[ITEM_WAR_HAMMER]) {
        display_line("Your War Hammer will be of aid");
    }
    if (gs->items[ITEM_CHAIN_MAIL]) {
        display_line("Chainmail armor gives you an edge");
    }
    if (gs->items[ITEM_SHIELD]) {
        display("Your shield will help you in this fight against the ");
        display_line(m->name);
    }
    if (gs->items[ITEM_CLOAK]) {
        display_line("The Cloak of Protection surrounds you");
    }
    if (gs->items[ITEM_WAND]) {
        display_line("The Wand of Fireballs enhances your strength");
    }

    if (gs->magic) {
        int choice = get_int("Enter 1 to fight with magic or 2 to rely on skill: ", 1, 2);
        if (choice == 1) {
            return perform_action(gs, 'F', 1, 0, 0);
        }
    }

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
    return perform_action(gs, 'F', 2, first_skill, second_skill);
}

static bool process_retreat(GameState * gs) {
    const int room = gs->room;
    if (!ROOM_GRAPH[room][RGINDEX_MONSTER]) {
        display_line("There is nothing to retreat from->");
        return false;
    }

    display_line("RUN AWAY!!!!!!!");
    // determine possible exits
    int num_exits = 0;
    int exits[RGINDEX_DOWN + 1] = {};
    for (int exit_index = RGINDEX_NORTH; exit_index <= RGINDEX_DOWN; ++exit_index ) {
        const int room_index = ROOM_GRAPH[room][exit_index];
        if ( room_index ) {
            //todo retreat through unlocked doors should be ok, but not through locked doors
            if ( !( room_index == ROOM_END || room_index ==  WINE_CELLAR_EAST || room_index == MARBLE_HALL) ) {
                // don't retreat to end room or through locked doors
                exits[num_exits++] = room_index;
            }
        }
    }

    // randomly move to an adjacent room-> If current room has paths to itself, new room may not change
    int retreat_index = rnd_range(gs, 0, num_exits);

    if ( rnd_d(gs) < .6 || num_exits == 0 || retreat_index == room) {
        display_line("The creature blocks your path. You must fight.");
        gs->must_fight = true;
        return false;
    }

    gs->room = exits[retreat_index];
    return true;
}

/**
 * Helper to check if a specific command character is currently legal 
 * based on the game rules and current room state.
 */
static bool is_action_legal(const GameState *gs, char c) {
    const char cmd = (char)toupper(c);
    const int room_index = gs->room;
    const int monster_index = ROOM_GRAPH[room_index][RGINDEX_MONSTER];
    const int treasure_index = ROOM_GRAPH[room_index][RGINDEX_TREASURE];

    // 1. Basic monster check
    if (monster_index > 0) {
        if (gs->must_fight && cmd != 'F') return false;
        if (cmd != 'F' && cmd != 'R') return false;
    }
    // 2. Directional check
    int dir_idx = calc_room_graph_direction_index(cmd);
    if (dir_idx != DIRECTION_ERR) {
        return ROOM_GRAPH[room_index][dir_idx] > 0;
    }
    // 3. Item check for 'P' (Pick up)
    if (cmd == 'P' && treasure_index == 0) return false;

    return true;
}

static void update_perception(GameState * gs) {
    const int room_index     = gs->room;
    const int monster_index  = ROOM_GRAPH[room_index][RGINDEX_MONSTER];
    const int treasure_index = ROOM_GRAPH[room_index][RGINDEX_TREASURE];

    // reset perceptions
    gs->perception.monster_is_visible  = false;
    gs->perception.treasure_is_visible = false;
    gs->perception.current_monster  = (Monster){};
    gs->perception.current_treasure = (Object){};
    gs->perception.legal_actions_mask = 0;

    // Populate Action Mask for ML
    for (int i = 0; VALID_COMMANDS[i] != '\0'; ++i) {
        if (is_action_legal(gs, VALID_COMMANDS[i])) {
            gs->perception.legal_actions_mask |= (1u << i);
        }
    }

    const Room *r = room_find_room(gs->room);

    // Only see things if the room is lit
    // Note: Object index 1 is the Torch itself, which is visible in the dark.
    if (gs->has_torch || treasure_index == ITEM_TORCH ) {
        if (monster_index > 0 ) {
            gs->perception.current_monster = *monsters_find_monster(monster_index);
            gs->perception.monster_is_visible  = true;
        }
        if (treasure_index > 0 ) {
            gs->perception.current_treasure = *obj_find_object(treasure_index);
            gs->perception.treasure_is_visible  = true;
        }
    }

}

// checks if there is a monster and if so, that the user has selected either F or R. Returns true for success.
bool monster_check(const GameState * gs, const char cmd) {
    const bool has_monster = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    // Rule: Must deal with monsters first. Recognized command, but logic fails.
    if ( has_monster && gs->must_fight && cmd != 'F') {
        display_line("DANGER! You can only FIGHT!");
        return false;
    }
    if (has_monster && cmd != 'F' && cmd != 'R') {
        display_line("DANGER! You must either FIGHT or RETREAT.");
        return false;
    }

    return true;
}

/**
  * Core Game Engine Logic
  * This function is "Pure Logic" - it updates state based on an action.
  * It returns true if the action was accepted as a turn, false otherwise.
  *
  * @param gs
  * @param action
  * @param arg1 For 'F': strategy (1:magic, 2:skill). For 'G': item index.
  * @param arg2 For 'F': first skill stat index.
  * @param arg3 For 'F': second skill stat index.
  *
  *
  */
bool perform_action(GameState *gs, char action, int arg1, int arg2, int arg3) {
    const char cmd = (char)toupper(action);

    if (!strchr(VALID_COMMANDS, cmd)) {
        return false; // Unknown command: Not a turn, no state change.
    }

    gs->turns++;

    if (gs->room == MARBLE_HALL ) {
        // if player is here, they already used the key to unlock the west door
        gs->items[ITEM_GOLD_KEY] = 0;
    }
    if (gs->room == WINE_CELLAR_EAST ) {
        gs->items[ITEM_SILVER_KEY] = 0;
    }

    if ( !monster_check(gs, cmd) ) {
        return false;
    }

    if (strchr(VALID_DIRECTIONS, cmd)) {
        // Special logic for locked doors
        if (gs->room == BEDCHAMBER_ROOM && cmd == 'W' && !gs->items[ITEM_SILVER_KEY]) {
            display_line("You need the Silver Key to unlock the door.");
            return false;
        }
        if (gs->room == SILVER_CROSSES_STOREROOM && cmd == 'W' && !gs->items[ITEM_GOLD_KEY]) {
            display_line("You need the Gold Key to unlock the door.");
            return false;
        }

        // We must update perception after the move is processed but before returning
        // so the new room's content is visible in the GameState.
        const bool result = cmd_move(gs, cmd);
        update_perception(gs);  // room may have changed
        return result;
    }

    bool result = false;
    switch (cmd) {
        case 'P':
            result = pick_up_treasure(gs);
            break;
        case 'F':
            result = action_fight(gs, arg1, (enum StatIndex)arg2, (enum StatIndex)arg3);
            break;
        case 'R':
            result =  process_retreat(gs);
            break;
        case 'G':
            result =  action_drop(gs, arg1);
            break;
        case 'H':
            display_help_info();
            result = true;
            break;
        case 'I':
            display_inventory(gs);
            result = true;
            break;
        case 'A':
            display_char_attributes(gs->stats);
            result = true;
            break;
        case 'T':
            // These are valid turns, but have no state-solving logic for ML.
            display_score(gs);
            result = true;
            break;
        case 'Q':
            gs->completed = true; // Signal the engine to stop
            result = true;
            break;
        default:
            // Unknown action
            result = false;
            break;
    }

    // Single point of truth. Update perception after any turn-based action.
    update_perception(gs);

    return result;
}

/**
 * Death and Win condition check
 * RETURNS: true if the game is over (win or loss).
 * The caller should check gs->is_dead or gs->completed to see the outcome.
 */
bool check_game_over(GameState *gs) {
    if (gs->completed) return true;

    if (gs->room == ROOM_END || gs->room >= DROWNING_ROOM) {
        if (gs->room >= DROWNING_ROOM) gs->is_dead = true;
        gs->completed = true;
        return true;
    }

    for (int i = STAT_STRENGTH; i < STAT_COUNT; ++i) {
        if (gs->stats.as_array[i] <= 0) {
            if (!GLOBALS.silent_mode) {
                display_char_attributes(gs->stats);
                display_line("\nYour combined attributes are no longer\nenough to sustain you... You are dead.");
            }
            gs->is_dead = true;
            gs->completed = true;
            return true;
        }
    }
    return false;
}

bool DEBUG = true;
uint32_t DEBUG_NORMAL_SLEEP = 0;
uint32_t DEBUG_VISITED_SLEEP = 0;

static bool main_game_loop(GameState * gs) {
    uint32_t saved_sleep_duration = GLOBAL_char_sleep_duration;
    const room_id room_id = gs->room;
    const Room *current_room = room_find_room(room_id);

    if (current_room->is_visited_bit) {
        // if we've already seen this room, speed up output display
        if (DEBUG) {
            set_char_sleep(DEBUG_VISITED_SLEEP);
        } else {
            set_char_sleep(1'000); // 1ms
        }
    }

    // printf("---------------------------------------------------------------------- %d\n", gs->turns);
    vdisplay_line("%s ----------", current_room->name);
    // display_status(gs);

    if (gs->room != gs->room_last_turn) {
        // only display room desc once when first entering room. Reduces screen clutter and scrolling.
        // user can always type "look" to re-display room desc.
        display_room_desc(gs);
    }

    if (check_game_over(gs)){
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return END_GAME;
    }

    display_room_content(gs);

    if (room_id == ROOM_START && gs->room_prev == 0 ) {
        // first room, we display initial inventory. Afterward, the user can view them with an explicit "inv" command
        display_line("");
        display_inventory(gs);
    }

    flush_input();
    char cmd = get_command_char("\n> ", VALID_COMMANDS, nullptr);

    //todo (rob) debug code
    if (cmd == 'L') {
        display_globals();
    }
    if (cmd == 'M') {
        gs->magic = 50;
    }
    if (cmd == '1') {
        display_game_state(gs);
    }
    if (cmd == '2') {
        reset(gs, DEBUG_RAND_SEED);
    }

    if (cmd == 'Q') {
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);

        return cmd_quit(gs);
    }

    if ( !monster_check(gs, cmd) ) {
        room_set_visited_flag(current_room);
        return true;
    }

    if ( cmd == 'F' ) {
        //specialized code to prompt user and gather options to pass to perform_action()
        cmd_fight(gs);
    } else if (cmd == 'G'){
        get_rid_of(gs);
    } else {
        // Now the human call and the ML call use the exact same entry point
        perform_action(gs, cmd, 0,0, 0);
    }

    set_char_sleep(saved_sleep_duration);

    display_line("");
    if (room_id == gs->room) {
        // if room at end of turn is same as start of turn, update this so we don't display the room desc again
        gs->room_last_turn = room_id;
    } else {
        gs->room_last_turn = gs->room_prev;
    }

    room_set_visited_flag(current_room);
    return CONTINUE_GAME;
}

//// ------------------------------------------------------------
////
////    MAIN
////
//// ------------------------------------------------------------



int main_citadel_of_pershu(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    set_silent_mode(false);

    if (DEBUG) {
        set_char_sleep(DEBUG_NORMAL_SLEEP);
    } else {
        set_char_sleep(10'000);
    }

    const CharBuffer *player_name = get_player_name();
    GameState gs = {.player_name = player_name};

    initialize();
    reset(&gs, DEBUG_RAND_SEED);
    display_line("Your attributes are:");
    display_char_attributes(gs.stats);
    display_line("");

    // obj_repr();
    // monsters_names_repr();
    // room_rooms_repr();

    bool continue_loop;
    do {
        continue_loop = main_game_loop(&gs);
    } while (continue_loop);


    display_conclusion(&gs);
    display_score(&gs);
    cleanup(&gs);

    return EXIT_SUCCESS;
}

// main() is defined when running this TU stand-alone and including -DCITADEL_OF_PERSHU_MAIN compiler flag.
#ifdef CITADEL_OF_PERSHU_MAIN


int main(void) {
    return main_citadel_of_pershu();
}
#endif