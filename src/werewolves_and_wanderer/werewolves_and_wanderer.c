// werewolves_and_wanderer.c
//
// Port of BASIC game from "Creating Adventure Games on Your Computer,"
// by Tim Hartnell, 1983
//
//
// Created 2026/05/01 18:00:08 PDT


/*

MAKE :

cd /Users/robross/Documents/Development/CLionProjects/werewolves_and_wanderer/text_adventures/src

 * DEBUG *

clang -g -DWEREWOLVES_AND_WANDERER_MAIN -fsanitize=address -fsanitize=leak -Wall -Werror \
    -Wno-unused-const-variable -Wno-unused-variable -Wno-unused-function \
    -std=c23 -o werewolves_and_wanderer.out werewolves_and_wanderer.c  \
            ../adventure_shared.c           \
            ../mersenne_twister.c           \
            ../common/console_utils.c       \
            ../parser.c                     \
            ../rooms.c                      \
            ../objects.c                    \
            ../monsters.c                   \
            ../common/string.c              \
            ../roblib/string/string_utils.c \
            ../roblib/string/string_builder.c \
            ../roblib/json_parser/json_parser.c \
            ../roblib/json_parser/arena.c   \
            ../roblib/json_parser/error_result.c

 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/adventure_shared.h"
#include "../common/directions.h"
#include "../common/console_utils.h"
#include "../common/mersenne_twister.h"
#include "../common/rooms.h"
#include "../common/monsters.h"
#include "../common/monster_loader.h"
#include "../common/objects.h"


enum Item {
    ITEM_DUMMY,
    ITEM_LIGHT,
    ITEM_AXE,
    ITEM_SWORD,
    ITEM_FOOD,
    ITEM_AMULET,
    ITEM_SUIT,
    ITEM_COUNT
};

constexpr int ROOM_START          =  6;
constexpr int ROOM_END            = 11;
constexpr int ROOM_LIFT           =  9;
constexpr int ROOM_REAR_VESTIBULE = 10;

const int MAX_ROOM_OBJECTS = 1; //maximum number of items that can be placed in a room
const int MAX_PLAYER_OBJECTS = 6; // max number of items that can be carried

int ROOM_GRAPH[][RGINDEX_COUNT] = {
    { 0,  0,  0,  0,  0,  0,  0}, // Room 0
    { 0,  2,  0,  0,  0,  0,  0}, // Room 1
    { 1,  3,  3,  0,  0,  0,  0}, // Room 2
    { 2,  0,  5,  2,  0,  0,  0}, // Room 3
    { 0,  5,  0,  0,  0,  0,  0}, // Room 4
    { 4,  0,  0,  3, 15, 13,  0}, // Room 5
    { 0,  0,  1,  0,  0,  0,  0}, // Room 6
    { 0,  8,  0,  0,  0,  0,  0}, // Room 7
    { 7, 10,  0,  0,  0,  0,  0}, // Room 8
    { 0, 19,  0,  8,  0,  8,  0}, // Room 9
    { 8,  0, 11,  0,  0,  0,  0}, // Room 10
    { 0,  0, 10,  0,  0,  0,  0}, // Room 11
    { 0,  0,  0, 13,  0,  0,  0}, // Room 12
    { 0,  0, 12,  0,  5,  0,  0}, // Room 13
    { 0, 15, 17,  0,  0,  0,  0}, // Room 14
    {14,  0,  0,  0,  0,  5,  0}, // Room 15
    {17,  0, 19,  0,  0,  0,  0}, // Room 16
    {18, 16,  0, 14,  0,  0,  0}, // Room 17
    { 0, 17,  0,  0,  0,  0,  0}, // Room 18
    { 9,  0, 16,  0,  0,  0,  0}, // Room 19
};

constexpr int DEBUG_RAND_SEED = 67;
struct GlobalState GLOBALS = {
    .player_name = nullptr,
    .silent_mode = false,
    .char_sleep_duration = _10ms,
    .char_sleep_visited_duration = _1ms,
    .debug_normal_sleep = 0,
    .debug_visited_sleep = 0,
    .debug_mode = false,
};

// -----------------------------------------------------------------
//      Forward Declarations
// -----------------------------------------------------------------
static bool main_game_loop(GameState * gs);
static void cmd_look_custom( GameState *gs);



static int calc_score(const GameState * gs) {
    const int score = 3* gs->turns + 5* gs->strength + 2* gs->cash + gs->food + 30*gs->monsters_killed;
    return score;
}

static void display_score(const GameState * gs) {
    display_linef("\nSCORE: %d", calc_score(gs));
    display_linef("\nturns: %d, strength: %d, cash: %d, food: %d, monsters fought: %d, killed: %d",
        gs->turns, gs->strength, gs->cash, gs->food, gs->monsters_fought, gs->monsters_killed);}


static void display_status(const GameState * gs) {
    if (GLOBALS.silent_mode ) return;
    if (gs->cash > 0) {
        display_linef("YOU HAVE $%d WEALTH", gs->cash);
    }

    if (gs->food > 0 ) {
        display_linef("YOUR PROVISIONS SACK HOLDS %d UNITS OF FOOD", gs->food);
    }
}

static void display_inventory(const GameState * gs) {
    display_status(gs);
    actor_display_inventory(gs, false, false);
}


// first_letter must be in "NSEWUD"
// return true if command was successfully processed. If false, the move is not allowed and an error message
// will have been displayed
static bool cmd_move(GameState * gs, char const cmd_char) {
    const int location = gs->room;
    const int direction_index = calc_room_graph_direction_index(cmd_char);
    if (direction_index == DIRECTION_ERR) {
        display_linef("Bad direction_index, first_letter='%c'", cmd_char);
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


static void display_inventory_menu( GameState * gs) {
    // we use printf here and not display to speed up the menu output
    printf("\nYOU HAVE $%d\n", gs->cash);

    printf("YOU CAN BUY 1 - FLAMING TORCH ($15)\n");
    printf("            2 - AXE ($10)\n");
    printf("            3 - SWORD ($20)\n");
    printf("            4 - FOOD($2 PER UNIT)\n");
    printf("            5 - MAGIC AMULET ($30)\n");
    printf("            6 - SUIT OF ARMOR ($50)\n");
    printf("            0 - TO CONTINUE ADVENTURE\n");
}

static char const * const VALID_COMMANDS = "QNSEWUDRFIBCTMHAOL123";

static void display_help_info(void) {
    if (GLOBALS.silent_mode) return;

    display_line("\nVALID COMMANDS ARE:\n");

    display_line("[H]elp       [I]nventory  [Q]uit" );
    display_line("[A]ttributes Sc[o]re      [L]ook" );
    display_line("[R]etreat    [F]ight      [C]onsume" );
    display_line("[T]ake       [B]uy        [M]agic Amulet");
    display_line("[N]orth      [S]outh");
    display_line("[E]ast       [W]est");
    display_line("[U]p         [D]own");

    display_line("\nDEBUG:");
    display_line("[1]Globals  [2]GameState [3]Reset  [M]agic");
}



static bool cmd_buy( GameState * gs) {
    display_line("PROVISIONS AND INVENTORY");
    if (gs->cash <=0 ) {
        display_line("YOU HAVE NO MONEY.");
        return false;
    }

    bool bought_torch = false;

    for (;;) {
        display_inventory_menu(gs);

        char option;
        fflush(stdin);
        do {
            display("ENTER NO. OF ITEM REQUIRED ");
            option = (char)getchar();
            fflush(stdin);
        } while ( !(option >= '0' && option <= '6') );

        const int option_index = option - '0';

        if ( option_index == 0 ) {
            //cls
            break;
        }

        const Object *o = obj_find_object(option_index);
        if (!o) {
            display_linef("got null Object for id=%d", option_index);
            return false;
        }


        if ( option_index != 4 ) {
            if (gs->cash < o->value) {
                display_line("YOU HAVE TRIED TO CHEAT ME!");
                //punish user
                gs->cash = 0;
                actor_remove_all_objects(gs);
                gs->has_torch = false;
                gs->food = gs->food / 4 ;
                return false;
            }
            gs->cash -= o->value;
            actor_add_object(gs, option_index);
            if (option_index == ITEM_LIGHT) {
                gs->has_torch = true;
                bought_torch = true;
            }
        }

        if (option_index == 4 ) {
            for (;;) {
                char food_quantity;
                fflush(stdin);
                do {
                    display("HOW MANY UNITS OF FOOD (0-9)? ");
                    food_quantity = (char)getchar();
                    fflush(stdin);
                } while ( !(food_quantity >= '0' && food_quantity <= '9') );

                const int qty = food_quantity - '0';
                int cost = qty * o->value;
                if (gs->cash - cost < 0 ) {
                    display_line("YOU HAVEN'T GOT ENOUGH MONEY!");
                } else {
                    gs->cash -= cost;
                    gs->food += qty;
                    break;
                }
            }
        }
    }
    if (bought_torch) {
        display_line("");
        cmd_look_custom(gs); // user can now see the room
    }
    return true;
}


static bool cmd_eat( GameState * gs) {
    if (gs->food <= 0) {
        display_line("YOU HAVE NO FOOD.");
        return false;
    }
    for (;;) {
        char food_quantity;
        fflush(stdin);
        do {
            display_linef("YOU HAVE %d UNITS OF FOOD.", gs->food);
            display("HOW MANY DO YOU WANT TO EAT (0-9)? ");

            food_quantity = (char)getchar();
            fflush(stdin);
        } while ( !(food_quantity >= '0' && food_quantity <= '9') );

        const int qty = food_quantity - '0';
        if ( qty <= gs->food) {
            gs->food -= qty;
            gs->strength += (5 * qty);
            break;
        } else {
            display_line("YOU DON'T HAVE THAT MUCH FOOD.");
        }
    }
    return true;
}

static bool cmd_take(GameState * gs) {
    if ( ROOM_GRAPH[gs->room][RGINDEX_TREASURE] <= 0 ) {
        display_line("THERE IS NO TREASURE TO PICK UP.");
        return false;
    }
    if ( !gs->has_torch ) {
        display_line("YOU CANNOT SEE WHERE IT IS.");
        return false;
    }
    gs->cash += ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = 0;
    display_line("TAKEN.");
    return true;
}

static bool cmd_use_amulet( GameState * gs) {
    if (!actor_has_item(gs, ITEM_AMULET)) {
        display_line("YOU'RE NOT CARRYING THE AMULET.");
        return false;
    }
    for (;;) {
        // Generate a random number between 1 and 19
        int room_index = rnd_range(gs, 1, room_num_rooms() + 1);
        if ( !(room_index == ROOM_START || room_index ==ROOM_END )) {
            gs->room = room_index;
            actor_remove_object( gs, ITEM_AMULET);
            break;
        }
    }
    return true;
}

static bool cmd_fight( GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == 0) {
        display_line("THERE IS NO MONSTER.");
        return false; // no monster to fight
    }
    int const monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    const MonsterPrototype * monster =  monsters_find_monster(monster_index);
    int ferocity_factor = monster->ferocity_factor;

    if ( actor_has_item(gs, ITEM_SUIT)) {
        display_line("YOUR ARMOR INCREASES YOUR CHANCE OF SUCCESS.");
        ferocity_factor = (int)(3 * ferocity_factor / 4.0 ) ;  //armor gives 25% more advantage
    }


    display_line("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");


    // we'll pause a bit after every turn during the fight
    uint32_t pause_seconds;
    if (GLOBALS.debug_mode ) {
        pause_seconds = 0;
    } else {
        pause_seconds = _1ms * 1000;
    }

    const bool has_axe   = actor_has_item(gs, ITEM_AXE);
    const bool has_sword = actor_has_item(gs, ITEM_SWORD);

    if ( !has_axe && !has_sword) {
        display_line("YOU HAVE NO WEAPONS. YOU MUST FIGHT WITH BARE HANDS.");
        ferocity_factor = (int)(ferocity_factor + ferocity_factor / 5.0);
    } else if ( has_axe && !has_sword) {
        display_line("YOU HAVE ONLY AN AXE TO FIGHT WITH.");
        ferocity_factor = (int)(4 * ferocity_factor / 5.0);
    } else if ( !has_axe && has_sword) {
        display_line("YOU MUST FIGHT WITH YOUR SWORD.");
        ferocity_factor = (int)(3 * ferocity_factor / 4.0);
    } else {
        char option;
        fflush(stdin);
        do {
            display("WHICH WEAPON? 1 - AXE, 2 - SWORD ");
            option = (char)getchar();
            fflush(stdin);
        } while (option != '1' && option != '2');

        if (option == '1') {
            ferocity_factor = (int)(4 * ferocity_factor / 5.0);
        } else {
            ferocity_factor = (int)(3 * ferocity_factor / 4.0);
        }

    }

    do {
        if ( rnd_d(gs) < .5 ) {
            display_linef("\n%s ATTACKS.", monster->name);
        } else {
            display_linef("\nYOU ATTACK.");
        }
        char_sleep((int32_t)pause_seconds);

        if ( rnd_d(gs) < .5 ) {
            display_line("YOU MANAGE TO WOUND IT.");
            ferocity_factor = (int)(5 * ferocity_factor / 6.0);
            char_sleep((int32_t)pause_seconds);
        }

        if ( rnd_d(gs) < .5 ) {
            display_line("THE MONSTER WOUNDS YOU!");
            gs->strength -= 5;
            char_sleep((int32_t)pause_seconds);
        }
    } while ( rnd_d(gs) < .666 );

    bool player_won = false;
    int roll = rnd_range(gs, 5, 16);
    if ( roll > ferocity_factor) {
        display_linef("\n\nAND YOU MANAGED TO KILL THE %s", monster->name);
        gs->monsters_killed++;
        player_won = true;
    } else {
        display_linef("\n\nTHE %s DEFEATED YOU!.", monster->name);
        gs->strength /= 2;
    }
    char_sleep((int32_t)pause_seconds);
    gs->monsters_fought++;
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
    return player_won;
}


static bool cmd_retreat(GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == 0) {
        display_line("THERE IS NO MONSTER HERE.");
        return false; // no monster to retreat from
    }
    if ( rnd_d(gs) < .30 ) {
        display_line("NO, YOU MUST STAND AND FIGHT!");
        cmd_fight(gs);
        return false;
    }



    char first_letter;
    bool is_invalid_command = false;
    char input_buffer[1024];
    // process the next user command. First, check the command is valid for the current state
    do {
        display("WHICH WAY DO YOU WANT TO FLEE? ");
        fscanf(stdin, "%s", input_buffer);
        first_letter = (char)toupper(input_buffer[0]);

        is_invalid_command = ! strchr(VALID_DIRECTIONS, first_letter);
        if (is_invalid_command) {
            display_linef("INVALID DIRECTION '%c'", first_letter);
        }
    } while (is_invalid_command || !cmd_move(gs, first_letter) );
    fflush(stdin);
    return true;
}

static void custom_display_room_content( GameState * gs) {
    const int treasure_id = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    const int monster_id = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];

    if ( treasure_id == 0 && monster_id == 0 ) return;  // room is empty

    if (treasure_id ) {
        if ( gs->has_torch ) {
            display_linef("THERE IS TREASURE HERE WORTH $%d", treasure_id);
        }
    }
    if (monster_id) {
        if (gs->has_torch ) {
            MonsterPrototype *m = monsters_find_monster(monster_id);
            display_line("\nDANGER••• THERE IS DANGER HERE•••• ");
            display_linef("IT IS A %s", m->name);
            display_linef("YOUR PERSONAL DANGER METER REGISTERS %d!!", m->ferocity_factor);
        } else {
            display_line("YOU FEEL A DANGEROUS PRESENCE!");
        }
    }
}

static void cmd_look_custom( GameState *gs) {
    display_room_desc(gs);
    custom_display_room_content(gs);
}



// If the user enters the lift, they are moved to ROOM_REAR_VESTIBULE. We call main_game_loop() recursively from here.
// We need a way to signal to the first main_game_loop() frame that it should exit early since we have processed
// the room here.
// Returns true if the current main_game_loop() iteration should exit early, otherwise returns false.
static bool do_room_actions(GameState *gs) {
    if (gs->room == ROOM_LIFT) {
        uint32_t pause_seconds;
        if (GLOBALS.debug_mode ) {
            pause_seconds = 0;
        } else {
            pause_seconds = _1ms * 1000;
        }
        char_sleep((int32_t)pause_seconds);
        display_line("IT SLOWLY DESCENDS...");
        char_sleep((int32_t)pause_seconds);


        // if in Room 9 transition to Room 10 and show description
        room_set_visited_flag(room_find_room(ROOM_LIFT));

        gs->room = ROOM_REAR_VESTIBULE;
        main_game_loop(gs);
        return true;
    }
    return false;
}

static void display_strength(const  GameState * gs) {
    display_linef("YOUR STRENGTH IS %d.", gs->strength);
    if (gs->strength <= 20) {
        displayf("*** WARNING ***\nCAPTAIN %s, ",gs->player_name->buffer);
        if (gs->strength <= 5) {
            display_line("YOUR STRENGTH IS EXTREMELY LOW.");
            display_line("YOU ARE ABOUT TO DIE!!!");
        } else if (gs->strength <= 10) {
            display_line("YOUR STRENGTH IS VERY LOW.");
            display_line("YOU NEED TO EAT.");
        } else {
            display_line("YOUR STRENGTH IS RUNNING LOW.");
        }
    }
}


/**
 * Death and Win condition check
 * RETURNS: true if the game is over (win or loss).
 * The caller should check gs->is_dead or gs->completed to see the outcome.
 */
static bool check_game_over(GameState *gs) {
    if (gs->completed || gs->game_over) return true;

    for (int i = STAT_STRENGTH; i < STAT_COUNT; ++i) {
        if (gs->stats.as_array[i] <= 0) {
            if (!GLOBALS.silent_mode) {
                display_char_attributes(gs->stats);
                display_line("\nYour combined attributes are no longer\nenough to sustain you... You are dead.");
            }
            gs->is_dead = true;
            gs->game_over = true;
            return true;
        }
    }

    if (gs->room == ROOM_END ) {
        gs->completed = true;
        gs->game_over = true;
        return true;
    }

    return false;
}

// checks if there is a monster and if so, that the user has selected either F or R. Returns true for success.
static bool monster_check(const GameState * gs, const char cmd) {
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


//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------


// -----------------------------------------------------------------
//      called at the start of each new game
// -----------------------------------------------------------------

static void reset(GameState * gs, const uint32_t seed) {
    // reset GameState
    *gs = (GameState){ .seed=seed, .player_name=GLOBALS.player_name, .room = ROOM_START, .cash = 75,  };
    mt_initialize_state(&gs->mt_state, seed); // initialize the PRNG

    gs->stats = random_hero_stats(gs);
    gs->stats.strength = 105;

    //clear all monsters, treasure
    const int num_rooms = room_num_rooms();
    for ( int room_index = 0; room_index < num_rooms; ++room_index ) {
        // note: if we dynamically modify the edge graph we'll need to reset those edges here
        ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
        ROOM_GRAPH[room_index][RGINDEX_MONSTER] = 0;
        ROOM_GRAPH[room_index][RGINDEX_REQUIRED_KEY] = 0;
        ROOM_GRAPH[room_index][RGINDEX_UNUSED] = 0;
        const Room *r = room_find_room(room_index);
        room_clear_monster( r );
        room_remove_all_objects(room_index);
    }



    //allot treasure
    for (int j = 0; j < 4; ++j ) {
        for (;;) {
            // Generate a random number between 1 and num_rooms inclusive
            const int rand_room = rnd_range(gs, 1, num_rooms );
            if ( !(rand_room == ROOM_START || rand_room == ROOM_END || rand_room == ROOM_LIFT
                || ROOM_GRAPH[rand_room][RGINDEX_TREASURE] !=0 ) ) {
                // rand val between 10 and 110 inclusive
                const int treasure = rnd_range(gs, 10, 110 + 1);
                ROOM_GRAPH[rand_room][RGINDEX_TREASURE] = treasure;
                break;
            }
        }
    }
    //allot monsters
    for (int j = 1; j <= 4; ++j ) {
        for (;;) {

            // Generate a random number between 1 and num_rooms inclusive
            const int rand_room = rnd_range(gs, 1, num_rooms );
            if ( !(rand_room == ROOM_START || rand_room == ROOM_END || rand_room == ROOM_LIFT
                || ROOM_GRAPH[rand_room][RGINDEX_MONSTER] !=0 ) ) {
                ROOM_GRAPH[rand_room][RGINDEX_MONSTER] = j;
                MonsterPrototype *m = monsters_find_monster(j);
                room_set_monster(room_find_room(rand_room), j);
                monsters_update_monster(
                    &(MonsterPrototype){
                        .name = m->name,
                        .id = j,
                        .ferocity_factor = m->ferocity_factor,
                        .stats = random_monster_stats(gs)
                    });
                break;
            }
        }
    }
    // rooms 4 and 16 get special treasures, $100-$200
    ROOM_GRAPH[4][RGINDEX_TREASURE]  = rnd_range(gs, 100, 200 + 1);
    ROOM_GRAPH[16][RGINDEX_TREASURE] = rnd_range(gs, 100, 200 + 1);

}

static void init_string_assets() {
    // this will eventually be loaded from a text file
    global_string_assets.conclusion_completed = "YOU HAVE SUCCEEDED!\nYOU MANAGED TO GET OUT OF THE CASTLE\nWELL DONE!";
    global_string_assets.conclusion_died      = "You have died.........";

}


// one time inits of ROOM or ROOM_GRAPH data
static void  init_rooms() {

}

static constexpr size_t num_roomz = 20;  // todo (temp) until room data is read from file
typedef struct RoomData {
    size_t size;
    Room data[num_roomz];
} RoomData;

static RoomData get_room_data(void) {
    return (RoomData){
        .size = num_roomz,
        .data = {
            {.id =  0,  .name= "ROOM 0",           .desc = "UNUSED"},
            {.id =  1,  .name= "HALLWAY",          .desc = "YOU ARE IN THE HALLWAY. THERE IS A DOOR TO THE SOUTH. THROUGH WINDOWS TO THE NORTH YOU CAN SEE A SECRET HERB GARDEN."},
            {.id =  2,  .name= "AUDIENCE CHAMBER", .desc = "THIS IS THE AUDIENCE CHAMBER. THERE IS A WINDOW TO THE WEST. BY LOOKING TO THE RIGHT, THROUGH IT YOU CAN SEE THE ENTRANCE TO THE CASTLE. DOORS LEAVE THIS ROOM TO THE NORTH, EAST, AND SOUTH."},
            {.id =  3,  .name= "GREAT HALL",       .desc = "YOU ARE IN THE GREAT HALL, AN L-SHAPED ROOM. THERE ARE DOORS TO THE EAST AND TO THE NORTH. IN THE ALCOVE IS A DOOR TO THE WEST."},
            {.id =  4,  .name= "MEETING ROOM",     .desc = "THIS IS THE MONARCH'S PRIVATE MEETING ROOM. THERE IS A SINGLE EXIT TO THE SOUTH."},
            {.id =  5,  .name= "INNER HALLWAY",    .desc = "THIS INNER HALLWAY CONTAINS A DOOR TO THE NORTH, AND ONE TO THE WEST, AND A CIRCULAR STAIRWELL PASSES THROUGH THE ROOM. YOU CAN SEE AN ORNAMENTAL LAKE THROUGH THE WINDOWS TO THE SOUTH."},
            {.id =  6,  .name= "ENTRANCE",         .desc = "YOU ARE AT THE ENTRANCE TO A FORBIDDING-LOOKING STONE CASTLE. YOU ARE FACING EAST."},
            {.id =  7,  .name= "KITCHEN",          .desc = "THIS IS THE CASTLE'S KITCHEN. THROUGH WINDOWS IN THE NORTH WALL YOU CAN SEE A SECRET HERB GARDEN. A DOOR LEAVES THE KITCHEN TO THE SOUTH."},
            {.id =  8,  .name= "STORE ROOM",       .desc = "YOU ARE IN THE STORE ROOM, AMIDST SPICES, VEGETABLES, AND VAST SACKS OF FLOUR AND OTHER PROVISIONS. THERE IS A DOOR TO THE NORTH AND ONE TO THE SOUTH."},
            {.id =  9,  .name= "LIFT",             .desc = "YOU HAVE ENTERED THE LIFT..."},
            {.id = 10, .name= "REAR VESTIBULE",    .desc = "YOU ARE IN THE REAR VESTIBULE. THERE ARE WINDOWS TO THE SOUTH FROM WHICH YOU CAN SEE THE ORNAMENTAL LAKE. THERE IS AN EXIT TO THE EAST, AND ONE TO THE NORTH."},
            {.id = 11, .name= "EXIT",              .desc = "YOU'VE DONE IT!!\nTHAT WAS THE EXIT FROM THE CASTLE"},
            {.id = 12, .name= "DUNGEON",           .desc = "YOU ARE IN THE DANK, DARK DUNGEON. THERE IS A SINGLE EXIT, A SMALL HOLE IN THE WALL TOWARDS THE WEST."},
            {.id = 13, .name= "GUARDROOM",         .desc = "YOU ARE IN THE PRISON GUARDROOM, IN THE BASEMENT OF THE CASTLE. THE STAIRWELL ENDS IN THIS ROOM. THERE IS ONE OTHER EXIT, A SMALL HOLE IN THE EAST WALL."},
            {.id = 14, .name= "MASTER BEDROOM",    .desc = "YOU ARE IN THE MASTER BEDROOM ON THE UPPER LEVEL OF THE CASTLE.... LOOKING DOWN FROM THE WINDOW TO THE WEST YOU CAN SEE THE ENTRANCE TO THE CASTLE, WHILE THE SECRET HERB GARDEN IS VISIBLE BELOW THE NORTH WINDOW. THERE ARE DOORS TO THE EAST AND TO THE SOUTH...."},
            {.id = 15, .name= "UPPER HALLWAY",     .desc = "THIS IS THE L-SHAPED UPPER HALLWAY. TO THE NORTH IS A DOOR, AND THERE IS A STAIRWELL IN THE HALL AS WELL. YOU CAN SEE THE LAKE THROUGH THE SOUTH WINDOWS."},
            {.id = 16, .name= "TREASURY",          .desc = "THIS ROOM WAS USED AS THE CASTLE TREASURY IN BY-GONE YEARS.... THERE ARE NO WINDOWS, JUST EXITS TO THE NORTH AND TO THE EAST."},
            {.id = 17, .name= "BEDROOM",           .desc = "OOOOH... YOU ARE IN THE CHAMBERMAID'S BEDROOM. THERE IS AN EXIT TO THE WEST AND A DOOR TO THE SOUTH...."},
            {.id = 18, .name= "DRESSING CHAMBER",  .desc = "THIS TINY ROOM ON THE UPPER LEVEL IS THE DRESSING CHAMBER. THERE IS A WINDOW TO THE NORTH, WITH A VIEW OF THE HERB GARDEN DOWN BELOW. A DOOR LEAVES TO THE SOUTH."},
            {.id = 19, .name= "LIFT ENTRY",        .desc = "THIS IS THE SMALL ROOM OUTSIDE THE CASTLE LIFT WHICH CAN BE ENTERED BY A DOOR TO THE NORTH. ANOTHER DOOR LEADS TO THE WEST. YOU CAN SEE THE LAKE THROUGH THE SOUTHERN WINDOWS."},
        }
    };
}

constexpr size_t num_objectz = 6;
typedef struct ObjectData {
    size_t size;
    Object data[num_objectz];
} ObjectData;

static ObjectData get_object_data(void) {
    return (ObjectData){
        .size = num_objectz,
        .data = {
                { .id =  1, .name = "FLAMING TORCH", .value = 15, .is_light_source_bit = true, .is_lit_bit = true} ,
                { .id =  2, .name = "AXE",           .value = 10, .is_weapon = true },
                { .id =  3, .name = "SWORD",         .value = 20, .is_weapon = true },
                { .id =  4, .name = "FOOD",          .value = 2,  .is_eatable_bit = true },
                { .id =  5, .name = "MAGIC AMULET",  .value = 30 },
                { .id =  6, .name = "SUIT OF ARMOR", .value = 50 },
            }
    };
}

static void initialize() {
    // note: randomized data is initialized in reset()
    RoomData rd = get_room_data();
    room_init(rd.size,rd.data);

    MonsterPrototypeArray *mpa = nullptr;
    int result = monster_read_json_file((MONSTER_DATA_PATH), &mpa);
    printf("monster_read_json_file returns %d\n", result);
    monsters_init( mpa );

    ObjectData od = get_object_data();
    obj_init(od.size, od.data);

    init_string_assets();

    init_rooms();
}


//// ------------------------------------------------------------
////
////    CLEANUP
////
//// ------------------------------------------------------------

static void cleanup(GameState * gs) {
    room_destroy();
    void *free_ptr = (void *) GLOBALS.player_name;
    GLOBALS.player_name = nullptr;
    gs->player_name = nullptr;
    free( free_ptr);
    monsters_destroy();
    obj_destroy();
}


//// ------------------------------------------------------------
////
////    MAIN
////
//// ------------------------------------------------------------


// Core Game Engine Logic
bool perform_action(GameState *gs, char action, int arg1, int arg2, int arg3) {
    const char cmd = (char)toupper(action);

    if (!strchr(VALID_COMMANDS, cmd)) {
        return false; // Unknown command: Not a turn, no state change.
    }

    gs->turns++;



    if ( !monster_check(gs, cmd) ) {
        return false;
    }

    if (strchr(VALID_DIRECTIONS, cmd) ) {
        const bool result = cmd_move(gs, cmd);
        return result;
    }

    bool result = false;
    switch ( cmd ) {
        case 'B':
            //add to inventory/PROVISIONS
            result = cmd_buy(gs);
            break;
        case 'C' :
            result = cmd_eat(gs);
            break;
        case 'T':
            result = cmd_take(gs);
            break;
        case 'M':
            result = cmd_use_amulet(gs);
            break;
        case 'R':
            result = cmd_retreat(gs);
            break;
        case 'F':
            result = cmd_fight(gs);
            break;
        default:
            // Unknown action
            result = false;
            break;
    }

    return result;
}


// 160 REM MAJOR HANDLING ROUTINE
// returns true if still alive
static bool main_game_loop(GameState * gs) {
    uint32_t saved_sleep_duration = GLOBALS.char_sleep_duration;
    const room_id room_id = gs->room;
    const Room *current_room = room_find_room(room_id);
    room_set_visit_started_flag(current_room);

    // in this game, every turn player loses 5 strength points
    // this normally belongs in the model, but we want to make every turn significant.
    gs->strength -= 5;


    if (current_room->is_visited_bit) {
        // if we've already seen this room, speed up output display
        if ( GLOBALS.debug_mode ) {
            set_char_sleep( GLOBALS.debug_visited_sleep );
        } else {
            set_char_sleep( GLOBALS.char_sleep_visited_duration );
        }
    }

    if (gs->room != gs->room_last_turn) {
        // only display room desc once when first entering room. Reduces screen clutter and scrolling.
        // user can always type "look" to re-display room desc.
        display_line("");
        cmd_look_custom(gs);
    }

    if (gs->strength <= 25) display_strength(gs);

    if (check_game_over(gs)){
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return END_GAME;
    }


    // todo (rob) we need a framework hook for action routines for rooms and objects
    if (do_room_actions(gs)) {
        return CONTINUE_GAME;
    }

    // speed up the display of text for the rest of the turn.
    if ( GLOBALS.debug_mode ) {
        set_char_sleep( GLOBALS.debug_visited_sleep );
    } else {
        set_char_sleep(GLOBALS.char_sleep_visited_duration);
    }



    // -----------------------------------------------------------------
    //      process user input
    // -----------------------------------------------------------------
    flush_input();
    char prompt_buffer[1024] = {};
    // we use the room name as the prompt
    snprintf(prompt_buffer, sizeof(prompt_buffer), "\n%s >", current_room->name);
    char cmd_char = get_command_char(prompt_buffer, VALID_COMMANDS, nullptr);

    if (cmd_char == 'Q') {
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return cmd_quit(gs);
    }

    // -----------------------------------------------------------------
    //          DEBUG COMMANDS
    // -----------------------------------------------------------------

    if (cmd_char == '1') {
        display_globals();
    }
    if (cmd_char == '2') {
        display_game_state(gs);
    }
    if (cmd_char == '3') {
        reset(gs, DEBUG_RAND_SEED);
    }

    // -----------------------------------------------------------------
    //      Player Presentation Only
    // -----------------------------------------------------------------

    if (cmd_char == 'H' ) {
        display_help_info();
    } else if (cmd_char == 'L') {
        cmd_look_custom(gs);
    } else if (cmd_char == 'I' ) {
        display_inventory(gs);
    } else if (cmd_char == 'A' ) {
        display_char_attributes(gs->stats);
    } else if (cmd_char == 'O' ) {
        display_score(gs);
    } else if ( !monster_check(gs, cmd_char) ) {
        // no-op, but prevents perform_action from running.
    } else {
        // Now the human call and the ML call use the exact same entry point
        perform_action(gs, cmd_char, 0,0, 0);
    }

    set_char_sleep(saved_sleep_duration);

    if (room_id == gs->room) {
        // if room at end of turn is same as start of turn, update this so we don't display the room desc again
        gs->room_last_turn = room_id;
    } else {
        gs->room_last_turn = gs->room_prev;
    }

    room_set_visited_flag(current_room);
    return CONTINUE_GAME;
}





int main_werewolves_and_wanderer(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    set_silent_mode(GLOBALS.silent_mode);

    if (GLOBALS.debug_mode) {
        set_char_sleep(GLOBALS.debug_normal_sleep);
    } else {
        set_char_sleep(GLOBALS.char_sleep_duration);
    }


    const CharBuffer *player_name = get_player_name("Hello, Explorer");
    GLOBALS.player_name = player_name;

    GameState gs = {};

    initialize();
    reset(&gs, DEBUG_RAND_SEED);
    display_line("Type '[H]elp' for a list of commands.");
    display_line("Your character attribute stats are:");
    display_char_attributes(gs.stats);
    display_line("");
    display_line("--------------------------------------------------------------------------------");
    display_line("");

    // obj_repr();
    // monsters_all_repr();
    // room_rooms_repr();

    bool continue_loop;
    do {
        continue_loop = main_game_loop(&gs);
    } while (continue_loop);


    display_conclusion(&gs);
    display_score(&gs);
    cleanup(&gs);
    display_line("");
    return EXIT_SUCCESS;
}


#ifdef WEREWOLVES_AND_WANDERER_MAIN
int main(void) {
    return main_werewolves_and_wanderer();
}
#endif
