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
    -std=c23 -o citadel_of_pershu.out citadel_of_pershu.c mersenne_twister.c


*/


#include "mersenne_twister.h"
#include "citadel_of_pershu.h"

constexpr int DEBUG_RAND_SEED = 67;
struct GlobalState GLOBALS = {.player_name = nullptr, .char_sleep_duration = _15ms };


// --------------------------------------------------------------
//      Forward references
// --------------------------------------------------------------
static void update_perception(GameState * gs);
static int calc_score(const GameState * gs) ;
static int count_rooms_visited(const GameState * gs);
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


static void cls() {
    // \033[2J clears the screen, \033[H moves the cursor to the top-left corner
    printf("\033[2J\033[H");
    fflush(stdout);
}

/** API for ML/AI to suppress text output */
void set_silent_mode(const bool silent) {
    GLOBALS.silent_mode = silent;
}

// sets the current sleep duration (in microseconds) for future calls to char_sleep().
// returns the previous sleep duration value
static uint32_t set_char_sleep(const uint32_t microseconds) {
    const uint32_t temp = GLOBALS.char_sleep_duration;
    GLOBALS.char_sleep_duration = microseconds;
    return temp;
}


// pass -1 to sleep for GLOBALS.char_sleep_duration  (see set_char_sleep(),
// or pass a duration >0 in microseconds
// ReSharper disable once CppDFAConstantParameter
static void char_sleep(const int32_t microseconds) {
    useconds_t sleep_time;
    // ReSharper disable once CppDFAConstantConditions
    if (microseconds >= 0) {
        sleep_time = microseconds;
    } else {
        sleep_time = GLOBALS.char_sleep_duration;
    }
    if (sleep_time > 0) {
        // usleep() takes argument in microseconds
        usleep(sleep_time); // todo is this portable? What about windows?
    }
}


//display string without adding newline
static void display(char const* msg) {
    if (GLOBALS.silent_mode) return;

    fflush(stdout);
    for (char const *next = msg; *next; ++next) {
        putchar(*next);
        fflush(stdout);
        char_sleep(-1);
    }
}

//displays the string and adds newline to end.
static void display_line(char const* msg) {
    if (GLOBALS.silent_mode) return;
    display(msg);
    putchar('\n');
    fflush(stdout);
    char_sleep(-1);
}


static void display_char_attributes(const CharStats stats) {
    if (GLOBALS.silent_mode) return;
    display("Strength:  ");
    printf("%2d", stats.strength);
    display("  Charisma:     ");
    printf("%2d\n", stats.charisma);

    display("Dexterity: ");
    printf("%2d", stats.dexterity);
    display("  Intelligence: ");
    printf("%2d\n", stats.intelligence);

    display("Wisdom:    ");
    printf("%2d", stats.wisdom);
    display("  Constitution: ");
    printf("%2d\n", stats.constitution);
}


static void display_inventory(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    display_line("\nItems:");
    int item_count = 0;
    for (int bag_index = 1; bag_index < ITEM_COUNT; ++bag_index ) {
        if (gs->items[bag_index]) {
            printf("%d. ", bag_index);
            display(TREASURE_NAMES[gs->items[bag_index]]);
            display("  ");
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
    display("magic spells: ");
    printf("%d\n", gs->magic);

    if (!gs->cash) {
        display_line("You have no money.");
    } else {
        display("You have $");
        printf("%d.\n", gs->cash);
    }
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

    display_line("");
    if (!gs->has_torch && ROOM_GRAPH[gs->room][RGINDEX_TREASURE] != 1 ) {
        display_line("IT IS TOO DARK TO SEE ANYTHING!\n");
    } else {
        Room r = ROOMS[gs->room];
        if (r.preamble) {
            display_random_room_text(gs, r.preamble);
        }

        display_line(ROOMS[gs->room].desc);

        if (r.epilog) {
            display_random_room_text(gs, r.epilog);
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
        Room room =  ROOMS[gs->room];
        display(room.monster.name);
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
    Room room =ROOMS[gs->room];
    display("\nYou can see ");

    if (treasure_index > 9 ) {
        display(room.treasure.name);
        display(" worth $");
        printf("%d\n", room.treasure.value);
    } else {
        display_line(room.treasure.name);
    }
}

static void display_room_content(GameState * gs) {
    if (GLOBALS.silent_mode) return;

    display_room_monster(gs);
    display_room_treasure(gs);
}


static void display_conclusion(const GameState * gs) {
    if (GLOBALS.silent_mode) return;

    set_char_sleep(_30ms);  // so final text display is slowed down

    if (gs->completed && !gs->is_dead) {
        display("\nYou have succeeded, ");
        display_line(gs->player_name);
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
    const int rooms_visited = count_rooms_visited(gs);
    printf("\nturns: %d, cash: %d, monsters fought: %d, killed: %d, rooms: %d\n",
        gs->turns, gs->cash, gs->monsters_fought, gs->monsters_killed, rooms_visited);
    printf("You completed %3.0f%% of the quest.\n", (double)rooms_visited * 100.0 / (NUM_ROOMS - NUM_DEATH_ROOMS - 1 ) );

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
        gs->player_name, gs->room, gs->turns, gs->cash, gs->monsters_killed,gs->monsters_fought, gs->magic,
        gs->has_torch, gs->is_dead, gs->completed, gs->must_fight);
    display_char_attributes(gs->stats);
    display_inventory(gs);
    printf("\n");
    printf("Rooms visited:\n");
    for (int room=0; room < NUM_ROOMS; ++room ) {
        if (gs->rooms_visited[room]) {
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

static void flush_input(void) {
    while (stdin_has_data()) {
        int c = getchar();
        if (c == '\n' || c == EOF) break;
    }
}


static struct StringBuffer {char buffer[1024];} get_str(char const *  prompt) {
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


static int get_int(char const * const prompt, const int min, const int max) {
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


static void display_command_err(char const * msg, char const command) {
    if (!msg) {
        msg = "INVALID COMMAND: ";
    }
    display(msg);
    printf("'%c'\n", command);
}

// return the first letter of the user's input converted to uppercase.
// input char must be in the `valid_chars` string to be accepted or user is re-prompted until it is
// err_msg may be null
static char get_command_char(char const * const prompt, char const * const valid_chars, char const * const err_msg) {
    char first_letter;
    bool is_invalid_command;
    do {
        const struct StringBuffer sb = get_str(prompt);
        first_letter = (char)toupper(sb.buffer[0]);
        is_invalid_command = ! strchr(valid_chars, first_letter);
        if (is_invalid_command) {
            display_command_err(err_msg, first_letter);
        }
    } while (is_invalid_command);

    return first_letter;
}


struct StringBuffer greet_player() {
    cls();
    const struct StringBuffer sb = get_str("What is your name, explorer? ");
    display("Hello, Explorer ");
    display(sb.buffer);
    display_line(".");
    display_line("Type '[H]elp' for a list of commands.");
    return sb;
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
    // randomized text in Rooms 1, 18, 37, 39
    // room 1
    ROOMS[1].epilog = create_rta(2);
    ROOMS[1].epilog->lines[0] = (RandomText){ .chance_percent = .5, .text="There is an exit to the west."};
    ROOMS[1].epilog->lines[1] = (RandomText){ .chance_percent = .5, .text="A tunnel leads to the south."};
    // room 18
    ROOMS[18].epilog = create_rta(1);
    ROOMS[18].epilog->lines[0] = (RandomText){ .chance_percent = .5, .text="A bat flies past you, shrieking."};
    // room 37
    ROOMS[37].epilog = create_rta(2);
    ROOMS[37].epilog->lines[0] = (RandomText){ .chance_percent = .7, .text="But now it tells you there is\na hidden stairwell in the room."};
    ROOMS[37].epilog->lines[1] = (RandomText){ .chance_percent = .3, .text="The voice faintly murmurs of the door to the south."};
    // room 39
    ROOMS[39].epilog = create_rta(1);
    ROOMS[39].epilog->lines[0] = (RandomText){ .chance_percent = .6, .text="A small door leaads to the north\nand another to the east."};
}

static Treasure generate_treasure( GameState * gs, int treasure_index) {
    return (Treasure){
        .name = TREASURE_NAMES[treasure_index],
        .treasure_index = treasure_index,
        .value = rnd_range(gs, 0, 100 ) + 56};
}

// called at the start of each new game
void reset(GameState * gs, const uint32_t seed) {
    const char * player_name = GLOBALS.player_name;  // we reuse the same string
    // reset GameState
    *gs = (GameState){ .player_name = player_name, .room = START_ROOM, .cash = 100, .magic = 3 };
    
    mt_initialize_state(&gs->mt_state, seed);  // initialize the PRNG
    
    gs->stats = random_hero_stats(gs);

    //clear all monsters, treasure
    for ( int room_index = 0; room_index < NUM_ROOMS; ++room_index ) {
        // note: if we dynamically modify the edge graph we'll need to reset those edges here
        ROOM_GRAPH[room_index][RGINDEX_TREASURE] = 0;
        ROOM_GRAPH[room_index][RGINDEX_MONSTER] = 0;
        ROOMS[room_index].monster =  (Monster){};
        ROOMS[room_index].treasure =  (Treasure){};
    }

    // special treasure items
    ROOMS[START_ROOM].treasure = generate_treasure(gs, ITEM_TORCH);
    ROOM_GRAPH[START_ROOM][RGINDEX_TREASURE] = ITEM_TORCH;
    ROOMS[LIBRARY_ROOM].treasure = generate_treasure(gs, ITEM_SILVER_KEY);
    ROOM_GRAPH[LIBRARY_ROOM][RGINDEX_TREASURE] = ITEM_SILVER_KEY;
    ROOMS[GLOVE_STOREROOM].treasure = generate_treasure(gs, ITEM_GOLD_KEY);
    ROOM_GRAPH[GLOVE_STOREROOM][RGINDEX_TREASURE] = ITEM_GOLD_KEY;
    
    // allot monsters
    for (int monster_index= 1; monster_index <= 16; ++monster_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, 43 + 1);
            if ( ! ( ROOM_GRAPH[rand_room][RGINDEX_MONSTER] ||
                    rand_room == START_ROOM ||
                    rand_room == END_ROOM ||
                    rand_room == LIBRARY_ROOM ||
                    rand_room == GLOVE_STOREROOM)) {
                ROOM_GRAPH[rand_room][RGINDEX_MONSTER]  = monster_index;
                ROOMS[rand_room].monster =
                    (Monster){
                        .name = MONSTER_NAMES[monster_index],
                        .monster_index = monster_index,
                        .stats = random_monster_stats(gs)};
                break;;
                    }
        }
    }
    // allot  treasure
    for (int treasure_index = 4; treasure_index < 19; ++treasure_index ) {
        for (;;) {
            int rand_room = rnd_range(gs, 1, 43 + 1);
            if ( ! ( ROOM_GRAPH[rand_room][RGINDEX_TREASURE] || rand_room == START_ROOM || rand_room == END_ROOM  ) ) {
                ROOM_GRAPH[rand_room][RGINDEX_TREASURE] = treasure_index;
                ROOMS[rand_room].treasure = generate_treasure(gs, treasure_index);
                break;
            }
        }
    }

    update_perception(gs);
}


// once time inits. Per-game inits happen in reset()
void initialize(const char* player_name) {
    // note: random data is initialized in reset()
    char * new_str = malloc(strlen(player_name) + 1);
    strcpy(new_str, player_name);
    GLOBALS.player_name = new_str;
    init_rooms();
}




//// ------------------------------------------------------------
////
////    CLEANUP
////
//// ------------------------------------------------------------

static void destroy_rooms() {
    for (int room_index = 0; room_index < NUM_ROOMS; ++room_index) {
        free(ROOMS[room_index].preamble);
        free(ROOMS[room_index].epilog);
    }
}

static void cleanup(GameState * gs) {
    destroy_rooms();
    const char * free_ptr = gs->player_name;
    gs->player_name = nullptr;
    free((void*)free_ptr);
}





//// ------------------------------------------------------------
////
////    GAME FUNCTIONS
////
//// ------------------------------------------------------------


static int count_rooms_visited(const GameState * gs) {
    int result = 0;
    for (int i = 0; i < NUM_ROOMS; ++i ) {
        result += gs->rooms_visited[i];
    }
    return result;
}

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
static bool has_items(const GameState * gs) {
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

static bool process_quit(const GameState * gs) {
    display_line("COWARD...QUITTER....TURNCOAT.....");
    // todo (rob) ask for confirmation?
    return END_GAME;
}




// first_letter must be in "NSEWUD"
// return true if command was sucessfully processed. If false, the move is not allowed and an error message
// will have been displayed
static bool process_move_command(GameState * gs, char const first_letter) {
    const int location = gs->room;
    const int direction_index = calc_direction_index(first_letter);
    if (direction_index == DIRECTION_ERR) {
        display("Bad direction_index, first_letter='");
        printf("%c'\n", first_letter);
        return false;
    }

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

    if (treasure_index > ITEM_WAND) {
        const Treasure treasure = ROOMS[gs->room].treasure;
        gs->cash += treasure.value;
    } else {
        gs->items[treasure_index] = treasure_index;
    }

    ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = 0;
    ROOMS[gs->room].treasure = (Treasure){};
    return true;
}

// clear the monster in the current room and its entry in the ROOMS array
static void clear_monster(const GameState * gs) {
    ROOM_GRAPH[gs->room][RGINDEX_MONSTER] = 0;
    ROOMS[gs->room].monster = (Monster){};
}

/**
 * Shared Validation: Can an item be dropped here?
 * Returns true if valid, false otherwise.
 * Prints error messages only if verbose is true.
 */
static bool can_drop_item(const GameState *gs, int item_index, bool verbose) {
    if (!has_items(gs)) {
        if (verbose) display_line("You have nothing to get rid of.");
        return false;
    }
    if (ROOM_GRAPH[gs->room][RGINDEX_TREASURE]) {
        if (verbose) {
            display("There is already a ");
            display(ROOMS[gs->room].treasure.name);
            display_line(" here.");
        }
        return false;
    }
    if (item_index == 0) return true; // Cancel/No-op is valid
    if (item_index < 0 || item_index >= ITEM_COUNT || !gs->items[item_index]) {
        if (verbose) display_line("You are not carrying that item.");
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
        display_inventory(gs);
        item = get_int("Enter number of object to drop (0 for none): ", 0, 9);
        if ( !item ) {
            return true;  // exit without dropping anything
        }

        if (gs->items[item]) {
            break;
        }
        display_line("You are not carrying that item.");
    }
    return perform_action(gs, 'G', item, 0, 0);
}



/** Logic Entry Point: ML and Human both end up here */
bool drop_action(GameState *gs, int item_index) {
    // Perform the check (protects against ML typos)
    if (!can_drop_item(gs, item_index, !GLOBALS.silent_mode)) {
        return false;
    }

    if (item_index == 0) return true; // successful no-op

    gs->items[item_index] = 0;
    ROOM_GRAPH[gs->room][RGINDEX_TREASURE] = item_index;
    ROOMS[gs->room].treasure = generate_treasure(gs, item_index);
    if (item_index == ITEM_TORCH) {
        gs->has_torch = false;
    }

    return true;

}

//todo (rob) make `strategy` an enum
//return false if fight action could not be completed, otherwise return true
bool fight_action(GameState * gs, int strategy, enum StatIndex stat1, enum StatIndex stat2) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        return false;  // nothing to fight
    }    
    
    Monster m = ROOMS[gs->room].monster;
    
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
    monster_tally += m.stats.as_array[stat1];
    monster_tally += m.stats.as_array[stat2];

    if (!gs->has_torch) {
        hero_tally -= 5;  // harder to see in the dark
    }
    
    if (!GLOBALS.silent_mode) {
        display("\nThe fight starts in favor of ");
        if (hero_tally > monster_tally ) {
            display_line("you.");
        } else {
            display_line(m.name);
        }
    
        display("The ");
        display(m.name);
        display(" - ");
        printf("%d\n",monster_tally);
        display(gs->player_name);
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
                display(m.name);
                display_line(" strikes out!");
                hero_tally -= 3;
                gs->stats.strength--;
                gs->stats.charisma--;
            } break;
            case 2: {
                display("You draw the ");
                display(m.name);
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
                display(m.name);
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
                display(m.name);
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
        display(m.name);
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
// fight_action(), the ML entry point for the fight action.
static bool process_fight(GameState * gs) {
    if (!ROOM_GRAPH[gs->room][RGINDEX_MONSTER]) {
        display_line("There is nothing to fight.");
        return false;
    }
    
    Monster m = ROOMS[gs->room].monster;
    if (gs->has_torch) {
        display("\nYour opponent is a ");
        display_line(m.name);
        display_line("With the following attributes:");
        display_char_attributes(m.stats);
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
        display_line(m.name);
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
        display_line("There is nothing to retreat from.");
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
            if ( !( room_index == END_ROOM || room_index ==  WINE_CELLAR_EAST || room_index == MARBLE_HALL) ) {
                // don't retreat to end room or through locked doors
                exits[num_exits++] = room_index;
            }
        }
    }

    // randomly move to an adjacent room. If current room has paths to itself, new room may not change
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
    int dir_idx = calc_direction_index(cmd);
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
    gs->perception.current_treasure = (Treasure){};
    gs->perception.legal_actions_mask = 0;

    // Populate Action Mask for ML
    for (int i = 0; VALID_COMMANDS[i] != '\0'; ++i) {
        if (is_action_legal(gs, VALID_COMMANDS[i])) {
            gs->perception.legal_actions_mask |= (1u << i);
        }
    }

    // Only see things if the room is lit
    // Note: Treasure index 1 is the Torch itself, which is visible in the dark.
    if (gs->has_torch || treasure_index == ITEM_TORCH ) {
        if (monster_index > 0 ) {
            gs->perception.current_monster = ROOMS[room_index].monster;
            gs->perception.monster_is_visible  = true;
        }
        if (treasure_index > 0 ) {
            gs->perception.current_treasure = ROOMS[room_index].treasure;
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
    gs->rooms_visited[gs->room] = true;

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
        const bool result = process_move_command(gs, cmd);
        update_perception(gs);  // room may have changed
        return result;
    }

    bool result = false;
    switch (cmd) {
        case 'P':
            result = pick_up_treasure(gs);
            break;
        case 'F':
            result = fight_action(gs, arg1, (enum StatIndex)arg2, (enum StatIndex)arg3);
            break;
        case 'R':
            result =  process_retreat(gs);
            break;
        case 'G':
            result =  drop_action(gs, arg1);
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

    if (gs->room == END_ROOM || gs->room >= DROWNING_ROOM) {
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



static bool main_game_loop(GameState * gs) {
    uint32_t saved_sleep_duration = GLOBALS.char_sleep_duration;
    if ( gs->rooms_visited[gs->room] ) {
        // if we've already seen this room, speed up output display
        set_char_sleep(1'000);  // 1ms
    }
    // This assignment is idempotent.
    // perform_action() is the main SOAT but may not always be called from here due to early returns
    gs->rooms_visited[gs->room] = true;

    printf("---------------------------------------------------------------------- %d\n", gs->turns);

    display_status(gs);
    display_line("");
    display_room_desc(gs);

    if (check_game_over(gs)){
        set_char_sleep(saved_sleep_duration);
        return END_GAME;
    }

    display_room_content(gs);

    flush_input();
    char cmd = get_command_char("\nWhat do you want to do? ", VALID_COMMANDS, nullptr);

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
        return process_quit(gs);
    }

    if ( !monster_check(gs, cmd) ) {
        return true;
    }

    if ( cmd == 'F' ) {
        //specialized code to prompt user and gather options to pass to perform_action()
        process_fight(gs);
    } else if (cmd == 'G'){
        get_rid_of(gs);
    } else {
        // Now the human call and the ML call use the exact same entry point
        perform_action(gs, cmd, 0,0, 0);
    }

    set_char_sleep(saved_sleep_duration);

    return CONTINUE_GAME;
}

//// ------------------------------------------------------------
////
////    MAIN
////
//// ------------------------------------------------------------

int main_citadel_of_pershu(void) {
    setvbuf(stdin, nullptr, _IONBF, 0);
    GameState gs = {};

    const struct StringBuffer sb = greet_player();

    initialize(sb.buffer);
    reset(&gs, DEBUG_RAND_SEED);

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