// parser.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/28 17:49:00 PDT
#pragma once

#include <regex.h>

// enum ids taken from https://en-word.net

enum Command {
    CMD_NO_MATCH = -2, // couldn't parse this command string
    CMD_ERROR = -1,
    CMD_NONE  =  0,

    // these are single word direction commands. Their ids match `enum Direction`.
    CMD_NORTH = 1,
    CMD_SOUTH,
    CMD_EAST,
    CMD_WEST,
    CMD_UP,
    CMD_DOWN,

    CMD_INV,       // equivalent to "show inventory"
    CMD_STATS,     // show character attributes/stats
    CMD_SCORE,     // show current score


    CMD_READ  = 24875,  // oewn-00626756-v (Interlingual Index: i24875)
    CMD_FIGHT = 27047,  // oewn-01092746-v (Interlingual Index: i27047)
    CMD_DRINK = 27466,  // oewn-01172332-v (Interlingual Index: i27466)
    CMD_TAKE  = 27693,  // oewn-01216829-v (Interlingual Index: i27693)
    CMD_OPEN  = 28422,  // oewn-01348685-v (Interlingual Index: i28422)
    CMD_MOVE  = 30898,  // oewn-01839438-v (Interlingual Index: i30898)
    CMD_DROP  = 31618,  // oewn-01981715-v (Interlingual Index: i31618)

    CMD_LOOK  = 32408,  // oewn-02134989-v (Interlingual Index: i32408)

    CMD_PAY   = 32996,  // oewn-02256551-v (Interlingual Index: i32996)

    CMD_HELP  = 34433,  // oewn-02553283-v (Interlingual Index: i34433)
    CMD_QUIT  = 35062,  // oewn-02686624-v (Interlingual Index: i35062)

    CMD_GOD,
};

constexpr size_t PC_BUFFER_LEN = 1024;
typedef struct ParsedCommand {
    enum Command verb_command;
    enum Command verb_object_command;  // normally CMD_NONE for an identifier/name
    bool has_verb_object; // true if verb_object field is not empty
    char verb[PC_BUFFER_LEN];
    char verb_object[PC_BUFFER_LEN];  // object of the verb in a sentence.
} ParsedCommand;

// Define a structure to hold regex patterns and their compiled forms
typedef struct {
    const char *pattern_string;
    regex_t compiled_regex;
    const char *name; // For easier identification in output
    enum Command cmd;
} RegexPattern;

constexpr int COMPILE_SUCCESS = 0;


//// ------------------------------------------------------------
////
////    API FUNCTIONS
////
//// ------------------------------------------------------------


// Compiles each regex pattern
// Returns COMPILE_SUCCESS if no errors, otherwise returns error code > 0.
int parser_init(void);
// Free all the compiled regex patterns.
// Must be called at the end of app execution
void parser_free_resources();

// parses the argument and returns a struct ParsedCommand.
// If .verb_command < 0, an error has occurred in parsing
struct ParsedCommand parse_cmd_string( char const * str);