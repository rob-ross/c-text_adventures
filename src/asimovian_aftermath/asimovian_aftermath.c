// asimovian_aftermath.c
//
//
//
// Created 2026/05/11 22:53:54 PDT

//
// The Aftermath of The Asimovian Disaster
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983


/*

MAKE :

cd /Users/robross/Documents/Development/CLionProjects/asimovian_aftermath/text_adventures/src

 * DEBUG *


clang -g -DASIMOVIAN_AFTERMATH_MAIN -DMONSTER_DATA_PATH= \"./monsters.json\" -fsanitize=address -fsanitize=leak -Wall -Werror \
    -Wno-unused-const-variable -Wno-unused-variable -Wno-unused-function \
    -std=c23 -o asimovian_aftermath.out asimovian_aftermath.c  \
            ../adventure_shared.c           \
            ../mersenne_twister.c           \
            ../common/console_utils.c       \
            ../parser.c                     \
            ../rooms.c                      \
            ../objects.c                    \
            ../monsters.c                   \
            ../common/cu_string.c              \
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





// constexpr int NUM_ROOMS      = 21;
constexpr int ROOM_START     = 3;
constexpr int ROOM_END       = 6;
constexpr int POD_ROOM       = 11;
constexpr int RADIATION_ROOM = 13;

const int MAX_ROOM_OBJECTS =  1; //maximum number of items that can be placed in a room
const int MAX_PLAYER_OBJECTS = 9; // max number of items that can be carried


int ROOM_GRAPH[][RGINDEX_COUNT] = {
    { 0,  0,  0,  0,  0,  0,  0}, // Room 0
    { 0,  5,  2,  0,  0,  0,  0}, // Room 1
    { 0,  0,  0,  1,  0,  0,  0}, // Room 2
    { 3,  7,  4,  3,  3,  3,  0}, // Room 3
    { 0,  0,  0,  3,  0,  0,  0}, // Room 4
    { 1,  5,  7,  5,  5,  5,  0}, // Room 5
    { 6,  6,  6,  6,  6,  6,  0}, // Room 6
    { 3,  0,  8,  5,  0,  0,  0}, // Room 7
    { 8, 12,  8,  7,  8,  8,  0}, // Room 8
    {11, 13, 10,  0,  0,  0,  0}, // Room 9
    { 0, 14,  0,  9,  0,  0,  0}, // Room 10
    { 9,  6,  6,  6,  6,  6,  0}, // Room 11
    { 8, 16, 19,  0,  0,  0,  0}, // Room 12
    {13,  0,  0, 13,  0, 13,  0}, // Room 13
    {10,  0, 15, 17,  0, 18,  0}, // Room 14
    { 0,  0,  0, 14,  0, 19,  0}, // Room 15
    {12, 16, 16, 18, 16, 16,  0}, // Room 16
    {14,  0, 18,  0,  0,  0,  0}, // Room 17
    { 0,  0, 16, 17, 14,  0,  0}, // Room 18
    { 0, 12,  0,  0, 15,  0,  0}, // Room 19
    { 0,  0,  0,  0, 15,  0,  0}, // Room 20, DEAD END
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

enum Item {
    ITEM_DUMMY,
    ITEM_LIGHT,
    ITEM_ION,
    ITEM_LASER,
    ITEM_OXY,
    ITEM_TRANSPORTER,
    ITEM_SUIT,
    ITEM_COUNT
};



// state for Mersenne Twister PRNG
static MTState mt_state;

char const * const VALID_COMMANDS = "HIQBOTRFPMNSEWUDALC";


static int radiation_turn_count = 0;  // number of turns player has been in radiation room.

//// ------------------------------------------------------------
////
////    Forward declarations
////
//// ------------------------------------------------------------

static void cmd_look_custom( GameState *gs);








void display_inventory_menu( GameState * gs) {
    display("\nYOU HAVE $");
    printf("%d\n",gs->cash);

    display_line("YOU CAN BUY 1 - NUCLEONIC LIGHT ($15)");
    display_line("            2 - ION GUN ($10)");
    display_line("            3 - LASER ($20)");
    display_line("            4 - OXYGEN ($2 PER UNIT)");
    display_line("            5 - MATTER TRANSPORTER ($30)");
    display_line("            6 - COMBAT SUIT ($50)");
    display_line("            0 - TO CONTINUE EXPLORATION");
}

static bool cmd_buy( GameState * gs) {
    display_line("\nA SUPPLY ANDROID HAS ARRIVED.");
    if (gs->cash <=0 ) {
        display_line("YOU HAVE NO MONEY.");
        return false;
    }

    bool bought_torch = false;

    for (;;) {
        display_inventory_menu(gs);

        char option;
        do {
            display("ENTER NO. OF ITEM REQUIRED ");
            option = (char)getchar();
        } while ( !(option >= '0' && option <= '6') );
        flush_input();
        const int option_index = option - '0';

        // printf("You selected ** %c ** \n", option);

        if ( option_index == 0 ) {
            break;
        }

        const Object *o = obj_find_object(option_index);
        if (!o) {
            printf("got null Object for id=%d\n", option_index);
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
            int oxy_cost = o->value;
            int qty = get_int("HOW MANY UNITS OF OXYGEN? ", 0, gs->cash / oxy_cost );
            int cost = qty * oxy_cost;
            if (gs->cash - cost < 0 ) {
                display_line("YOU HAVEN'T GOT ENOUGH MONEY!");
            } else {
                gs->cash -= cost;
                gs->food += qty;
            }
        }
    }
    if (bought_torch) {
        display_line("");
        cmd_look_custom(gs); // user can now see the room
    }
    return true;
}

static bool cmd_consume_oxygen( GameState * gs) {
    if (gs->food <= 0) {
        display_line("\nYOU HAVE NO OXYGEN.");
        return false;
    }
    display_linef("\nYOU HAVE %d UNITS OF OXYGEN.", gs->food);
    int qty = get_int("HOW MANY DO YOU WANT TO CONSUME? ", 0, gs->food);
    gs->food -= qty;
    gs->strength += (5 * qty);
    return true;
}


static bool cmd_use_transporter( GameState * gs) {
    if ( !actor_has_item(gs, ITEM_TRANSPORTER)) {
        display_line("\nYOU DON'T HAVE A MATTER TRANSPORTER.");
        return false;
    }
    if (gs->room == RADIATION_ROOM) {
        display_line("\nNOTHING HAPPENS.");
        return false;
    }

    for (;;) {
        // Generate a random number between 1 and 19
        int room_index = rnd_range(gs, 1, 20);
        if ( !(room_index == ROOM_END || room_index == POD_ROOM )) {
            gs->room = room_index;
            break;
        }
    }
    return true;
}


static bool cmd_take( GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_TREASURE] == 0 ) {
        display_line("THERE IS NO TREASURE TO PICK UP.");
        return false;
    }
    if ( !gs->has_torch ) {
        display_line("YOU CANNOT SEE WHERE IT IS.");
        return false;
    }

    gs->cash += ROOM_GRAPH[gs->room][RGINDEX_TREASURE] ;
    ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = 0;
    display_line("TAKEN");
    return true;
}




static bool cmd_fight( GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == 0) {
        display_line("THERE IS NOTHING TO FIGHT.");
        return false; // no monster to fight
    }

    int const monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    Room const *r =  room_find_room(gs->room);

    MonsterPrototype *m = monsters_find_monster(r->monster);

    int ferocity_factor = m->ferocity_factor;

    display_line("");

    if (actor_has_item(gs, ITEM_SUIT)) {
        display_line("YOUR ARMOR INCREASES YOUR CHANCE OF SUCCESS.");
        ferocity_factor = (int)(3.0 * (ferocity_factor / 4.0 ));  //armor gives 25% more advantage
    }



    const bool has_ion   = actor_has_item(gs, ITEM_ION);
    const bool has_laser = actor_has_item(gs, ITEM_LASER);

    bool use_ion   = has_ion;
    bool use_laser = has_laser;
    if ( has_ion && has_laser ) {
        int option = get_int("WHICH WEAPON? 1 - ION, 2 - LASER ", 1, 2);
        if (option == 1) {
            use_laser = false;
        } else {
            use_ion = false;
        }
    }

    if ( !use_ion && !use_laser ) {
        display_line("YOU HAVE NO WEAPONS.\nYOU MUST FIGHT WITH BARE HANDS.");
        ferocity_factor = (int)(ferocity_factor + ferocity_factor / 5.0);
    } else if ( use_ion ) {
        display_line("USING THE ION GUN.");
        ferocity_factor = (int)(4.0 * ferocity_factor / 5.0);
    } else {
        display_line("USING YOUR LASER.");
        ferocity_factor = (int)(3.0 * ferocity_factor / 4.0);
    }

    display_line("");
    display_line("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");

    // we'll pause a bit after every turn during the fight
    uint32_t pause_seconds;
    if (GLOBALS.debug_mode ) {
        pause_seconds = 0;
    } else {
        pause_seconds = _1ms * 1000;
    }

    int hits_on_monster = 0;

    do {
        bool has_light = gs->has_torch;

        if ( rnd_d(gs) < .5 || !has_light ) {
            display_linef("%s ATTACKS.", m->name);
            char_sleep((int32_t)pause_seconds);
            if ( rnd_d(gs) < .5) {
                display_line("THE MONSTER WOUNDS YOU.");
                gs->strength -= 5;
            } else {
                if ( rnd_d(gs) < .5) {
                    display_line("YOU SUCCESSFULLY BLOCK IT.");
                } else {
                    display_line("IT MISSES YOU.");
                }
            }
            char_sleep((int32_t)pause_seconds);
            if ( has_light &&  rnd_d(gs) < .1 ) {
                display_line("YOUR LIGHT WAS KNOCKED FROM YOUR HAND!");
                actor_remove_object(gs, ITEM_LIGHT);
                gs->has_torch = false;
                has_light = false;
            } else if ( use_ion &&  rnd_d(gs) < .1 ) {
                display_line("YOU DROP YOUR ION GUN IN THE HEAT OF BATTLE!");
                actor_remove_object(gs, ITEM_ION);
                use_ion = false;
                ferocity_factor = 5 * ferocity_factor / 4;
            } else if (use_laser &&  rnd_d(gs) < .2 ) {
                display_line("YOUR LASER IS KNOCKED FROM YOUR HAND!!");
                actor_remove_object(gs, ITEM_LASER);
                use_laser = false;
                ferocity_factor = 4 * ferocity_factor / 3;
            }
            char_sleep((int32_t)pause_seconds);

        } else {
            display_line("YOU ATTACK.");
            char_sleep((int32_t)pause_seconds);
            if ( rnd_d(gs) < .6) {
                display_line("YOU MANAGE TO WOUND IT.");
                ferocity_factor = 5 * ferocity_factor / 6;
                hits_on_monster++;
            } else {
                display_line("IT BLOCKS YOU.");
            }
            char_sleep((int32_t)pause_seconds);
        }

        if ( rnd_d(gs) < .05) {
            display_line("Aaaaargh!!!\nRIP! TEAR! RIP!");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .05) {
            display_line("YOU WANT TO RUN, BUT YOU STAND YOUR GROUND...");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .05) {
            display_line("*&%%$#$%$%# !! @#$$! #$@! !$ $#$");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .075) {
            display_line("WILL THIS BE A BATTLE TO THE DEATH?");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .075) {
            display_line("HIS EYES FLASH FEARFULLY");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .075) {
            display_line("BLOOD DRIPS FROM HIS CLAWS");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .1) {
            display_line("YOU SMELL THE LUBRICANTS ON HIS BREATH");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .1) {
            display_line("HE STRIKES WILDLY, MADLY.............");
            char_sleep((int32_t)pause_seconds);
        }
        if ( rnd_d(gs) < .1) {
            display_line("YOU HAVE NEVER FOUGHT AN OPPONENT LIKE THIS!!");
            char_sleep((int32_t)pause_seconds);
        }

    } while ( rnd_d(gs) < .65);

    display_line("\n");

    bool player_won = false;
    const int win_chance = rnd_range(gs, hits_on_monster, 16 + hits_on_monster);
    // printf("win_chance: %d, ferocity_factor: %d\n", win_chance, ferocity_factor);
    if ( win_chance > ferocity_factor) {
        display("AND YOU MANAGED TO KILL THE ");
        display_line( m->name);
        gs->monsters_killed++;
        player_won = true;
    } else {
        display("THE ");
        display(m->name);
        display_line(" SERIOUSLY WOUNDS YOU.");
        gs->strength /= 2;
    }
    char_sleep((int32_t)pause_seconds);
    gs->monsters_fought++;
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
    room_clear_monster(r);
    return player_won;
}

static bool cmd_retreat( GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] == 0) {
        display_line("THERE IS NO MONSTER HERE.");
        return false; // no monster to retreat from
    }

    // determine possible exits
    int num_exits = 0;
    int exits[6] = {};
    for (int i = RGINDEX_NORTH; i <= RGINDEX_DOWN; ++i ) {
        int room_index = ROOM_GRAPH[gs->room][i];
        if ( room_index ) {
            if ( !( room_index == ROOM_END || room_index == POD_ROOM) ) {
                // don't retreat to end rooms
                exits[num_exits++] = room_index;
            }
        }
    }

    // randomly move to an adjacent room. If the current room has paths to itself, the room may not change
    int retreat_index = rnd_range(gs, 0, num_exits);

    if (  rnd_d(gs) < .3 || num_exits == 0 || retreat_index == gs->room) {
        display_line("THE CREATURE BLOCKS YOUR PATH.");
        cmd_fight(gs);
        return false;
    }

    gs->room = exits[retreat_index];
    return true;
}

// first_letter must be in "NSEWUD"
// return true if command was sucessfully processed. If false, the move is not allowed and an error message
// will have been displayed
bool cmd_move( GameState * gs, char const cmd_char) {
    const int location = gs->room;
    const int direction_index = calc_room_graph_direction_index(cmd_char);
    if (ROOM_GRAPH[location][direction_index] > 0) {
        gs->room = ROOM_GRAPH[location][direction_index];
        return true;
    }

    display_line(BAD_MOVE_DESC[direction_index]);
    return false;
}

int calc_score(const  GameState * gs) {
    return 3 * gs->turns + 5 * gs->strength + 2 * gs->cash + 10 * gs->food + 30 * gs->monsters_killed;
}

//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------



static void display_help_info(void) {
    display_line("\nVALID COMMANDS ARE:\n");

    display_line("[H]ELP     [I]NVENTORY     [Q]UIT");
    display_line("[B]UY      [C]ONSUME OXY   SC[O]RE");
    display_line("[R]ETREAT  [F]IGHT");
    display_line("[T]AKE     [M]ATTER TRANSPORTER");
    display_line("[N]ORTH    [S]OUTH");
    display_line("[E]AST     [W]EST");
    display_line("[U]P       [D]OWN");
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

void custom_display_inventory(const GameState * gs, bool show_item_index, bool show_item_value ) {
    display_line("");

    if (gs->has_torch) {
        display_line("YOU ARE CARRYING A NUCLEONIC LIGHT.");
    }

    if (gs->cash > 0) {
        display("YOU HAVE $");
        printf("%d", gs->cash);
        char_sleep(-1);
        display_line(" WEALTH IN SOLARIAN CREDITS.");
    }

    if (gs->food > 0 ) {
        display("YOUR RESERVE TANKS HOLD ");
        printf("%d",gs->food);
        char_sleep(-1);
        display_line(" UNITS OF OXYGEN.");
    }

    if ( actor_has_item(gs, ITEM_SUIT) ) {
        display_line("YOU ARE WEARING BATTLE ARMOR.");
    }
    bool has_ion = actor_has_item(gs, ITEM_ION);
    bool has_laser = actor_has_item(gs, ITEM_LASER);
    bool has_transporter = actor_has_item(gs, ITEM_TRANSPORTER);

    const int num_items = has_ion + has_laser  + has_transporter;

    if (num_items > 0) {
        display("YOU ARE CARRYING ");
    }

    // grammar : commas and conjunctions
    // NOTE (rob) - This won't scale well when adding more items.
    if (num_items == 1) {
        if ( has_ion )         display_line("AN ION GUN.");
        else if ( has_laser )  display_line("A LASER.");
        else if (has_transporter) display_line("THE MATTER TRANSPORTER.");
    }

    if (num_items == 3) {
        display_line("AN ION GUN, A LASER, AND THE MATTER TRANSPORTER.");
    }

    if (num_items == 2) {
        if ( has_ion ) {
            display("AN ION GUN AND");
            if ( has_laser ) display_line(" A LASER.");
            else display_line(" THE MATTER TRANSPORTER.");
        } else if ( has_laser) {
            display_line("A LASER AND THE MATTER TRANSPORTER.");
        }
    }
}




static void display_score(const  GameState * gs) {
    display_linef("\nSCORE: %d", calc_score(gs));
    display_linef("\nturns: %d, strength: %d, cash: %d, oxy: %d, monsters fought: %d, killed: %d",
        gs->turns, gs->strength, gs->cash, gs->food, gs->monsters_fought, gs->monsters_killed);
}

void display_strength(const  GameState * gs) {
    display("YOUR STRENGTH IS ");
    printf("%d.\n", gs->strength);
    if (gs->strength <= 20) {
        display("*** WARNING ***\nCAPTAIN ");
        display(gs->player_name->buffer);
        display_line(",");
        if (gs->strength <= 5) {
            display_line("YOUR STRENGTH IS EXTREMELY LOW.");
            display_line("YOU ARE ABOUT TO DIE!!!");
        } else if (gs->strength <= 10) {
            display_line("YOUR STRENGTH IS VERY LOW.");
            display_line("YOU NEED AN OXYGEN BOOST.");
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
bool check_game_over(GameState *gs) {
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

    if ( radiation_turn_count == 2 || gs->strength < 1 ) {
        if (!GLOBALS.silent_mode) {
            if (gs->strength < 1) {
                display_line("YOU HAVE RUN OUT OF OXYGEN....");
            } else {
                display_line("RADIATION DESTROYS YOUR BODY...");
            }
        }
        gs->is_dead = true;
        gs->game_over = true;
        return true;
    }


    if (gs->room == ROOM_END ) {
        gs->completed = true;
        gs->game_over = true;
        return true;
    }

    return false;
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




//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------


// -----------------------------------------------------------------
//      called at the start of each new game
// -----------------------------------------------------------------

void reset(GameState * gs, const uint32_t seed) {
    // reset GameState
    *gs = (GameState){ .seed=seed, .player_name=GLOBALS.player_name, .room = ROOM_START  };
    mt_initialize_state(&gs->mt_state, seed); // initialize the PRNG

    gs->stats    = random_hero_stats(gs);
    gs->strength = rnd_range(gs, 0, 50) + 75;
    gs->cash     = rnd_range(gs, 0, 50) + 50;
    gs->food     = rnd_range(gs, 0, 16);

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
    for (int j = 0; j < 7; ++j ) {
        for (;;) {
            // Generate a random number between 1 and num_rooms inclusive
            const int room_index = rnd_range(gs, 1, num_rooms );
            if ( !(room_index == ROOM_END || room_index == POD_ROOM || room_index == RADIATION_ROOM ||
                    ROOM_GRAPH[room_index][RGINDEX_TREASURE] != 0 ) ) {
                    const int treasure = rnd_range(gs, 10, 111); // rand val between 10 and 110 inclusive
                    ROOM_GRAPH[room_index][RGINDEX_TREASURE] = treasure;
                    break;
            }
        }
    }

    //allot monsters
    for (int t = 0; t < 2; ++t) {
        for (int j = 1; j < 5; ++j ) {
            for (;;) {
                // Generate a random number between 1 and num_rooms inclusive
                const int rand_room = rnd_range(gs, 1, num_rooms );
                if ( !(rand_room == ROOM_END || rand_room == ROOM_START || rand_room == RADIATION_ROOM ||
                        ROOM_GRAPH[rand_room][RGINDEX_MONSTER] != 0 ) ) {
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
    }

    radiation_turn_count = 0;

}

static void init_string_assets() {
    // this will eventually be loaded from a text file
    global_string_assets.conclusion_completed = "YOU HAVE SUCCEEDED!\nYOU HAVE ESCAPED IN THE POD.\nWELL DONE!";
    global_string_assets.conclusion_died      = "YOU HAVE DIED.........";

}

void init_rooms() {
    RandomTextArray *rta;
    // rooms 4, 5, 7, 8, 12, 13, 14, 16, and 19 have randomized text
    // room 4
    rta = create_rta(1);
    rta->lines[0] = ( RandomText){ .chance_percent = .4, .text="WHAT A SUPERB SIGHT....... "};
    room_set_preamble(1, rta);

    // room 5
    rta =  create_rta(2);
    rta->lines[0] = ( RandomText){ .chance_percent = .5, .text="THE SOLAR LAMPS ARE STILL SHINING."};
    rta->lines[1] = ( RandomText){ .chance_percent = .5, .text="A FEW PLANTS ARE STILL ALIVE TO THE EAST."};
    room_set_epilog(5,rta);

    // room 7
    rta =  create_rta(3);
    rta->lines[0] = ( RandomText){ .chance_percent = .5, .text="MOST OF THE SLEEPING SHELLS ARE EMPTY."};
    rta->lines[1] = ( RandomText){ .chance_percent = .5, .text="THE FEW REMAINING CREW STIR FITFULLY IN THEIR ENDLESS, DREAMLESS SLEEP."};
    rta->lines[2] = ( RandomText){ .chance_percent = .3, .text="THERE ARE EXITS TO THE NORTH, EAST AND WEST."};
    room_set_epilog(7,rta);

    // room 8
    rta =  create_rta(3);
    rta->lines[0] = ( RandomText){ .chance_percent = .5, .text="PASSENGERS FLOAT BY AT RANDOM."};
    rta->lines[1] = ( RandomText){ .chance_percent = .5, .text="IT IS ENORMOUS, IT SEEMS TO GO ON FOREVER."};
    rta->lines[2] = ( RandomText){ .chance_percent = .1, .text="THE ONLY EXITS ARE TO THE WEST AND SOUTH."};
    room_set_epilog(8,rta);

    // room 12
    rta =  create_rta(1);
    rta->lines[0] = ( RandomText){ .chance_percent = .5, .text="THIS IS THE SHIP'S MAIN NAVIGATION ROOM."};
    room_set_preamble(12, rta);

    rta =  create_rta(1);
    rta->lines[0] = ( RandomText){ .chance_percent = .2, .text="YOU CAN JUST MAKE OUT EXITS TO THE SOUTH AND TO THE EAST."};
    room_set_epilog(12,rta);

    // room 13
    rta =  create_rta(1);
    rta->lines[0] = ( RandomText){ .chance_percent = .5, .text="YOUR BODY TWISTS AND BURNS..."};
    room_set_preamble(13, rta);

    rta =  create_rta(2);
    rta->lines[0] = ( RandomText){ .chance_percent = .5, .text="NO MATTER WHAT YOU DO"};
    rta->lines[1] = ( RandomText){ .chance_percent = .5, .text="YOU ARE DOOMED TO DIE HERE."};
    room_set_epilog(13,rta);

    // room 14
    rta =  create_rta(2);
    rta->lines[0] = ( RandomText){ .chance_percent = .1, .text="YOU CAN BARELY MAKE OUT DOORS TO THE NORTH AND WEST."};
    rta->lines[1] = ( RandomText){ .chance_percent = .4, .text="A SHAFT LEADS DOWNWARDS TO THE REPAIR CENTER."};
    room_set_epilog(14,rta);

    // room 16
    rta =  create_rta(5);
    rta->lines[0] = ( RandomText){ .chance_percent = .3, .text="RARE METALS AND VENUSIAN SCULPTURES"};
    rta->lines[1] = ( RandomText){ .chance_percent = .2, .text="PRESERVED SCALAPIAN DESERT FISH"};
    rta->lines[2] = ( RandomText){ .chance_percent = .3, .text="FLASHING EBONY SCITH STONES FROM XARIAX IV"};
    rta->lines[3] = ( RandomText){ .chance_percent = .2, .text="AWESOME TRADER ANT EFFIGIES FROM THE QWERTYIOPIAN EMPIRE"};
    rta->lines[4] = ( RandomText){ .chance_percent = .1, .text="THE LIGHT IS STRONGER TO THE WEST"};
    room_set_epilog(16,rta);

    // room 19
    rta =  create_rta(1);
    rta->lines[0] =
        ( RandomText){
            .chance_percent = .5,
            .text  = "ONE OF WHICH IS THE GRAVITY WELL.",
        .else_text = "ONE OF WHICH LEADS TO THE GOODS HOLD." };
    room_set_epilog(19,rta);

}



static constexpr size_t num_roomz = 21;  // todo (temp) until room data is read from file
typedef struct RoomData {
    size_t size;
    Room data[num_roomz];
} RoomData;

static RoomData get_room_data(void) {
    return (RoomData){
        .size = num_roomz,
        .data = {
        {.id =  0,  .name= "ROOM 0",       .desc = "UNUSED"},
        {.id =  1,  .name= "REC CENTER",   .desc = "YOU ARE IN THE FORMER RECREATION. CENTER. EQUIPMENT FOR MUSCLE-TRAINING IN ZERO GRAVITY LITTERS THE AREA."},
        {.id =  2,  .name= "REPAIR HOLD",  .desc = "THIS WAS THE REPAIR AND MAINTENANCE HOLD OF THE SHIP. YOU CAN ONLY LEAVE IT VIA THE GIANT HANGAR DOOR TO THE WEST."},
        {.id =  3,  .name= "WRECKED HOLD", .desc = "YOU ARE IN THE WRECKED HOLD OF A SPACE SHIP. THE CAVERNOUS INTERIOR IS LITTERED WITH FLOATING WRECKAGE, AS IF FROM SOME TERRIBLE EXPLOSION EONS AGO......"},
        {.id =  4,  .name= "OBSERVATORY",  .desc = "THE VIEW OF THE STARS FROM THIS OBSERVATION PLATFORM IS MAGNIFICENT, AS FAR AS THE EYE CAN SEE. THE SINGLE EXIT IS BACK WHERE YOU CAME FROM."},
        {.id =  5,  .name= "HYDRO FARM",   .desc = "ACRE UPON ACRE OF DRIED-UP HYDROPONIC PLANT BEDS STRETCH AROUND YOU. ONCE THIS AREA FED THE THOUSAND ON BOARD THE SHIP."},
        {.id =  6,  .name= "ESCAPE",             .desc = "YOU ARE FREE. YOU HAVE MADE IT. YOUR POD SAILS FREE INTO SPACE..........."},
        {.id =  7,  .name= "CREW QUARTERS",      .desc = "YOU ARE IN THE CREW'S SLEEPING QUARTERS."},
        {.id =  8,  .name= "PASSENGER QUARTERS", .desc = "THE FORMER PASSENGER SUSPENDED ANIMATION DORMITORY..."},
        {.id =  9,  .name= "HOSPITAL",      .desc = "THIS IS THE SHIP'S HOSPITAL, WHITE AND STERILE. A BUZZING SOUND, AND A STRANGE WARMTH COME FROM THE SOUTH, WHILE A CHILL IS FELT TO THE NORTH."},
        {.id = 10, .name=  "GALLEY",        .desc = "FOOD FOR ALL THE CREW WAS PREPARED IN THIS GALLEY. THE REMAINS FROM PREPARATIONS OF THE FINAL MEAL CAN BE SEEN. DOORS LEAVE THE GALLEY TO THE SOUTH AND TO THE WEST."},
        {.id = 11, .name= "ESCAPE POD",     .desc = "AHA • • • THAT LOOKS LIKE THE SPACE POD NOW, AND ITS OUTSIDE DIALS INDICATE IT IS STILL IN PERFECT CONDITION."},
        {.id = 12, .name= "NAV ROOM",       .desc = "STRANGE MACHINERY LINES THE WALLS, WHILE OVERHEAD, A HOLOGRAPHIC STAR MAP SLOWLY TURNS. THE FLICKERING GREEN LIGHT MAKES IT HARD TO SEE."},
        {.id = 13, .name= "RADIATION",      .desc = "YOU ARE CAUGHT IN A DEADLY RADIATION FIELD. SLOWLY YOU REALISE THIS IS THE END."},
        {.id = 14, .name= "ENGINE ROOM",    .desc = "THIS IS THE POWER CENTER OF THE SHIP. THE CHARACTERISTIC BLUE METAL LIGHT OF THE STILL-FUNCTIONING ION DRIVE FILLS THE ENGINE ROOM. THE HAZE MAKES IT DIFFICULT TO SEE."},
        {.id = 15, .name= "ANDROID ROOM",   .desc = "YOU ARE STANDING IN THE ANDROID STORAGE HOLD. ROW UPON ROW OF METAL MEN STAND STIFFLY AT ATTENTION, AWAITING THE DISTINCTIVE SOUND OF THEIR LONG-DEAD CAPTAIN TO SET THEM INTO MOTION. A LIGHT COMES FROM THE WEST AND THROUGH THE GRAVITY WELL SET INTO THE FLOOR."},
        {.id = 16, .name= "TRADE HALL",     .desc = "ANOTHER CAVERNOUS, SEEMINGLY ENDLESS HOLD, THIS ONE CRAMMED WITH GOODS FOR TRADING..."},
        {.id = 17, .name= "ARMORY",         .desc = "A STARK, METALLIC ROOM, REEKING OF LUBRICANTS. WEAPONS LINE THE WALL, RANK UPON RANK. EXITS FOR SOLDIER ANDROIDS ARE TO THE NORTH AND THE EAST."},
        {.id = 18, .name= "REPAIR HOLD",    .desc = "ABOVE YOU IS THE GRAVITY SHAFT LEADING TO THE ENGINE ROOM. THIS IS THE SHIP REPAIR CENTER WITH EMERGENCY EXITS TO THE SOLDIER ANDROIDS STORAGE AND TO THE TRADING GOODS HOLD."},
        {.id = 19, .name= "COMMAND CENTER", .desc = "YOU'VE STUMBLED ON THE SECRET COMMAND CENTER WHERE SCREENS BRING VIEWS FROM ALL AROUND THE SHIP. THERE ARE TWO EXITS........"},
        {.id = 20, .name= "DEAD END",       .desc = "YOU HAVE RUN OUT OF OXYGEN..."},
        }
    };
}

static constexpr size_t num_objectz = 6;
typedef struct ObjectData {
    size_t size;
    Object data[num_objectz];
} ObjectData;



static ObjectData get_object_data(void) {
    return (ObjectData){
        .size = num_objectz,
        .data = {
                { .id =  1, .name = "NUCLEONIC LIGHT",    .value = 15, .is_light_source_bit = true, .is_lit_bit = true} ,
                { .id =  2, .name = "ION GUN",            .value = 10, .is_weapon = true },
                { .id =  3, .name = "LASER",              .value = 20, .is_weapon = true },
                { .id =  4, .name = "OXYGEN",             .value = 2,  .is_eatable_bit = true },
                { .id =  5, .name = "MATTER TRANSPORTER", .value = 30 },
                { .id =  6, .name = "COMBAT SUIT",        .value = 50 },
            }
    };
}



void initialize( GameState * gs) {
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
    const char cmd_char = (char)toupper(action);

    if (!strchr(VALID_COMMANDS, cmd_char)) {
        return false; // Unknown command: Not a turn, no state change.
    }

    gs->turns++;

    if ( !monster_check(gs, cmd_char) ) {
        return false;
    }

    if (strchr(VALID_DIRECTIONS, cmd_char) ) {
        const bool result = cmd_move(gs, cmd_char);
        return result;
    }


    bool result = false;
    switch ( cmd_char ) {
        case 'B':
            //add to inventory/PROVISIONS
            result = cmd_buy(gs);
            break;
        case 'C' :
            result = cmd_consume_oxygen(gs);
            break;
        case 'T':
            result = cmd_take(gs);
            break;
        case 'M':
            result = cmd_use_transporter(gs);
            break;
        case 'R':
            result = cmd_retreat(gs);
            break;
        case 'F':
            result = cmd_fight(gs);
            break;
        default:
            // Unknown action
            printf("perform_action: unknown action: %c", cmd_char);
            result = false;
            break;
    }

    return result;



    return true;
}

static bool main_game_loop( GameState * gs) {
    uint32_t saved_sleep_duration = GLOBALS.char_sleep_duration;
    const room_id room_id = gs->room;
    const Room *current_room = room_find_room(room_id);
    room_set_visit_started_flag(current_room);


    if (gs->room == RADIATION_ROOM ) {
        radiation_turn_count++;
    }



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
    // do_room_actions(gs);

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
        custom_display_inventory(gs, false, false);
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


static int main_asimovian_aftermath(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    set_silent_mode(GLOBALS.silent_mode);

    if (GLOBALS.debug_mode) {
        set_char_sleep(GLOBALS.debug_normal_sleep);
    } else {
        set_char_sleep(GLOBALS.char_sleep_duration);
    }


    const CharBuffer *player_name = get_player_name("HELLO CAPTAIN");
    GLOBALS.player_name = player_name;

    GameState gs = {};


    initialize(&gs);
    reset(&gs, DEBUG_RAND_SEED);

    display_line("TYPE '[H]ELP' FOR LIST OF COMMANDS.");
    display_line("YOUR CHARACTER ATTRIBUTE STATS ARE:");
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

#ifdef ASIMOVIAN_AFTERMATH_MAIN
int main(void) {
    return main_asimovian_aftermath();
}
#endif


//// ------------------------------------------------------------
////
////    DEBUG
////
//// ------------------------------------------------------------

static void debug_room_desc() {
    const int num_rooms = room_num_rooms();
    for (int room_index = 0; room_index < num_rooms; ++room_index) {
        const Room r = *room_find_room(room_index);
        putchar('\n');
        display_line(r.name);
        display_line("---------------------------------");
        if (r.preamble) {
            display_line("PREAMBLE");
            for ( size_t i = 0; i < r.preamble->length; ++i) {
                display_line(r.preamble->lines[i].text);
                if (r.preamble->lines[i].else_text) {
                    display_line(r.preamble->lines[i].else_text);
                }
            }
        }
        putchar('\n');
        display_line(r.desc);
        putchar('\n');
        if (r.epilog) {
            display_line("EPILOG");
            for ( size_t i = 0; i < r.epilog->length; ++i) {
                display_line(r.epilog->lines[i].text);
                if (r.epilog->lines[i].else_text) {
                    display_line(r.epilog->lines[i].else_text);
                }
            }
        }

    }
}


