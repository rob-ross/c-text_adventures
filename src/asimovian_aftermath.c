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

#include "mersenne_twister.h"



enum Direction {
    DIRECTION_ERR = -1,
    DIRECTION_NORTH = 0,
    DIRECTION_SOUTH,
    DIRECTION_EAST,
    DIRECTION_WEST,
    DIRECTION_UP,
    DIRECTION_DOWN,
};

// direction in "NSEWUD"
enum Direction calc_room_graph_direction_index(char const direction_char) {
    switch (toupper(direction_char)) {
        case 'N': return DIRECTION_NORTH;
        case 'S': return DIRECTION_SOUTH;
        case 'E': return DIRECTION_EAST;
        case 'W': return DIRECTION_WEST;
        case 'U': return DIRECTION_UP;
        case 'D': return DIRECTION_DOWN;
        default: return DIRECTION_ERR;
    }
}

const char * direction_string(const enum Direction direction_index) {
    switch (direction_index) {
        case DIRECTION_ERR:   return "ERROR";
        case DIRECTION_NORTH: return "NORTH";
        case DIRECTION_SOUTH: return "SOUTH";
        case DIRECTION_EAST:  return "EAST";
        case DIRECTION_WEST:  return "WEST";
        case DIRECTION_UP:    return "UP";
        case DIRECTION_DOWN:  return "DOWN";
        default:              return "UNKNOWN";
    }
}

char const * const BAD_MOVE_DESC[6] = {
    "NO EXIT THAT WAY",
    "THERE IS NO EXIT SOUTH",
    "YOU CANNOT GO IN THAT DIRECTION",
    "IN THAT WAY LIES MADNESS",
    "THERE IS NO WAY UP FROM HERE",
    "YOU CANNOT DESCEND FROM HERE",
};


// This allows text to be displayed with some probability.
struct RandomText {
    const char *text;  // displayed if chance_percent is satisifed
    const char *else_text; // if not null, displayed when chance_percent not satisfied
    double chance_percent;  // betwen 0 and 1. Random number between 0 and 1  must be less (<) than this to be displayed
};

struct RandomTextArray {
    size_t length;
    struct RandomText lines[];  // flexible array
};

struct Room {
    int id;
    char const * name;
    char const * desc;
    struct RandomTextArray  * preamble;
    struct RandomTextArray  * epilog;
};


static struct Room ROOMS[20] = {
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


enum RoomGraphIndex {
    RGINDEX_NORTH, RGINDEX_SOUTH, RGINDEX_EAST, RGINDEX_WEST, RGINDEX_UP, RGINDEX_DOWN, RGINDEX_CONTENTS,
    RGINDEX_COUNT
};

constexpr int NUM_ROOMS      = 20;
constexpr int ROOM_START     = 3;
constexpr int ROOM_END       = 6;
constexpr int POD_ROOM       = 11;
constexpr int RADIATION_ROOM = 13;


static int ROOM_GRAPH[NUM_ROOMS][RGINDEX_COUNT] = {
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

struct GameState {
    const char * player_name;
    int room; // current room
    int tally; // 1 point per move
    int strength;
    int wealth;
    int oxy;
    int monsters_killed;  // number aliens/androids destroyed
    int monsters_fought;
    int radiation_turn_count;  // when in RADIATION_ROOM, counts turns spent in the room.

    bool is_dead;
    bool completed; // true if reached final room

    // true when user has Item:
    bool items[ITEM_COUNT];
};

struct Monster {
    int FF; // ferocity factor
    char const * name;
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
char const * const VALID_DIRECTIONS = "NSEWUD";

//// ------------------------------------------------------------
////
////    Forward declarations
////
//// ------------------------------------------------------------

void initialize(struct GameState * gs);
void cleanup(struct GameState * gs);
bool main_game_loop(struct GameState * gs);
void display_conclusion(const struct GameState * gs);
void display_strength(const struct GameState * gs);
void display_score(const struct GameState * gs);
void display_line(char const* msg);
void display(char const* msg);
void display_room_desc(struct GameState * gs);
void display_room_content(struct GameState * gs);
void display_help_info(void);
void flush_input(void);
struct StringBuffer {char buffer[1024];} get_str(char const *  prompt);
char get_command_char(char const *  prompt, char const *  valid_chars, char const *  err_msg);
int get_int(char const * const prompt, int min, int max);
bool cmd_move(struct GameState * gs, char first_letter);
int calc_score(const struct GameState * gs);
void display_command_err(char const * msg, char  command);
void display_inventory(struct GameState * gs);
void display_tally(const struct GameState * gs);

void cls();

void buy_supplies(struct GameState * gs);
void consume_oxygen(struct GameState * gs);
void use_transporter(struct GameState * gs);
void pick_up_treasure(struct GameState * gs);
void retreat(struct GameState * gs);
void fight(struct GameState * gs);


//// ------------------------------------------------------------
////
////    MAIN
////
//// ------------------------------------------------------------

int main_impl(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    mt_initialize_state(&mt_state, 0);  // initialize the PRNG
    struct GameState game_state = {};
    bool continue_loop;
    initialize(&game_state);
    do {
        continue_loop = main_game_loop(&game_state);
    } while (continue_loop);


    display_conclusion(&game_state);

    display_tally(&game_state);
    cleanup(&game_state);
    return EXIT_SUCCESS;
}

#ifdef ASIMOVIAN_AFTERMATH_MAIN
int main(void) {
    return main_impl();
}
#endif


/*
*WELL DONE!

SCORE: 4950

tally: 60, strength: 930, wealth: 0, oxy: 0, monsters fought: 8, killed: 4
 */


bool main_game_loop(struct GameState * gs) {
    gs->tally++;

    printf("---------------------------------------------------------------------- %d\n", gs->tally);

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
    display_room_content(gs);
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
            display_inventory(gs);
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

        default: display_command_err("UNHANDLED COMMAND: ", first_letter);

    }

    return true;
}

int const ITEM_COSTS[] = { 0, 15, 10, 20, 2, 30, 50};

void display_inventory_menu(struct GameState * gs) {
    display("\nYOU HAVE $");
    printf("%d\n",gs->wealth);

    display_line("YOU CAN BUY 1 - NUCLEONIC LIGHT ($15)");
    display_line("            2 - ION GUN ($10)");
    display_line("            3 - LASER ($20)");
    display_line("            4 - OXYGEN ($2 PER UNIT)");
    display_line("            5 - MATTER TRANSPORTER ($30)");
    display_line("            6 - COMBAT SUIT ($50)");
    display_line("            0 - TO CONTINUE EXPLORATION");
}

void buy_supplies(struct GameState * gs) {
    display_line("\nA SUPPLY ANDROID HAS ARRIVED.");
    if (gs->wealth <=0 ) {
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
            gs->wealth -= ITEM_COSTS[option_index];
            gs->items[option_index] = true;
            if (gs->wealth < 0) {
                display_line("YOU HAVE TRIED TO CHEAT ME!");
                //punish user
                gs->wealth = 0;
                for (int i = 0; i < ITEM_COUNT; ++i) {
                    gs->items[i] = false;  // no soup for you!
                }
                gs->oxy = gs->oxy / 4 ;
            }
        }

        if (option_index == 4 ) {
            int oxy_cost = ITEM_COSTS[ITEM_OXY];
            int qty = get_int("HOW MANY UNITS OF OXYGEN? ", 0, gs->wealth / oxy_cost );
            int cost = qty * oxy_cost;
            if (gs->wealth - cost < 0 ) {
                display_line("YOU HAVEN'T GOT ENOUGH MONEY!");
            } else {
                gs->wealth -= cost;
                gs->oxy += qty;
                gs->items[ITEM_OXY] = gs->oxy > 0;
            }
        }
    }

}

void consume_oxygen(struct GameState * gs) {
    if (gs->oxy <= 0) {
        display_line("\nYOU HAVE NO OXYGEN.");
        return;
    }
    display("\nYOU HAVE ");
    printf("%d", gs->oxy);
    display_line(" UNITS OF OXYGEN.");

    int qty = get_int("HOW MANY DO YOU WANT TO CONSUME? ", 0, gs->oxy);
    gs->oxy -= qty;
    gs->strength += (5 * qty);
    gs->items[ITEM_OXY] = gs->oxy > 0;
}


void use_transporter(struct GameState * gs) {
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


void pick_up_treasure(struct GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_CONTENTS] > 0) {
        gs->wealth += ROOM_GRAPH[gs->room][RGINDEX_CONTENTS] ;
        ROOM_GRAPH[gs->room][RGINDEX_CONTENTS] = 0;
    }
}

void retreat(struct GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_CONTENTS] >= 0) {
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


void fight(struct GameState * gs) {
    if (ROOM_GRAPH[gs->room][RGINDEX_CONTENTS] >= 0) {
        return; // no monster to fight
    }

    int const monster_index = -ROOM_GRAPH[gs->room][RGINDEX_CONTENTS];
    struct Monster const monster = MONSTERS[ monster_index ];
    int ferocity_factor = monster.FF;

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
    ROOM_GRAPH[gs->room][RGINDEX_CONTENTS] = 0;
}


// first_letter must be in "NSEWUD"
// return true if command was sucessfully processed. If false, the move is not allowed and an error message
// will have been displayed
bool cmd_move(struct GameState * gs, char const first_letter) {
    const int location = gs->room;
    const int direction_index = calc_room_graph_direction_index(first_letter);
    if (ROOM_GRAPH[location][direction_index] > 0) {
        gs->room = ROOM_GRAPH[location][direction_index];
        return true;
    }

    display_line(BAD_MOVE_DESC[direction_index]);
    return false;
}

int calc_score(const struct GameState * gs) {
    return 3 * gs->tally + 5 * gs->strength + 2 * gs->wealth + 10 * gs->oxy + 30 * gs->monsters_killed;
}

//// ------------------------------------------------------------
////
////    DISPLAY FUNCTIONS
////
//// ------------------------------------------------------------


void cls() {
    // \033[2J clears the screen, \033[H moves the cursor to the top-left corner
    printf("\033[2J\033[H");
    fflush(stdout);
}




// pass -1 to sleep default time of 30 miliseconds or last time set > 0
void char_sleep(const int32_t microseconds) {
    constexpr int _30ms = 30'000;  // usleep() takes argument in microseconds
    static useconds_t current_delay = _30ms; // initial default

    useconds_t sleep_time;

    if (microseconds >= 0) {
        current_delay = microseconds;  // sticky sleep time
        sleep_time = microseconds;
    } else {
        sleep_time = current_delay;
    }

    if (sleep_time > 0) {
        usleep(sleep_time);
    }
}

//display string without adding newline
void display(char const* msg) {
    fflush(stdout);
    for (char const *next = msg; *next; ++next) {
        putchar(*next);
        fflush(stdout);
        char_sleep(-1);
    }
}

//displays the string and adds newline to end.
void display_line(char const* msg) {
    display(msg);
    putchar('\n');
    fflush(stdout);
    char_sleep(-1);
}

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

void display_random_room_text(struct RandomTextArray *rta) {
    for (int i=0; i< rta->length; ++i) {
        const struct RandomText rt = rta->lines[i];
        const double random = mt_random_double(&mt_state); // random double in [0,1)
        if (random < rt.chance_percent) {
            display_line(rt.text);
        } else if (rt.else_text) {
            display_line(rt.else_text);
        }
    }
}

void display_room_desc(struct GameState * gs) {
    display_line("");
    if (!gs->items[ITEM_LIGHT]) {
        display_line("IT IS TOO DARK TO SEE ANYTHING!\n");
    } else {
        struct Room r = ROOMS[gs->room];
        if (r.preamble) {
            display_random_room_text(r.preamble);
        }

        display_line(ROOMS[gs->room].desc);

        if (r.epilog) {
            display_random_room_text(r.epilog);
        }
    }
}

void display_room_content(struct GameState * gs) {
    const int room_content = ROOM_GRAPH[gs->room][RGINDEX_CONTENTS];
    if ( room_content == 0 ) return;  // room is empty

    if (room_content > 0 ) {
        if ( gs->items[ITEM_LIGHT] ) {
            display("THERE IS TREASURE HERE WORTH $");
            printf("%d\n", room_content);
        }
    } else if (gs->items[ITEM_LIGHT] ) {
        struct Monster m = MONSTERS[-room_content];
        display_line("\nDANGER••• THERE IS DANGER HERE•••• ");
        display("IT IS A ");
        display_line(m.name);
        display("YOUR PERSONAL DANGER METER REGISTERS ");
        printf("%d!!\n", m.FF);
    } else {
        display_line("YOU FEEL A DANGEROUS PRESENCE!");
    }
}

void display_inventory(struct GameState * gs) {
    display_line("");

    if (gs->items[ITEM_LIGHT]) {
        display_line("YOU ARE CARRYING A NUCLEONIC LIGHT.");
    }

    if (gs->wealth > 0) {
        display("YOU HAVE $");
        printf("%d", gs->wealth);
        char_sleep(-1);
        display_line(" WEALTH IN SOLARIAN CREDITS.");
    }

    if (gs->oxy > 0 ) {
        display("YOUR RESERVE TANKS HOLD ");
        printf("%d",gs->oxy);
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


void display_conclusion(const struct GameState * gs) {
    if (gs->completed && !gs->is_dead) {
        display("\nYOU HAVE SUCCEEDED, ");
        display_line(gs->player_name);
        display_line("YOU HAVE ESCAPED IN THE POD.");
        display_line("\nWELL DONE!");
    } else if (gs->is_dead) {
        display_line("YOU HAVE DIED.........");
    }
}


void display_tally(const struct GameState * gs) {
    display("\nSCORE: ");
    printf("%d\n", calc_score(gs));
    printf("\ntally: %d, strength: %d, wealth: %d, oxy: %d, monsters fought: %d, killed: %d\n",
        gs->tally, gs->strength, gs->wealth, gs->oxy, gs->monsters_fought, gs->monsters_killed);
}

void display_strength(const struct GameState * gs) {
    display("YOUR STRENGTH IS ");
    printf("%d.\n", gs->strength);
    if (gs->strength <= 20) {
        display("*** WARNING ***\nCAPTAIN ");
        display(gs->player_name);
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

void flush_input(void) {
    while (stdin_has_data()) {
        int c = getchar();
        if (c == '\n' || c == EOF) break;
    }
}


int get_int(char const * const prompt, const int min, const int max) {
    for (;;) {
        struct StringBuffer sb = get_str(prompt);
        
        // If the user just hit enter, sb.buffer[0] will be '\0'
        if (sb.buffer[0] == '\0') {
            display_line("INPUT CANNOT BE EMPTY. PLEASE ENTER A NUMBER.");
            continue;
        }

        char *endptr;
        long val = strtol(sb.buffer, &endptr, 10);

        // If endptr is the same as the buffer, no numbers were found at the start
        if (endptr == sb.buffer) {
            display_line("INVALID INPUT. PLEASE ENTER A VALID INTEGER.");
            continue;
        }

        if (val < min || val > max) {
            printf("OUT OF RANGE. PLEASE ENTER A NUMBER BETWEEN %d AND %d.\n", min, max);
            continue;
        }

        return (int)val;
    }
}

struct StringBuffer get_str(char const * const prompt) {
    struct StringBuffer sb = {};
    display(prompt);
    
    if (fgets(sb.buffer, sizeof(sb.buffer), stdin)) {
        size_t len = strlen(sb.buffer);
        if (len > 0) {
            if (sb.buffer[len - 1] == '\n') {
                // Normal case: entire line read, remove newline
                sb.buffer[len - 1] = '\0';
            } else {
                // Truncation case: buffer was too small, leftovers remain in stdin
                flush_input();
            }
        }
    }
    return sb;
}

void display_command_err(char const * msg, char const command) {
    if (!msg) {
        msg = "INVALID COMMAND: ";
    }
    display(msg);
    printf("'%c'\n", command);
}

// return the first letter of the user's input converted to uppercase.
// input char must be in the `valid_chars` string to be accepted or user is re-prompted until it is
// err_msg may be null
char get_command_char(char const * const prompt, char const * const valid_chars, char const * const err_msg) {
    char first_letter;
    bool is_invalid_command;
    do {
        struct StringBuffer sb = get_str(prompt);
        first_letter = (char)toupper(sb.buffer[0]);
        is_invalid_command = ! strchr(valid_chars, first_letter);
        if (is_invalid_command) {
            display_command_err(err_msg, first_letter);
        }
    } while (is_invalid_command);

    return first_letter;
}

//// ------------------------------------------------------------
////
////    INITIALIZE
////
//// ------------------------------------------------------------

struct RandomTextArray * create_rta(int length) {
    const size_t mem_size = sizeof(struct RandomTextArray) + sizeof(struct RandomText) * length;
    struct RandomTextArray * result = calloc(1, mem_size);
    result->length = length;
    return result;
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
    ROOMS[7].epilog->lines[1] = (struct RandomText){ .chance_percent = .5, .text="THE FEW REMAINING CREW STIR FITFULLY\nIN THEIR ENDLESS, DREAMLESS SLEEP."};
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
    ROOMS[12].epilog->lines[0] = (struct RandomText){ .chance_percent = .2, .text="YOU CAN JUST MAKE OUT EXITS\nTO THE SOUTH AND TO THE EAST."};
    // room 13
    ROOMS[13].preamble = create_rta(1);
    ROOMS[13].preamble->lines[0] = (struct RandomText){ .chance_percent = .5, .text="YOUR BODY TWISTS AND BURNS..."};
    ROOMS[13].epilog = create_rta(2);
    ROOMS[13].epilog->lines[0] = (struct RandomText){ .chance_percent = .5, .text="NO MATTER WHAT YOU DO"};
    ROOMS[13].epilog->lines[1] = (struct RandomText){ .chance_percent = .5, .text="YOU ARE DOOMED TO DIE HERE."};
    // room 14
    ROOMS[14].epilog = create_rta(2);
    ROOMS[14].epilog->lines[0] = (struct RandomText){ .chance_percent = .1, .text="YOU CAN BARELY MAKE OUT DOORS\nTO THE NORTH AND WEST."};
    ROOMS[14].epilog->lines[1] = (struct RandomText){ .chance_percent = .4, .text="A SHAFT LEADS DOWNWARDS TO THE REPAIR CENTER."};
    // room 16
    ROOMS[16].epilog = create_rta(5);
    ROOMS[16].epilog->lines[0] = (struct RandomText){ .chance_percent = .3, .text="RARE METALS AND VENUSIAN SCULPTURES"};
    ROOMS[16].epilog->lines[1] = (struct RandomText){ .chance_percent = .2, .text="PRESERVED SCALAPIAN DESERT FISH"};
    ROOMS[16].epilog->lines[2] = (struct RandomText){ .chance_percent = .3, .text="FLASHING EBONY SCITH STONES FROM XARIAX IV"};
    ROOMS[16].epilog->lines[3] = (struct RandomText){ .chance_percent = .2, .text="AWESOME TRADER ANT EFIGIES FROM THE QWERTYIOPIAN EMPIRE"};
    ROOMS[16].epilog->lines[4] = (struct RandomText){ .chance_percent = .1, .text="THE LIGHT IS STRONGER TO THE WEST"};
    // room 19
    ROOMS[19].epilog = create_rta(1);
    ROOMS[19].epilog->lines[0] =
        (struct RandomText){
            .chance_percent = .5,
            .text  = "ONE OF WHICH IS THE GRAVITY WELL.",
        .else_text = "ONE OF WHICH LEADS TO THE GOODS HOLD." };
}


void debug_room_desc() {
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

void initialize(struct GameState * gs) {
    init_rooms();

    char_sleep(15'000); // set char delay to 15 ms
    // debug_room_desc();

    gs->room = ROOM_START;
    gs->strength = mt_rand_range(&mt_state, 0, 50) + 75;
    gs->wealth   = mt_rand_range(&mt_state, 0, 50) + 50;
    gs->oxy   = mt_rand_range(&mt_state, 0, 16);

    cls();

    struct StringBuffer sb = get_str("WHAT IS YOUR NAME, EXPLORER? ");
    char * new_str = malloc(strlen(sb.buffer) + 1);
    strcpy(new_str, sb.buffer);
    gs->player_name = new_str;

    display("HELLO CAPTAIN ");
    display_line(gs->player_name);
    display_line("TYPE 'HELP' FOR LIST OF COMMANDS.");

    //allot treasure
    for (int j = 0; j < 7; ++j ) {
        for (;;) {
            // Generate a random number between 1 and 19
            const int room_index = mt_rand_range(&mt_state, 1, 20);
            if ( !(room_index == ROOM_END || room_index == POD_ROOM || room_index == RADIATION_ROOM ||
                    ROOM_GRAPH[room_index][RGINDEX_CONTENTS] != 0 ) ) {
                const int treasure = mt_rand_range(&mt_state, 10, 111); // rand val between 10 and 110 inclusive
                ROOM_GRAPH[room_index][RGINDEX_CONTENTS] = treasure;
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
                        ROOM_GRAPH[room_index][RGINDEX_CONTENTS] != 0 ) ) {
                    ROOM_GRAPH[room_index][RGINDEX_CONTENTS] = -j;
                    break;
                }
            }
        }
    }
}




//// ------------------------------------------------------------
////
////    CLEANUP
////
//// ------------------------------------------------------------

void destroy_rooms() {
    for (int room_index = 0; room_index < NUM_ROOMS; ++room_index) {
        free(ROOMS[room_index].preamble);
        free(ROOMS[room_index].epilog);
    }
}

void cleanup(struct GameState * gs) {
    destroy_rooms();
    free((void*)gs->player_name);
}