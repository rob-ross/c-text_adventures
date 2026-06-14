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


// make :
// cd /Users/robross/Documents/Development/CLionProjects/text_adventures/src
//  DEBUG:
// clang -g -DASIMOVIAN_AFTERMATH_MAIN -fsanitize=address -fsanitize=leak -Wall -Werror -std=c23 -o asimovian_aftermath.out asimovian_aftermath.c mersenne_twister.c
///
//  PROD:
// clang -std=c23 -o asimovian_aftermath.out asimovian_aftermath.c mersenne_twister.c

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <poll.h>
#endif

#include "../adventure_shared.h"
#include "../directions.h"
#include "../common/console_utils.h"
#include "../mersenne_twister.h"
#include "../rooms.h"
#include "../monsters.h"
#include "../objects.h"





static  Room ROOMS[20] = {
    {.id =  0,  .name= "DEAD END", .desc = "YOU HAVE RUN OUT OF OXYGEN..."},
    {.id =  1,  .name= "ROOM 1",   .desc = "YOU ARE IN THE FORMER RECREATION.\nCENTER. EQUIPMENT FOR MUSCLE-TRAINING\nIN ZERO GRAVITY LITTERS THE AREA."},
    {.id =  2,  .name= "ROOM 2",   .desc = "THIS WAS THE REPAIR AND MAINTENANCE\nHOLD OF THE SHIP. YOU CAN ONLY LEAVE IT\nVIA THE GIANT HANGAR DOOR TO THE WEST."},
    {.id =  3,  .name= "ROOM 3",   .desc = "YOU ARE IN THE WRECKED HOLD OF A SPACE SHIP.\nTHE CAVERNOUS INTERIOR IS LITERED WITH\nFLOATING WRECKAGE, AS IF FROM SOME\nTERRIBLE EXPLOSION EONS AGO......"},
    {.id =  4,  .name= "ROOM 4",   .desc = "THE VIEW OF THE STARS FROM THIS OBSERVATION\nPLATFORM IS MAGNIFICENT, AS FAR AS THE EYE\nCAN SEE. THE SINGLE EXIT IS BACK WHERE YOU\nCAME FROM."},
    {.id =  5,  .name= "ROOM 5",   .desc = "ACRE UPON ACRE OF DRIED-UP HYDROPONIC\nPLANT BEDS STRETCH AROUND YOU. ONCE THIS\nAREA FED THE THOUSAND ON BOARD THE SHIP."},

    {.id =  6,  .name= "ROOM 6",   .desc = "YOU ARE FREE. YOU HAVE MADE IT. YOUR\nPOD SAILS FREE INTO SPACE..........."},
    {.id =  7,  .name= "ROOM 7",   .desc = "YOU ARE IN THE CREW'S SLEEPING QUARTERS."},
    {.id =  8,  .name= "ROOM 8",   .desc = "THE FORMER PASSENGER SUSPENDED ANIMATION DORMITORY..."},
    {.id =  9,  .name= "ROOM 9",   .desc = "THIS IS THE SHIP'S HOSPITAL, WHITE AND STERILE.\nA BUZZING SOUND, AND A STRANGE WARMTH COME FROM\nTHE SOUTH, WHILE A CHILL IS FELT TO THE NORTH."},
    {.id = 10, .name= "ROOM 10",   .desc = "FOOD FOR ALL THE CREW WAS PREPARED IN THIS\nGALLEY. THE REMAINS FROM PREPARATIONS OF THE\nFINAL MEAL CAN BE SEEN. DOORS LEAVE THE GALLEY\nTO THE SOUTH AND TO THE WEST."},
    {.id = 11, .name= "ROOM 11",   .desc = "AHA • • • THAT LOOKS LIKE THE SPACE POD\nNOW, AND ITS OUTSIDE DIALS\nINDICATE IT IS STILL IN PERFECT CONDITION."},
    {.id = 12, .name= "ROOM 12",   .desc = "STRANGE MACHINERY LINES THE WALLS, WHILE\nOVERHEAD, A HOLOGRAPHIC STAR MAP SLOWLY TURNS.\nTHE FLICKERING GREEN LIGHT MAKES IT\nHARD TO SEE."},
    {.id = 13, .name= "ROOM 13",   .desc = "YOU ARE CAUGHT IN A DEADLY RADIATION FIELD.\nSLOWLY YOU REALISE THIS IS THE END."},
    {.id = 14, .name= "ROOM 14",   .desc = "THIS IS THE POWER CENTER OF THE SHIP.\nTHE CHARACTERISTIC BLUE METAL LIGHT\nOF THE STILL-FUNCTIONING ION DRIVE\nFILLS THE ENGINE ROOM. THE HAZE\nMAKES IT DIFFICULT TO SEE."},
    {.id = 15, .name= "ROOM 15",   .desc = "YOU ARE STANDING IN THE ANDROID STORAGE HOLD.\nROW UPON ROW OF METAL MEN "
                                           "STAND STIFFLY AT\nATTENTION, AWAITING THE DISTINCTIVE SOUND OF\nTHEIR LONG-DEAD CAPTAIN TO SET THEM INTO MOTION.\nA LIGHT COMES FROM THE WEST AND THROUGH THE\nGRAVITY WELL SET INTO THE FLOOR."},
    {.id = 16, .name= "ROOM 16",   .desc = "ANOTHER CAVERNOUS, SEEMINGLY ENDLESS HOLD,\nTHIS ONE CRAMMED WITH GOODS FOR TRADING..."},
    {.id = 17, .name= "ROOM 17",   .desc = "A STARK, METALLIC ROOM, REEKING OF LUBRICANTS.\nWEAPONS LINE THE WALL, RANK UPON RANK. EXITS FOR\nSOLDIER ANDROIDS ARE TO THE NORTH AND THE EAST."},
    {.id = 18, .name= "ROOM 18",   .desc = "ABOVE YOU IS THE GRAVITY SHAFT LEADING TO\nTHE ENGINE ROOM. THIS IS THE SHIP REPAIR\nCENTER WITH EMERGENCY EXITS TO THE SOLDIER\nANDROIDS STORAGE AND TO THE TRADING GOODS HOLD."},
    {.id = 19, .name= "ROOM 19",   .desc = "YOU'VE STUMBLED ON THE SECRET COMMAND CENTER\nWHERE SCREENS BRING VIEWS FROM ALL AROUND\nTHE SHIP. THERE ARE TWO EXITS........"},
};



constexpr int NUM_ROOMS      = 21;
constexpr int ROOM_START     = 3;
constexpr int ROOM_END       = 6;
constexpr int POD_ROOM       = 11;
constexpr int RADIATION_ROOM = 13;


int ROOM_GRAPH[NUM_ROOMS][RGINDEX_COUNT] = {
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



static struct Monster MONSTERS[5] = {
    { .FF =  1, .name = "ELON ANDROID"},  // dummy placeholder, not actually used
    { .FF =  5, .name = "BERSERK ANDROID"},
    { .FF = 10, .name = "DERANGED DEL-FIEVIAN"},
    { .FF = 15, .name = "RAMPAGING ROBOTIC DEVICE"},
    { .FF = 20, .name = "SNIGGERING GREEN ALIEN"},
};

// state for Mersenne Twister PRNG
static MTState mt_state;

char const * const VALID_COMMANDS = "HIQBOTRFPMNSEWUD";

//// ------------------------------------------------------------
////
////    Forward declarations
////
//// ------------------------------------------------------------

static void initialize( GameState * gs);
static void cleanup( GameState * gs);
static bool main_game_loop( GameState * gs);
static void display_strength(const  GameState * gs);
static void display_score(const  GameState * gs);
static void custom_display_room_content( GameState * gs);
static void display_help_info(void);
static bool cmd_move( GameState * gs, char first_letter);
static int calc_score(const  GameState * gs);
static void custom_display_inventory(const GameState * gs, bool show_item_index, bool show_item_value );
static void display_tally(const  GameState * gs);



static void buy_supplies( GameState * gs);
static void consume_oxygen( GameState * gs);
static void use_transporter( GameState * gs);
static void pick_up_treasure( GameState * gs);
static void retreat( GameState * gs);
static void fight( GameState * gs);










int const ITEM_COSTS[] = { 0, 15, 10, 20, 2, 30, 50};

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

void buy_supplies( GameState * gs) {
    display_line("\nA SUPPLY ANDROID HAS ARRIVED.");
    if (gs->cash <=0 ) {
        display_line("YOU HAVE NO MONEY.");
        return;
    }

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

        if ( option_index != 4 ) {
            gs->cash -= ITEM_COSTS[option_index];
            gs->items[option_index] = true;
            if (gs->cash < 0) {
                display_line("YOU HAVE TRIED TO CHEAT ME!");
                //punish user
                gs->cash = 0;
                for (int i = 0; i < ITEM_COUNT; ++i) {
                    gs->items[i] = false;  // no soup for you!
                }
                gs->food = gs->food / 4 ;
            }
        }

        if (option_index == 4 ) {
            int oxy_cost = ITEM_COSTS[ITEM_OXY];
            int qty = get_int("HOW MANY UNITS OF OXYGEN? ", 0, gs->cash / oxy_cost );
            int cost = qty * oxy_cost;
            if (gs->cash - cost < 0 ) {
                display_line("YOU HAVEN'T GOT ENOUGH MONEY!");
            } else {
                gs->cash -= cost;
                gs->food += qty;
                gs->items[ITEM_OXY] = gs->food > 0;
            }
        }
    }

}

void consume_oxygen( GameState * gs) {
    if (gs->food <= 0) {
        display_line("\nYOU HAVE NO OXYGEN.");
        return;
    }
    display("\nYOU HAVE ");
    printf("%d", gs->food);
    display_line(" UNITS OF OXYGEN.");

    int qty = get_int("HOW MANY DO YOU WANT TO CONSUME? ", 0, gs->food);
    gs->food -= qty;
    gs->strength += (5 * qty);
    gs->items[ITEM_OXY] = gs->food > 0;
}


void use_transporter( GameState * gs) {
    if ( !gs->items[ITEM_TRANSPORTER]) {
        display_line("\nYOU DON'T HAVE A MATTER TRANSPORTER.");
    } else if (gs->room == RADIATION_ROOM) {
        display_line("\nNOTHING HAPPENS.");
    } else {
        for (;;) {
            // Generate a random number between 1 and 19
            int room_index = mt_rand_range(&mt_state, 1, 20);
            if ( !(room_index == ROOM_END || room_index == POD_ROOM )) {
                gs->room = room_index;
                break;
            }
        }
    }
}


void pick_up_treasure( GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_TREASURE] > 0) {
        gs->cash += ROOM_GRAPH[gs->room][RGINDEX_TREASURE] ;
        ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = 0;
    }
}

void retreat( GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] != 0) {
        display_line("THERE IS NO MONSTER HERE.");
        return; // no monster to retreat from
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

    // randomly move to an adjacent room. If current room has paths to itself, new room may not change
    int retreat_index = mt_rand_range(&mt_state, 0, num_exits);

    if ( mt_random_double(&mt_state) < .3 || num_exits == 0 || retreat_index == gs->room) {
        display_line("THE CREATURE BLOCKS YOUR PATH.");
        fight(gs);
        return;
    }

    gs->room = exits[retreat_index];
}


void fight( GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_MONSTER] != 0) {
        display_line("THERE IS NOTHING TO FIGHT.");
        return; // no monster to fight
    }

    int const monster_index = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];
    struct Monster const monster = MONSTERS[ monster_index ];
    int ferocity_factor = monster.ferocity_factor;

    display_line("");

    if (gs->items[ITEM_SUIT]) {
        display_line("YOUR ARMOR INCREASES YOUR CHANCE OF SUCCESS.");
        ferocity_factor = 3 * (ferocity_factor / 4);  //armor gives 25% more advantage
    }



    const bool has_ion   = gs->items[ITEM_ION];
    const bool has_laser = gs->items[ITEM_LASER];

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
        ferocity_factor = ferocity_factor + ferocity_factor / 5;
    } else if ( use_ion ) {
        display_line("USING THE ION GUN.");
        ferocity_factor = 4 * ferocity_factor / 5;
    } else {
        display_line("USING YOUR LASER.");
        ferocity_factor = 3 * ferocity_factor / 4;
    }

    display_line("");
    display_line("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");

    int hits_on_monster = 0;

    do {
        bool has_light = gs->items[ITEM_LIGHT];

        if ( mt_random_double(&mt_state) < .5 || !has_light ) {
            display(monster.name);
            display_line(" ATTACKS.");

            if (mt_random_double(&mt_state) < .5) {
                display_line("THE MONSTER WOUNDS YOU.");
                gs->strength -= 5;
            } else {
                if (mt_random_double(&mt_state) < .5) {
                    display_line("YOU SUCCESSFULLY BLOCK IT.");
                } else {
                    display_line("IT MISSES YOU.");
                }
            }

            if ( has_light && mt_random_double(&mt_state) < .1 ) {
                display_line("YOUR LIGHT WAS KNOCKED FROM YOUR HAND!");
                gs->items[ITEM_LIGHT] = false;
                has_light = false;
            } else if ( use_ion && mt_random_double(&mt_state) < .1 ) {
                display_line("YOU DROP YOUR ION GUN IN THE HEAT OF BATTLE!");
                gs->items[ITEM_ION] = false;
                use_ion = false;
                ferocity_factor = 5 * ferocity_factor / 4;
            } else if (use_laser && mt_random_double(&mt_state) < .2 ) {
                display_line("YOUR LASER IS KNOCKED FROM YOUR HAND!!");
                gs->items[ITEM_LASER] = false;
                use_laser = false;
                ferocity_factor = 4 * ferocity_factor / 3;
            }


        } else {
            display_line("YOU ATTACK.");
            if (mt_random_double(&mt_state) < .6) {
                display_line("YOU MANAGE TO WOUND IT.");
                ferocity_factor = 5 * ferocity_factor / 6;
                hits_on_monster++;
            } else {
                display_line("IT BLOCKS YOU.");
            }
        }

        if (mt_random_double(&mt_state) < .05) {
            display_line("Aaaaargh!!!\nRIP! TEAR! RIP!");
        }
        if (mt_random_double(&mt_state) < .05) {
            display_line("YOU WANT TO RUN, BUT YOU STAND YOUR GROUND...");
        }
        if (mt_random_double(&mt_state) < .05) {
            display_line("*&%%$#$%$%# !! @#$$! #$@! !$ $#$");
        }
        if (mt_random_double(&mt_state) < .075) {
            display_line("WILL THIS BE A BATTLE TO THE DEATH?");
        }
        if (mt_random_double(&mt_state) < .075) {
            display_line("HIS EYES FLASH FEARFULLY");
        }
        if (mt_random_double(&mt_state) < .075) {
            display_line("BLOOD DRIPS FROM HIS CLAWS");
        }
        if (mt_random_double(&mt_state) < .1) {
            display_line("YOU SMELL THE LUBRICANTS ON HIS BREATH");
        }
        if (mt_random_double(&mt_state) < .1) {
            display_line("HE STRIKES WILDLY, MADLY.............");
        }
        if (mt_random_double(&mt_state) < .1) {
            display_line("YOU HAVE NEVER FOUGHT AN OPPONENT LIKE THIS!!");
        }

    } while (mt_random_double(&mt_state) < .65);

    display_line("\n");
    const int win_chance = mt_rand_range(&mt_state, hits_on_monster, 16 + hits_on_monster);
    printf("win_chance: %d, ferocity_factor: %d\n", win_chance, ferocity_factor);
    if ( win_chance > ferocity_factor) {
        display("AND YOU MANAGED TO KILL THE ");
        display_line(monster.name);
        gs->monsters_killed++;
    } else {
        display("THE ");
        display(monster.name);
        display_line(" SERIOUSLY WOUNDS YOU.");
        gs->strength /= 2;
    }

    gs->monsters_fought++;
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
}


// first_letter must be in "NSEWUD"
// return true if command was sucessfully processed. If false, the move is not allowed and an error message
// will have been displayed
bool cmd_move( GameState * gs, char const first_letter) {
    const int location = gs->room;
    const int direction_index = calc_room_graph_direction_index(first_letter);
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



void display_help_info(void) {
    display_line("\nVALID COMMANDS ARE:\n");

    display_line("[H]ELP     [I]NVENTORY  [Q]UIT");
    display_line("[B]UY      [O]XYGEN     [T]ALLY");
    display_line("[R]ETREAT  [F]IGHT");
    display_line("[P]ICK UP  [M]ATTER TRANSPORTER");
    display_line("[N]ORTH    [S]OUTH");
    display_line("[E]AST     [W]EST");
    display_line("[U]P       [D]OWN");
}



void custom_display_room_content( GameState * gs) {
    const int treasure_id = ROOM_GRAPH[gs->room][RGINDEX_TREASURE];
    const int monster_id = ROOM_GRAPH[gs->room][RGINDEX_MONSTER];

    if ( treasure_id == 0 && monster_id == 0 ) return;  // room is empty

    if (treasure_id ) {
        if ( gs->items[ITEM_LIGHT] ) {
            display("THERE IS TREASURE HERE WORTH $");
            printf("%d\n", treasure_id);
        }
    }
    if (monster_id) {
        if (gs->items[ITEM_LIGHT] ) {
            Monster m = MONSTERS[monster_id];
            display_line("\nDANGER••• THERE IS DANGER HERE•••• ");
            display("IT IS A ");
            display_line(m.name);
            display("YOUR PERSONAL DANGER METER REGISTERS ");
            printf("%d!!\n", m.ferocity_factor);
        } else {
            display_line("YOU FEEL A DANGEROUS PRESENCE!");
        }
    }
}

void custom_display_inventory(const GameState * gs, bool show_item_index, bool show_item_value ) {
    display_line("");

    if (gs->items[ITEM_LIGHT]) {
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

    if (gs->items[ITEM_SUIT]) {
        display_line("YOU ARE WEARING BATTLE ARMOR.");
    }

    const int num_items = gs->items[ITEM_ION] + gs->items[ITEM_LASER]  + gs->items[ITEM_TRANSPORTER];

    if (num_items > 0) {
        display("YOU ARE CARRYING ");
    }

    // grammar : commas and conjunctions
    // NOTE (rob) - This won't scale well when adding more items.
    if (num_items == 1) {
        if (gs->items[ITEM_ION])         display_line("AN ION GUN.");
        else if (gs->items[ITEM_LASER])  display_line("A LASER.");
        else if (gs->items[ITEM_TRANSPORTER]) display_line("THE MATTER TRANSPORTER.");
    }

    if (num_items == 3) {
        display_line("AN ION GUN, A LASER, AND THE MATTER TRANSPORTER.");
    }

    if (num_items == 2) {
        if (gs->items[ITEM_ION]) {
            display("AN ION GUN AND");
            if (gs->items[ITEM_LASER]) display_line(" A LASER.");
            else display_line(" THE MATTER TRANSPORTER.");
        } else if (gs->items[ITEM_LASER]) {
            display_line("A LASER AND THE MATTER TRANSPORTER.");
        }
    }
}




void display_tally(const  GameState * gs) {
    display("\nSCORE: ");
    printf("%d\n", calc_score(gs));
    printf("\nturns: %d, strength: %d, cash: %d, food: %d, monsters fought: %d, killed: %d\n",
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










//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------

void reset(GameState * gs, const uint32_t seed) {

    mt_initialize_state(&mt_state, 0);  // initialize the PRNG

    gs->room = ROOM_START;
    gs->strength = mt_rand_range(&mt_state, 0, 50) + 75;
    gs->cash   = mt_rand_range(&mt_state, 0, 50) + 50;
    gs->food   = mt_rand_range(&mt_state, 0, 16);

    //allot treasure
    for (int j = 0; j < 7; ++j ) {
        for (;;) {
            // Generate a random number between 1 and 19
            const int room_index = mt_rand_range(&mt_state, 1, 20);
            if ( !(room_index == ROOM_END || room_index == POD_ROOM || room_index == RADIATION_ROOM ||
                    ROOM_GRAPH[room_index][RGINDEX_TREASURE] != 0 ) ) {
                const int treasure = mt_rand_range(&mt_state, 10, 111); // rand val between 10 and 110 inclusive
                ROOM_GRAPH[room_index][RGINDEX_TREASURE] = treasure;
                break;
                    }

        }
    }
    //allot monsters
    for (int t = 0; t < 2; ++t) {
        for (int j = 1; j < 5; ++j ) {
            for (;;) {
                // Generate a random number between 1 and 19
                const int room_index = mt_rand_range(&mt_state, 1, 20);
                if ( !(room_index == ROOM_END || room_index == ROOM_START || room_index == RADIATION_ROOM ||
                        ROOM_GRAPH[room_index][RGINDEX_MONSTER] != 0 ) ) {
                    ROOM_GRAPH[room_index][RGINDEX_MONSTER] = j;
                    break;
                        }
            }
        }
    }

}

static void init_string_assets() {
    // this will eventually be loaded from a text file
    global_string_assets.conclusion_completed = "YOU HAVE SUCCEEDED!\nYOU HAVE ESCAPED IN THE POD.\nWELL DONE!";
    global_string_assets.conclusion_died      = "YOU HAVE DIED.........";

}

void init_rooms() {
    // rooms 4, 5, 7, 8, 12, 13, 14, 16, and 19 have randomized text
    // room 4
    ROOMS[4].preamble = create_rta(1);
    ROOMS[4].preamble->lines[0] = (struct RandomText){ .chance_percent = .4, .text="WHAT A SUPERB SIGHT....... "};
    // room 5
    ROOMS[5].epilog = create_rta(2);
    ROOMS[5].epilog->lines[0] = (struct RandomText){ .chance_percent = .5, .text="THE SOLAR LAMPS ARE STILL SHINING."};
    ROOMS[5].epilog->lines[1] = (struct RandomText){ .chance_percent = .5, .text="A FEW PLANTS ARE STILL ALIVE TO THE EAST."};
    // room 7
    ROOMS[7].epilog = create_rta(3);
    ROOMS[7].epilog->lines[0] = (struct RandomText){ .chance_percent = .5, .text="MOST OF THE SLEEPING SHELLS ARE EMPTY."};
    ROOMS[7].epilog->lines[1] = (struct RandomText){ .chance_percent = .5, .text="THE FEW REMAINING CREW STIR FITFULLY IN THEIR ENDLESS, DREAMLESS SLEEP."};
    ROOMS[7].epilog->lines[2] = (struct RandomText){ .chance_percent = .3, .text="THERE ARE EXITS TO THE NORTH, EAST AND WEST."};
    // room 8
    ROOMS[8].epilog = create_rta(3);
    ROOMS[8].epilog->lines[0] = (struct RandomText){ .chance_percent = .5, .text="PASSENGERS FLOAT BY AT RANDOM."};
    ROOMS[8].epilog->lines[1] = (struct RandomText){ .chance_percent = .5, .text="IT IS ENORMOUS, IT SEEMS TO GO ON FOREVER."};
    ROOMS[8].epilog->lines[2] = (struct RandomText){ .chance_percent = .1, .text="THE ONLY EXITS ARE TO THE WEST AND SOUTH."};
    // room 12
    ROOMS[12].preamble = create_rta(1);
    ROOMS[12].preamble->lines[0] = (struct RandomText){ .chance_percent = .5, .text="THIS IS THE SHIP'S MAIN NAVIGATION ROOM."};
    ROOMS[12].epilog = create_rta(1);
    ROOMS[12].epilog->lines[0] = (struct RandomText){ .chance_percent = .2, .text="YOU CAN JUST MAKE OUT EXITS TO THE SOUTH AND TO THE EAST."};
    // room 13
    ROOMS[13].preamble = create_rta(1);
    ROOMS[13].preamble->lines[0] = (struct RandomText){ .chance_percent = .5, .text="YOUR BODY TWISTS AND BURNS..."};
    ROOMS[13].epilog = create_rta(2);
    ROOMS[13].epilog->lines[0] = (struct RandomText){ .chance_percent = .5, .text="NO MATTER WHAT YOU DO"};
    ROOMS[13].epilog->lines[1] = (struct RandomText){ .chance_percent = .5, .text="YOU ARE DOOMED TO DIE HERE."};
    // room 14
    ROOMS[14].epilog = create_rta(2);
    ROOMS[14].epilog->lines[0] = (struct RandomText){ .chance_percent = .1, .text="YOU CAN BARELY MAKE OUT DOORS TO THE NORTH AND WEST."};
    ROOMS[14].epilog->lines[1] = (struct RandomText){ .chance_percent = .4, .text="A SHAFT LEADS DOWNWARDS TO THE REPAIR CENTER."};
    // room 16
    ROOMS[16].epilog = create_rta(5);
    ROOMS[16].epilog->lines[0] = (struct RandomText){ .chance_percent = .3, .text="RARE METALS AND VENUSIAN SCULPTURES"};
    ROOMS[16].epilog->lines[1] = (struct RandomText){ .chance_percent = .2, .text="PRESERVED SCALAPIAN DESERT FISH"};
    ROOMS[16].epilog->lines[2] = (struct RandomText){ .chance_percent = .3, .text="FLASHING EBONY SCITH STONES FROM XARIAX IV"};
    ROOMS[16].epilog->lines[3] = (struct RandomText){ .chance_percent = .2, .text="AWESOME TRADER ANT EFFIGIES FROM THE QWERTYIOPIAN EMPIRE"};
    ROOMS[16].epilog->lines[4] = (struct RandomText){ .chance_percent = .1, .text="THE LIGHT IS STRONGER TO THE WEST"};
    // room 19
    ROOMS[19].epilog = create_rta(1);
    ROOMS[19].epilog->lines[0] =
        (struct RandomText){
            .chance_percent = .5,
            .text  = "ONE OF WHICH IS THE GRAVITY WELL.",
        .else_text = "ONE OF WHICH LEADS TO THE GOODS HOLD." };
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

    monsters_init("monsters.txt");
    const int num_monsters = monsters_num_monsters();
    for (int i = 1; i < num_monsters; ++i) {
        Monster *m = monsters_find_monster(i);
        m->ferocity_factor = 5 * i;
    }

    ObjectData od = get_object_data();
    obj_init(od.size, od.data);

    init_string_assets();

    init_rooms();




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

    return true;
}

static bool main_game_loop( GameState * gs) {
    gs->turns++;

    printf("---------------------------------------------------------------------- %d\n", gs->turns);

    if (gs->room == RADIATION_ROOM ) {
        gs->radiation_turn_count++;
    }

    gs->strength -= 5;

    if ( gs->radiation_turn_count == 2 || gs->strength < 1 ) {
        if (gs->strength < 1) {
            display_line("YOU HAVE RUN OUT OF OXYGEN....");
        } else {
            display_line("RADIATION DESTROYS YOUR BODY...");
        }
        gs->is_dead = true;
        return false;
    }

    display_line("");
    display_strength(gs);
    display_room_desc(gs);

    if (gs->room == ROOM_END) {
        gs->completed = true;
        return false;
    }
    custom_display_room_content(gs);
    const int room_contents = ROOM_GRAPH[gs->room][RGINDEX_CONTENTS];

    char first_letter;
    bool is_invalid_command;
    bool user_moved = false;  // set to true if user successfully moved to a new room

    do {
        is_invalid_command = false;
        first_letter = get_command_char("\nWHAT DO YOU WANT TO DO? ", VALID_COMMANDS, nullptr);
        putchar('\n');

        if (first_letter == 'Q') {
            return false; // quit game
        }

        if (room_contents < 0 &&
                !( first_letter == 'F' || first_letter == 'R' ) ) {
            // if monster, can only Fight or Retreat
            display_line("DANGER! YOU MUST EITHER FIGHT OR RETREAT.");
            is_invalid_command = true;
            continue;
        }
        if (room_contents >= 0 &&
            ( first_letter == 'F' || first_letter == 'R' )) {
            // nothing to fight
            display_line("THERE IS NOTHING TO FIGHT.");
            is_invalid_command = true;
            continue;
        }

        if ( strchr(VALID_DIRECTIONS, first_letter) ) {
            if ( cmd_move(gs, first_letter)) {
                user_moved = true;
            } else {
                is_invalid_command = true;
                continue;
            }
        }

    } while (is_invalid_command);


    if (user_moved) return true;

    switch (first_letter) {
        case 'H':
            display_help_info();
            break;
        case 'I':
            custom_display_inventory(gs);
            break;
        case 'B':
            buy_supplies(gs);
            break;
        case 'O' :
            consume_oxygen(gs);
            break;
        case 'T' :
            display_tally(gs);
            break;
        case 'P':
            pick_up_treasure(gs);
            break;
        case 'M':
            use_transporter(gs);
            break;
        case 'R':
            retreat(gs);
            break;
        case 'F':
            fight(gs);
            break;

        default: display_linef("UNHANDLED COMMAND: %c", first_letter);

    }

    return true;
}


static int main_asimovian_aftermath(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);

    if (GLOBALS.debug_mode) {
        set_char_sleep(GLOBALS.debug_normal_sleep);
    } else {
        set_char_sleep(GLOBALS.char_sleep_duration);
    }



    const CharBuffer *player_name = get_player_name();
    GLOBALS.player_name = player_name;

    GameState gs = {};

    display("HELLO CAPTAIN ");
    display_line(gs.player_name->buffer);
    display_line("TYPE 'HELP' FOR LIST OF COMMANDS.");





    initialize(&gs);

    bool continue_loop;
    do {
        continue_loop = main_game_loop(&gs);
    } while (continue_loop);


    display_conclusion(&gs);

    display_tally(&gs);
    cleanup(&gs);
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
    for (int room_index = 0; room_index < NUM_ROOMS; ++room_index) {
        struct Room r = ROOMS[room_index];
        putchar('\n');
        display_line(r.name);
        display_line("---------------------------------");
        if (r.preamble) {
            display_line("PREAMBLE");
            for (int i = 0; i < r.preamble->length; ++i) {
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
            for (int i = 0; i < r.epilog->length; ++i) {
                display_line(r.epilog->lines[i].text);
                if (r.epilog->lines[i].else_text) {
                    display_line(r.epilog->lines[i].else_text);
                }
            }
        }

    }
}


