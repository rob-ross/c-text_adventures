// warewolves_and_wanderer.c
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
    int room; // current room
    const char * player_name;// name of player
    int tally; // 1 point per move
    int strength;
    int wealth;
    int food;
    int monsters_killed; // numer monsters killed

    // true when user has Item:
    bool items[ITEM_COUNT];

    /*bool DUMMY;
    bool LIGHT;
    bool AXE;
    bool HAS_FOOD;
    bool SWORD;
    bool AMULET;
    bool SUIT;   */

    bool is_dead;
    bool completed; // true if reached final room
};

struct Monster {
    int FF; // ferocity factor
    char const * name;
};

static struct Monster MONSTERS[4] = {
    { .FF =  5, .name = "FEROCIOUS WEREWOLF"},
    { .FF = 10, .name = "FANATICAL FLESHGORG"},
    { .FF = 15, .name = "MALOVENTY MALDEMER"},
    { .FF = 20, .name = "DEVASTATING ICE-DRAGON"},
};

enum Idx {
    NORTH, SOUTH, EAST, WEST, UP, DOWN, CONTENTS
};

// direction in "NSEWUD"
static int calc_room_graph_direction_index(char const direction) {
    switch (direction) {
        case 'N': return NORTH;
        case 'S': return SOUTH;
        case 'E': return EAST;
        case 'W': return WEST;
        case 'U': return UP;
        case 'D': return DOWN;
        default: return -1;
    }
}

static int ROOM_GRAPH[20][7] = {
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

struct Room {
    int id;
    char const * name;
    char const * desc;
};

static struct Room ROOMS[20] = {
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
};


char const * const BAD_MOVE_DESC[6] = {
    "NO EXIT THAT WAY",
    "THERE IS NO EXIT SOUTH",
    "YOU CANNOT GO IN THAT DIRECTION",
    "YOU CANNOT MOVE THROUGH SOLID STONE",
    "THERE IS NO WAY UP FROM HERE",
    "YOU CANNOT DESCEND FROM HERE",
};

// FORWARD REFERENCES
static bool major_handling_routine(struct GameState * gs);
static void display_room_desc(struct GameState * gs);
static void initialize(struct GameState * gs);
static void display_line(char const* msg);
static void display_score(const struct GameState * gs);

int main_warewolves_and_wanderer(void) {
    srand( time(nullptr) );

    // 10 REM WEREWOLVES AND WANDERER
    struct GameState game_state = { .room = 6, .player_name = "Joe", .strength = 105, .wealth = 75,  };
    bool continue_loop = true;
    initialize(&game_state); // 20 GOSUB 2600  (initialize)
    do {
        continue_loop = major_handling_routine(&game_state);  // 30 GOSUB 160
    } while (continue_loop);

    if (game_state.completed && !game_state.is_dead) {
        display_line("YOU'VE DONE IT!!");
        display_line("THAT WAS THE EXIT FROM THE CASTLE");
        printf("\nYOU HAVE SUCCEEDED, %s!\n", game_state.player_name);
        display_line("\nYOU MANAGED TO GET OUT OF THE CASTLE");
        display_line("\nWELL DONE!");
    } else if (game_state.is_dead) {
        display_line("YOU HAVE DIED.........");
    }

    display_score(&game_state);

    //140 END
    return EXIT_SUCCESS;
}

#ifdef WAREWOLVES_AND_WANDERER_MAIN
int main(void) {
    return main_warewolves_and_wanderer();
}
#endif

static void display_line(char const* msg) {
    fflush(stdout);
    constexpr int _30ms = 30'000;
    for (char const *next = msg; *next; ++next) {
        putchar(*next);
        fflush(stdout);
        usleep(_30ms);
    }
    putchar('\n');
    fflush(stdout);
}

static void display_score(const struct GameState * gs) {
    display_line("\nYOUR SCORE IS");
    printf("%d\n", 3* gs->tally + 5* gs->strength + 2* gs->wealth + gs->food + 30*gs->monsters_killed);
}

static void actor_display_inventory(struct GameState * gs) {
    if (gs->wealth > 0) {
        printf("YOU HAVE $%d WEALTH\n", gs->wealth);
    }

    if (gs->food > 0 ) {
        printf("YOUR PROVISIONS SACK HOLDS %d UNITS OF FOOD\n", gs->food);
    }

    if (gs->items[ITEM_SUIT]) {
        display_line("YOU ARE WEARING ARMOR");
    }

    if (gs->items[ITEM_ION] || gs->items[ITEM_LASER] || gs->items[ITEM_TRANSPORTER]) {
        printf("YOU ARE CARRYING ");
    }

    if (gs->items[ITEM_ION]) {
        printf("AN AXE ");
    }

    if (gs->items[ITEM_LASER]) {
        printf("A SWORD ");
    }

    if (gs->items[ITEM_TRANSPORTER] && (gs->items[ITEM_ION] || gs->items[ITEM_LASER]) ) {
        printf("AND ");
    }

    if (gs->items[ITEM_TRANSPORTER]) {
        printf("THE MAGIC AMULET");
    }

    putchar('\n');
}

//990 REM ROOM DESCRIPTION
static void display_room_desc(struct GameState * gs) {
    printf("\n%s", ROOMS[gs->room].desc);
    if (gs->room == 9) {
        // if in Room 9 transition to Room 10 and show description
        gs->room = 10;
        display_room_desc(gs);
    }
}

// first_letter must be in "NSEWUD"
// return true if command was sucessfully processed. If false, the move is not allowed.
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
    printf("\nYOU HAVE $%d\n", gs->wealth);

    printf("YOU CAN BUY 1 - FLAMING TORCH ($15)\n");
    printf("            2 - AXE ($10)\n");
    printf("            3 - SWORD ($20)\n");
    printf("            4 - FOOD($2 PER UNIT)\n");
    printf("            5 - MAGIC AMULET ($30)\n");
    printf("            6 - SUIT OF ARMOR ($50)\n");
    printf("            0 - TO CONTINUE ADVENTURE\n");
}

static void do_inventory(struct GameState * gs) {
    printf("PROVISIONS AND INVENTORY\n");
    if (gs->wealth <=0 ) {
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

        if ( option_index != 4 ) {
            gs->wealth -= ITEM_COSTS[option_index];
            gs->items[option_index] = true;
            if (gs->wealth < 0) {
                printf("YOU HAVE TRIED TO CHEAT ME!\n");
                //punish user
                gs->wealth = 0;
                for (int i = 0; i < ITEM_COUNT; ++i) {
                    gs->items[i] = false;  // no soup for you!
                }
                gs->food = gs->food / 4 ;
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
                int cost = qty * ITEM_COSTS[ITEM_OXY];
                if (gs->wealth - cost < 0 ) {
                    printf("YOU HAVEN'T GOT ENOUGH MONEY!\n");
                } else {
                    gs->wealth -= cost;
                    gs->food += qty;
                    break;
                }
            }
        }
    }

}


static void eat_food(struct GameState * gs) {
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

static void pick_up_treasure(struct GameState * gs) {
    if ( ROOM_GRAPH[gs->room][CONTENTS] <= 0 ) {
        printf("THERE IS NO TREASURE TO PICK UP.\n");
        return;
    }
    if ( !gs->items[ITEM_LIGHT] ) {
        printf("YOU CANNOT SEE WHERE IT IS\n");
        return;
    }
    gs->wealth += ROOM_GRAPH[gs->room][CONTENTS];
    ROOM_GRAPH[gs->room][CONTENTS] = 0;
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

static void fight(struct GameState * gs) {
    if (ROOM_GRAPH[gs->room][CONTENTS] >= 0) {
        return; // no monster to fight
    }
    int const monster_index = -ROOM_GRAPH[gs->room][CONTENTS];
    struct Monster const monster = MONSTERS[ monster_index ];
    int ferocity_factor = monster.FF;

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
            printf("\n%s ATTACKS.\n", monster.name);
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
        printf("\n\nAND YOU MANAGED TO KILL THE %s\n", monster.name);
        gs->monsters_killed++;
    } else {
        printf("\n\nTHE %s DEFEATED YOU!.\n", monster.name);
        gs->strength /= 2;
    }

    ROOM_GRAPH[gs->room][CONTENTS] = 0;
}

char const * const VALID_COMMANDS = "QNSEWUDRFICPM";
char const * const VALID_DIRECTIONS = "NSEWUD";

static void retreat(struct GameState * gs) {
    if (ROOM_GRAPH[gs->room][CONTENTS] >= 0) {
        return; // no monster to retreat from
    }
    if ( (rand() % 100) > 69 ) {
        printf("NO, YOU MUST STAND AND FIGHT!\n");
        fight(gs);
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

// 160 REM MAJOR HANDLING ROUTINE
// returns true if still alive
static bool major_handling_routine(struct GameState * gs) {
    gs->strength -= 5;

    if (gs->strength <= 15) {
        printf("WARNING, %s, YOUR STRENGTH\nIS RUNNING LOW\n", gs->player_name);
    }
    if (gs->strength <= 0) {
        //goto 2300
        //REM DEATH
        gs->is_dead = true;
        return false; // is dead
    }

    gs->tally += 1;
    printf("%s, YOUR STRENGTH IS %d\n", gs->player_name, gs->strength);

    actor_display_inventory(gs);

    if (gs->items[ITEM_LIGHT]) {
        //GOSUB 990:REM ROOM DESCRIPTION
        display_room_desc(gs);
        putchar('\n');
    } else {
        display_line("IT IS TOO DARK TO SEE ANYTHING\n");
    }


    int room_contents = ROOM_GRAPH[gs->room][CONTENTS];
    if ( room_contents > 0 ) {
        printf("THERE IS TREASURE HERE WORTH $%d\n", room_contents);
    } else if (room_contents < 0 ) {
        int index = -room_contents;
        display_line("\n\nDANGER...THERE IS A MONSTER HERE....");
        printf("\nIT IS A %s\n", MONSTERS[index].name);
        printf("\nTHE DANGER LEVEL IS %d!!\n", MONSTERS[index].FF);
    }




    char input_buffer[1024];  // there's an env variable for terminal that defines this
    // fscanf(stdin, "%s", input_buffer);
    // printf("   you entered: %s\n", input_buffer);
    putchar('\n');

    char first_letter;
    bool is_invalid_command = false;

    // process the next user command. First, check the command is valid for the current state
    do {
        printf("WHAT DO YOU WANT TO DO? ");
        fscanf(stdin, "%s", input_buffer);
        // printf("   you entered: %s\n", input_buffer);
        first_letter = (char)toupper(input_buffer[0]);

        is_invalid_command = ! strchr(VALID_COMMANDS, first_letter);

        if (is_invalid_command) {
            printf("INVALID COMMAND: '%c'\n", first_letter);
            continue;
        }

        if (first_letter == 'Q') {
            return false; // quit game
        }

        if (room_contents < 0 &&
            !( first_letter == 'F' || first_letter == 'R' )) {
            // if monster, can only Fight or Retreat
            printf("MONSTER! YOU MUST EITHER FIGHT OR RETREAT.\n");
            is_invalid_command = true;
            continue;
        }

        if (room_contents >= 0 &&
            ( first_letter == 'F' || first_letter == 'R' )) {
            // nothing to fight
            printf("THERE IS NO MONSTER.\n");
            is_invalid_command = true;
            continue;
        }

        if (first_letter == 'C' && gs->food == 0) {
            printf("YOU HAVE NO FOOD.\n");
            is_invalid_command = true;
            continue;
        }

        if ( strchr(VALID_DIRECTIONS, first_letter) ) {
            int direction_index = calc_room_graph_direction_index(first_letter);
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

    // 480 PRINT:PRINT:PRINT "----------------- -------------------"
    printf("\n\n----------------- -------------------\n");

    if (strchr(VALID_DIRECTIONS, first_letter) ) {
        // move command
        int direction_index = calc_room_graph_direction_index(first_letter);
        gs->room = ROOM_GRAPH[gs->room][direction_index];
        if (gs->room == 11) {
            // Exit!!
            gs->completed = true;
            return false;
        }
        return true;
    }




    switch (first_letter) {
        case 'I':
            //INVENTORY/PROVISIONS
            do_inventory(gs);
            break;
        case 'C' :
            eat_food(gs);
            break;
        case 'P':
            pick_up_treasure(gs);
            break;
        case 'M':
            use_magic_amulet(gs);
            break;
        case 'R':
            retreat(gs);
            break;
        case 'F':
            fight(gs);
            break;


        default: printf("UNEXPECTED COMMAND '%c'\n", first_letter);

    }


    return true; // continue game
}


static void initialize(struct GameState * gs) {
    char name_buffer[1024];
    printf("WHAT IS YOUR NAME, EXPLORER? ");
    scanf("%s", name_buffer);
    fflush(stdin);
    char * new_str = malloc(strlen(name_buffer) + 1);
    strcpy(new_str, name_buffer);
    gs->player_name = new_str;
    //allot treasure
    for (int j = 0; j < 4; ++j ) {
        for (;;) {
            // Generate a random number between 1 and 19
            int room_index = (rand() % 19) + 1;
            if ( !(room_index == 6 || room_index == 11 || ROOM_GRAPH[room_index][CONTENTS] !=0 ) ) {
                int treasure = (rand() % 100) + 10;; // rand val between 10 and 109 inclusive
                ROOM_GRAPH[room_index][CONTENTS] = treasure;
                break;
            }

        }
    }
    //allot monsters
    for (int j = 0; j < 4; ++j ) {
        for (;;) {
            // Generate a random number between 1 and 19
            int room_index = (rand() % 19) + 1;
            if ( !(room_index == 6 || room_index == 11 || ROOM_GRAPH[room_index][CONTENTS] !=0 ) ) {
                ROOM_GRAPH[room_index][CONTENTS] = -j;
                break;
            }
        }
    }
    // rooms 4 and 16 get special treasures
    ROOM_GRAPH[4][CONTENTS] = 100 + (rand() % 100);
    ROOM_GRAPH[16][CONTENTS] = 100 + (rand() % 100);
}

