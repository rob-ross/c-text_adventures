// werewolves_and_wanderer.c
//
// Port of BASIC game from "Creating Adventure Games on Your Computer,"
// by Tim Hartnell, 1983
//
//
// Created 2026/05/01 18:00:08 PDT

// make :
// cd /Users/robross/Documents/Development/CLionProjects/text_adventures/src/
// DEBUG:
//  clang -g -DWAREWOLVES_AND_WANDERER_MAIN -std=c23 -o warewolves_and_wanderer.out warewolves_and_wanderer.c


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../adventure_shared.h"
#include "../directions.h"
#include "../common/console_utils.h"
#include "../mersenne_twister.h"
#include "../rooms.h"
#include "../monsters.h"
#include "../objects.h"


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

constexpr int ROOM_START         =  6;
constexpr int ROOM_END           = 11;

const int MAX_ROOM_OBJECTS = 1; //maximum number of items that can be placed in a room
const int MAX_PLAYER_OBJECTS = 6; // max number of items that can be carried

constexpr bool CONTINUE_GAME = true;
constexpr bool END_GAME      = false;


int ROOM_GRAPH[20][10] = {
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





static void display_conclusion(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    set_char_sleep(_30ms);  // so final text display is slowed down

    if (gs->completed && !gs->is_dead) {
        display_line("\nYOU'VE DONE IT!!");
        display_line("THAT WAS THE EXIT FROM THE CASTLE");
        display_linef("\nYOU HAVE SUCCEEDED, %s!\n", gs->player_name->buffer);
        display_line("YOU MANAGED TO GET OUT OF THE CASTLE");
        display_line("\nWELL DONE!");
    } else if (gs->is_dead) {
        display_line("You have died.........");
    }
}

static void display_score(const GameState * gs) {
    const int score = 3* gs->turns + 5* gs->strength + 2* gs->cash + gs->food + 30*gs->monsters_killed;
    display_linef("\nYOUR SCORE IS %d", score);
}


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
// return true if the command was successfully processed. If false, the move is not allowed.
static bool cmd_move(struct GameState * gs, char first_letter) {

    int location = gs->room;
    int direction_index = calc_room_graph_direction_index(first_letter);
    if (ROOM_GRAPH[location][direction_index] > 0) {
        gs->room = ROOM_GRAPH[location][direction_index];
        if (gs->room == 11) {
            // Exit!!

            gs->completed = true;
        }
        return true;
    }

    printf("%s\n", BAD_MOVE_DESC[direction_index]);


    return false;
}

static int const ITEM_COSTS[] = { 0, 15, 10, 20, 2, 30, 50};

static void display_inventory_menu(struct GameState * gs) {
    printf("\nYOU HAVE $%d\n", gs->cash);

    printf("YOU CAN BUY 1 - FLAMING TORCH ($15)\n");
    printf("            2 - AXE ($10)\n");
    printf("            3 - SWORD ($20)\n");
    printf("            4 - FOOD($2 PER UNIT)\n");
    printf("            5 - MAGIC AMULET ($30)\n");
    printf("            6 - SUIT OF ARMOR ($50)\n");
    printf("            0 - TO CONTINUE ADVENTURE\n");
}

char const * const VALID_COMMANDS = "QNSEWUDRFIBCTPMHAOL123";

static void display_help_info(void) {
    if (GLOBALS.silent_mode) return;

    display_line("\nVALID COMMANDS ARE:\n");

    display_line("[H]elp       [I]nventory  [Q]uit" );
    display_line("[A]ttributes Sc[o]re      [L]ook" );
    display_line("[R]etreat    [F]ight      [C]onsume" );
    display_line("[T]ake       Dro[p]       [M]agic Amulet");
    display_line("[N]orth      [S]outh      [B]uy");
    display_line("[E]ast       [W]est");
    display_line("[U]p         [D]own");

    display_line("\nDEBUG:");
    display_line("[1]Globals  [2]GameState [3]Reset  [M]agic");
}

static void cmd_buy( GameState * gs) {
    printf("PROVISIONS AND INVENTORY\n");
    if (gs->cash <=0 ) {
        printf("YOU HAVE NO MONEY.\n");
        return;
    }

    for (;;) {
        display_inventory_menu(gs);

        char option;
        fflush(stdin);
        do {
            printf("ENTER NO. OF ITEM REQUIRED ");
            option = (char)getchar();
            fflush(stdin);
        } while ( !(option >= '0' && option <= '6') );

        const int option_index = option - '0';

        printf("You selected ** %c ** \n", option);

        if ( option_index == 0 ) {
            //cls
            break;
        }

        const Object *o = obj_find_object(option_index);
        if (!o) {
            printf("got null Object for id=%d\n", option_index);
            return;
        }


        if ( option_index != 4 ) {
            gs->cash -= o->value;
            if (gs->cash < 0) {
                printf("YOU HAVE TRIED TO CHEAT ME!\n");
                //punish user
                gs->cash = 0;
                actor_remove_all_objects(gs);
                gs->has_torch = false;
                gs->food = gs->food / 4 ;
            } else {
                actor_add_object(gs, option_index);
                if (option_index == ITEM_LIGHT) gs->has_torch = true;
            }
        }

        if (option_index == 4 ) {
            for (;;) {
                char food_quantity;
                fflush(stdin);
                do {
                    printf("HOW MANY UNITS OF FOOD (0-9)? ");
                    food_quantity = (char)getchar();
                    fflush(stdin);
                } while ( !(food_quantity >= '0' && food_quantity <= '9') );

                const int qty = food_quantity - '0';
                int cost = qty * o->value;
                if (gs->cash - cost < 0 ) {
                    printf("YOU HAVEN'T GOT ENOUGH MONEY!\n");
                } else {
                    gs->cash -= cost;
                    gs->food += qty;
                    break;
                }
            }
        }
    }

}


static void cmd_eat(struct GameState * gs) {
    if (gs->food <= 0) return;
    for (;;) {
        char food_quantity;
        fflush(stdin);
        do {
            printf("YOU HAVE %d UNITS OF FOOD.\n", gs->food);
            printf("HOW MANY DO YOU WANT TO EAT (0-9)? ");

            food_quantity = (char)getchar();
            fflush(stdin);
        } while ( !(food_quantity >= '0' && food_quantity <= '9') );

        const int qty = food_quantity - '0';
        if ( qty <= gs->food) {
            gs->food -= qty;
            gs->strength += (5 * qty);
            break;
        }
    }
}

static void cmd_take(struct GameState * gs) {
    if ( ROOM_GRAPH[gs->room][RGINDEX_TREASURE] <= 0 ) {
        printf("THERE IS NO TREASURE TO PICK UP.\n");
        return;
    }
    if ( !gs->has_torch ) {
        printf("YOU CANNOT SEE WHERE IT IS\n");
        return;
    }
    gs->cash += ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = 0;
}

static void use_magic_amulet(struct GameState * gs) {
    for (;;) {
        // Generate a random number between 1 and 19
        int room_index = (rand() % 19) + 1;
        if ( !(room_index == 6 || room_index == 11 )) {
            gs->room = room_index;
            break;
        }
    }
}

static void cmd_fight(struct GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_TREASURE] >= 0) {
        return; // no monster to fight
    }
    int const monster_index = -ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    const Monster * monster =  monsters_find_monster(monster_index);
    int ferocity_factor = monster->ferocity_factor;

    if (gs->items[ITEM_SUIT]) {
        printf("YOUR ARMOR INCREASES YOUR CHANCE OF SUCCESS.\n");
        ferocity_factor = 3 * (ferocity_factor / 4);  //armor gives 25% more advantage
    }

    for (int j = 0; j < 6; ++j ) {
        printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    }

    const bool has_axe   = gs->items[ITEM_ION];
    const bool has_sword = gs->items[ITEM_LASER];

    if ( !has_axe && !has_sword) {
        printf("YOU HAVE NO WEAPONS.\nYOU MUST FIGHT WITH BARE HANDS.\n");
        ferocity_factor = ferocity_factor + ferocity_factor / 5;
    } else if ( has_axe && !has_sword) {
        printf("YOU HAVE ONLY AN AXE TO FIGHT WITH.\n");
        ferocity_factor = 4 * ferocity_factor / 5;
    } else if ( !has_axe && has_sword) {
        printf("YOU MUST FIGHT WITH YOUR SWORD.\n");
        ferocity_factor = 3 * ferocity_factor / 4;
    } else {
        char option;
        fflush(stdin);
        do {
            printf("WHICH WEAPON? 1 - AXE, 2 - SWORD ");
            option = (char)getchar();
            fflush(stdin);
        } while (option != '1' && option != '2');

        if (option == '1') {
            ferocity_factor = 4 * ferocity_factor / 5;
        } else {
            ferocity_factor = 3 * ferocity_factor / 4;
        }

    }

    do {
        if ( rand() % 2 == 1) {
            printf("\n%s ATTACKS.\n", monster->name);
        } else {
            printf("\nYOU ATTACK.\n");
        }

        if ( rand() % 2 == 1 ) {
            printf("YOU MANAGE TO WOUND IT.\n");
            ferocity_factor = 5 * ferocity_factor / 6;
        }

        if ( rand() % 2 == 1 ) {
            printf("THE MONSTER WOUNDS YOU!\n");
            gs->strength -= 5;
        }
    } while ( rand() % 100 > 34);

    if ( rand() % 16 > ferocity_factor) {
        printf("\n\nAND YOU MANAGED TO KILL THE %s\n", monster->name);
        gs->monsters_killed++;
    } else {
        printf("\n\nTHE %s DEFEATED YOU!.\n", monster->name);
        gs->strength /= 2;
    }

    ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = 0;
}


static void cmd_retreat(GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_TREASURE] >= 0) {
        return; // no monster to retreat from
    }
    if ( (rand() % 100) > 69 ) {
        printf("NO, YOU MUST STAND AND FIGHT!\n");
        cmd_fight(gs);
        return;
    }



    char first_letter;
    bool is_invalid_command = false;
    char input_buffer[1024];
    // process the next user command. First, check the command is valid for the current state
    do {
        printf("WHICH WAY DO YOU WANT TO FLEE? ");
        fscanf(stdin, "%s", input_buffer);
        // printf("   you entered: %s\n", input_buffer);
        first_letter = (char)toupper(input_buffer[0]);

        is_invalid_command = ! strchr(VALID_DIRECTIONS, first_letter);
        if (is_invalid_command) {
            printf("INVALID DIRECTION '%c'\n", first_letter);
        }
    } while (is_invalid_command || !cmd_move(gs, first_letter) );


}

bool cmd_look(GameState *gs) {
    // This is a presentation-layer-only command. It just displays text to the user they have already seen.
    display_room_desc(gs);
    display_room_content(gs);
    return true;
}

void do_room_actions(GameState *gs) {
    if (gs->room == 9) {
        // if in Room 9 transition to Room 10 and show description
        gs->room = 10;
        display_room_desc(gs);
    }
}
/**
 * Death and Win condition check
 * RETURNS: true if the game is over (win or loss).
 * The caller should check gs->is_dead or gs->completed to see the outcome.
 */
bool check_game_over(GameState *gs) {
    if (gs->completed) return true;

    if (gs->room == ROOM_END ) {
        gs->completed = true;
        gs->game_over = true;
        return true;
    }

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
    return false;
}


//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------

// one time inits of ROOM or ROOM_GRAPH data
static void  init_rooms() {

}

// -----------------------------------------------------------------
//      called at the start of each new game
// -----------------------------------------------------------------

void reset(GameState * gs, const uint32_t seed) {
    // reset GameState
    *gs = (GameState){ .seed=seed, .player_name=GLOBALS.player_name, .room = ROOM_START, .cash = 75,  };
    mt_initialize_state(&gs->mt_state, seed); // initialize the PRNG

    gs->stats = random_hero_stats(gs);
    gs->stats.strength = 105;

    //clear all monsters, treasure

    // allot random treasure

    // allot random monsters


    //allot treasure
    for (int j = 0; j < 4; ++j ) {
        for (;;) {
            // Generate a random number between 1 and 19
            int room_index = (rand() % 19) + 1;
            if ( !(room_index == ROOM_START || room_index == ROOM_END || ROOM_GRAPH[room_index][RGINDEX_TREASURE] !=0 ) ) {
                int treasure = (rand() % 100) + 10;; // rand val between 10 and 109 inclusive
                ROOM_GRAPH[room_index][RGINDEX_TREASURE] = treasure;
                break;
            }

        }
    }
    //allot monsters
    for (int j = 0; j < 4; ++j ) {
        for (;;) {
            // Generate a random number between 1 and 19
            int room_index = (rand() % 19) + 1;
            if ( !(room_index == ROOM_START || room_index == ROOM_END || ROOM_GRAPH[room_index][RGINDEX_TREASURE] !=0 ) ) {
                ROOM_GRAPH[room_index][RGINDEX_TREASURE] = -j;
                break;
            }
        }
    }
    // rooms 4 and 16 get special treasures
    ROOM_GRAPH[4][RGINDEX_TREASURE] = 100 + (rand() % 100);
    ROOM_GRAPH[16][RGINDEX_TREASURE] = 100 + (rand() % 100);

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
            {.id =  9,  .name= "LIFT",             .desc = "YOU HAVE ENTERED THE LIFT... IT SLOWLY DESCENDS..."},
            {.id = 10, .name= "REAR VESTIBULE",    .desc = "YOU ARE IN THE REAR VESTIBULE. THERE ARE WINDOWS TO THE SOUTH FROM WHICH YOU CAN SEE THE ORNAMENTAL LAKE. THERE IS AN EXIT TO THE EAST, AND ONE TO THE NORTH."},
            {.id = 11, .name= "EXIT",              .desc = "EXIT"},
            {.id = 12, .name= "DUNGEON",           .desc = "YOU ARE IN THE DANK, DARK DUNGEON. THERE IS A SINGLE EXIT, A SMALL HOLE IN THE WALL TOWARDS THE WEST."},
            {.id = 13, .name= "GUARDROOM",         .desc = "YOU ARE IN THE PRISON GUARDROOM, IN THE BASEMENT OF THE CASTLE. THE STAIRWELL ENDS IN THIS ROOM. THERE IS ONE OTHER EXIT, A SMALL HOLE IN THE EASST WALL."},
            {.id = 14, .name= "MASTER BEDROOM",    .desc = "YOU ARE IN THE MASTER BEDROOM ON THE UPPER LEVEL OF THE CASTLE.... LOOKING DOWN FROM THE WINDOW TO THE WEST YOU CAN SEE THE ENTRANCE TO THE CASTLE, WHILE THE SECRET HERB GARDEN IS VISIBLE BELOW THE NORTH WINDOW. THERE ARE DOORS TO THE EAST AND TO THE SOUTH...."},
            {.id = 15, .name= "UPPER HALLWAY",     .desc = "THIS IS THE L-SHAPPED UPPER HALLWAY. TO THE NORTH IS A DOOR, AND THERE IS A STAIRWELL IN THE HALL AS WELL. YOU CAN SEE THE LAKE THROUGH THE SOUTH WINDOWS."},
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
                { .id =  1, .name = "Flaming Torch", .value = 15, .is_light_source_bit = true, .is_lit_bit = true} ,
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

    monsters_init("monsters.txt");
    const int num_monsters = monsters_num_monsters();
    for (int i = 1; i < num_monsters; ++i) {
        Monster *m = monsters_find_monster(i);
        m->ferocity_factor = 5 * i;
    }

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



// 160 REM MAJOR HANDLING ROUTINE
// returns true if still alive
static bool main_game_loop(GameState * gs) {
    uint32_t saved_sleep_duration = GLOBALS.char_sleep_duration;
    const room_id room_id = gs->room;
    const Room *current_room = room_find_room(room_id);
    room_set_visit_started_flag(current_room);

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
        display_room_desc(gs);
        display_room_content(gs);  // we need to be able to query if any contents exist to add a newline before here
    }

    if (check_game_over(gs)){
        set_char_sleep(saved_sleep_duration);
        room_set_visited_flag(current_room);
        return END_GAME;
    }

    // speed up the display of text for the rest of the turn.
    if ( GLOBALS.debug_mode ) {
        set_char_sleep( GLOBALS.debug_visited_sleep );
    } else {
        set_char_sleep(GLOBALS.char_sleep_visited_duration);
    }

    gs->strength -= 5;

    if (gs->strength <= 15) {
        printf("WARNING, %s, YOUR STRENGTH\nIS RUNNING LOW\n", gs->player_name->buffer);
    }

    gs->turns += 1;
    printf("%s, YOUR STRENGTH IS %d\n", gs->player_name->buffer, gs->strength);

    // display_inventory(gs);
    //
    // display_room_desc(gs);


    int room_contents = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];

    if ( room_contents > 0 ) {
        printf("THERE IS TREASURE HERE WORTH $%d\n", room_contents);
    } else if (room_contents < 0 ) {
        int index = -room_contents;
        display_line("\n\nDANGER...THERE IS A MONSTER HERE....");
        const Monster *m = monsters_find_monster(index);
        printf("\nIT IS A %s\n", m->name );
        printf("\nTHE DANGER LEVEL IS %d!!\n", m->ferocity_factor);
    }



    // todo (rob) we need a framework hook for action routines for rooms and objects
    do_room_actions(gs);

    char input_buffer[1024];  // there's an env variable for terminal that defines this
    // fscanf(stdin, "%s", input_buffer);
    // printf("   you entered: %s\n", input_buffer);
    putchar('\n');

    char cmd_char;
    bool is_invalid_command = false;

    // process the next user command. First, check the command is valid for the current state
    do {
        printf("WHAT DO YOU WANT TO DO? ");
        fscanf(stdin, "%s", input_buffer);
        // printf("   you entered: %s\n", input_buffer);
        cmd_char = (char)toupper(input_buffer[0]);

        is_invalid_command = ! strchr(VALID_COMMANDS, cmd_char);

        if (is_invalid_command) {
            printf("INVALID COMMAND: '%c'\n", cmd_char);
            continue;
        }

        if (cmd_char == 'Q') {
            return false; // quit game
        }

        if (room_contents < 0 &&
            !( cmd_char == 'F' || cmd_char == 'R' )) {
            // if monster, can only Fight or Retreat
            printf("MONSTER! YOU MUST EITHER FIGHT OR RETREAT.\n");
            is_invalid_command = true;
            continue;
        }

        if (room_contents >= 0 &&
            ( cmd_char == 'F' || cmd_char == 'R' )) {
            // nothing to fight
            printf("THERE IS NO MONSTER.\n");
            is_invalid_command = true;
            continue;
        }

        if (cmd_char == 'C' && gs->food == 0) {
            printf("YOU HAVE NO FOOD.\n");
            is_invalid_command = true;
            continue;
        }

        if ( strchr(VALID_DIRECTIONS, cmd_char) ) {
            int direction_index = calc_room_graph_direction_index(cmd_char);
            if (ROOM_GRAPH[gs->room][direction_index] == 0) {
                printf("%s\n", BAD_MOVE_DESC[direction_index]);
                is_invalid_command = true;
                continue;
            }
            is_invalid_command = false;
            break;

        }

    } while (is_invalid_command);

    // Now process the command

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

    // 480 PRINT:PRINT:PRINT "----------------- -------------------"
    printf("\n\n----------------- -------------------\n");

    if (strchr(VALID_DIRECTIONS, cmd_char) ) {
        // move command
        int direction_index = calc_room_graph_direction_index(cmd_char);
        gs->room = ROOM_GRAPH[gs->room][direction_index];
        // if (gs->room == 11) {
        //     // Exit!!
        //     gs->completed = true;
        //     return false;
        // }
        return CONTINUE_GAME;
    }




    switch (cmd_char) {
        case 'H':
            display_help_info();
            break;
        case 'L':
            cmd_look(gs);
            break;
        case 'I':
            display_inventory(gs);
            break;
        case 'A':
            display_char_attributes(gs->stats);
            break;
        case 'O':
            display_score(gs);
            break;
        case 'B':
            //INVENTORY/PROVISIONS
            cmd_buy(gs);
            break;
        case 'C' :
            cmd_eat(gs);
            break;
        case 'T':
            cmd_take(gs);
            break;
        case 'M':
            use_magic_amulet(gs);
            break;
        case 'R':
            cmd_retreat(gs);
            break;
        case 'F':
            cmd_fight(gs);
            break;


        default: printf("UNHANDLED COMMAND '%c'\n", cmd_char);

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


    const CharBuffer *player_name = get_player_name();
    GLOBALS.player_name = player_name;

    GameState gs = {};

    initialize();
    reset(&gs, DEBUG_RAND_SEED);
    display_line("Your character attribute stats are:");
    display_char_attributes(gs.stats);
    display_line("");
    display_line("--------------------------------------------------------------------------------");
    display_line("");

    // obj_repr();
    monsters_names_repr();
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