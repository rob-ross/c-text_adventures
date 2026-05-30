// parser.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/28 17:49:00 PDT


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"



// -----------------------------------------------------------------
//      REGULAR EXPRESSION STRINGS
// -----------------------------------------------------------------

// RSL: "Regex Start of Line"
#define SP  "[[:space:]]*"
#define RSL "^" SP
#define REL SP "$"
// VOBJ: verb's object
#define VOBJ "(.*)"

static const char * const REGEX_HELP   = RSL "(HELP)" REL;
static const char * const REGEX_QUIT   = RSL "(QUIT)" REL;
static const char * const REGEX_UNLOCK = RSL "(UNLOCK)";
static const char * const REGEX_OPEN   = RSL "(OPEN)" SP VOBJ REL;
static const char * const REGEX_READ   = RSL "(READ)";

static const char * const REGEX_NORTH  = RSL "(NORTH|N)" REL;
static const char * const REGEX_SOUTH  = RSL "(SOUTH|S)" REL;
static const char * const REGEX_EAST   = RSL "(EAST|E)" REL;
static const char * const REGEX_WEST   = RSL "(WEST|W)" REL;
static const char * const REGEX_UP     = RSL "(UP|U)" REL;
static const char * const REGEX_DOWN   = RSL "(DOWN|D)" REL;

static const char * const REGEX_BRIBE  = RSL "(BRIBE|PAY)(.*)";
static const char * const REGEX_DRINK  = RSL "(DRINK|SWALLOW)(.*)";

static const char * const REGEX_DROP = RSL "(DROP|PUT|THROW|BREAK)" SP VOBJ REL;
static const char * const REGEX_TAKE = RSL "(TAKE|GET|STEAL|LIFT)"  SP VOBJ REL;

static const char * const REGEX_MOVE = RSL "(GO|MOVE|CLIMB|RUN|WALK)" SP VOBJ REL;

static const char * const REGEX_DIRECTION = RSL "(NORTH|SOUTH|EAST|WEST|UP|DOWN|N|S|E|W|U|D)" REL;
static const char * const REGEX_FIGHT     = RSL "(FIGHT|STAB|KILL|KICK|PUNCH|SLAY|ATTACK)(.*)" REL;

#undef VOBJ
#undef REL
#undef RSL
#undef SP

// -----------------------------------------------------------------
//      COMPILED PATTERNS
// -----------------------------------------------------------------
// patterns: order of these matters. Single word direction commands need to be tested before REGEX_MOVE

static RegexPattern patterns[] = {
    {REGEX_HELP,        {}, "HELP",      CMD_HELP  },
    {REGEX_QUIT,        {}, "QUIT",      CMD_QUIT  },
    {REGEX_UNLOCK,      {}, "UNLOCK" },
    {REGEX_OPEN,        {}, "OPEN",      CMD_OPEN  },
    {REGEX_READ,        {}, "READ" },

    {REGEX_NORTH,       {}, "NORTH", CMD_NORTH },
    {REGEX_SOUTH,       {}, "SOUTH", CMD_SOUTH },
    {REGEX_EAST,        {}, "EAST",  CMD_EAST },
    {REGEX_WEST,        {}, "WEST",  CMD_WEST },
    {REGEX_UP,          {}, "UP",    CMD_UP },
    {REGEX_DOWN,        {}, "DOWN",  CMD_DOWN },


    {REGEX_BRIBE,       {}, "BRIBE"},
    {REGEX_DRINK,       {}, "DRINK"},
    {REGEX_DROP,        {}, "DROP",       CMD_DROP  },
    {REGEX_TAKE,        {}, "TAKE",       CMD_TAKE  } ,
    {REGEX_MOVE,        {}, "MOVE",       CMD_MOVE  },
    {REGEX_FIGHT,       {}, "FIGHT",      CMD_FIGHT },

    // DO NOT ADD NEW ITEMS BELOW REGEX_DIRECTION
    {REGEX_DIRECTION,   {}, "DIRECTION"},
    // {REGEX_VERB_OBJECT, {}, "VERB_OBJECT"           }   ,
    {nullptr, {}, nullptr, CMD_NONE}  // sentinel value, end of array
};
constexpr int NUM_PATTERNS = sizeof(patterns) / sizeof(patterns[0]) - 1;

// todo (rob) these indices are fragile. We need a map structure to hold these
constexpr int REGEX_NORTH_INDEX =  5;
constexpr int REGEX_DOWN_INDEX  = 10;

// constexpr int REGEX_VERB_OBJECT_INDEX = NUM_PATTERNS - 1;
constexpr int REGEX_DIRECTION_INDEX   = NUM_PATTERNS - 2;

// This will hold the patterns for the directions,
// from patterns[REGEX_NORTH_INDEX] to patterns[REGEX_DOWN_INDEX] inclusive.
// initialized in init_parser()
static RegexPattern direction_patterns[6] = {};

constexpr int MATCH_FOUND = 0;
static bool is_initialized = false;  // set to true when init_parser() is called.

static int match_one_pattern(const RegexPattern rp, char const * str, struct ParsedCommand *pc_out) {
    constexpr size_t max_groups = 4;
    // The first element (0) is the entire match, subsequent elements are capture groups
    regmatch_t pmatch[max_groups] = {}; // Assuming max_groups - 1 capture groups + full match
    const int reti = regexec(&rp.compiled_regex, str, max_groups, pmatch, 0);
    if ( reti == MATCH_FOUND) {
        // debug
        // printf("match found for %s\n", pattern.name);

        pc_out->verb_command = rp.cmd;
        if (pmatch[1].rm_so != -1) {
            // capture group 1
            int len1 = (int)(pmatch[1].rm_eo - pmatch[1].rm_so);
            strncpy(pc_out->verb,  str + pmatch[1].rm_so, len1);
            pc_out->verb[PC_BUFFER_LEN - 1] = 0; // defensive null termination

            if (pmatch[2].rm_so != -1) {
                // capture group 2
                int len2 = (int)(pmatch[2].rm_eo - pmatch[2].rm_so);
                strncpy(pc_out->verb_object,  str + pmatch[2].rm_so, len2);
                pc_out->verb_object[PC_BUFFER_LEN - 1] = 0; // defensive null termination
            }
        }
    }
    return reti;
}

static int try_match(size_t len, const RegexPattern rp[restrict len], char const * str, struct ParsedCommand *pc_out) {
    if (! is_initialized) {
        parser_init();
    }

    for (int pattern_index = 0; pattern_index < len; ++pattern_index ) {
        RegexPattern pattern = rp[pattern_index];
        if (match_one_pattern(pattern, str, pc_out) == MATCH_FOUND) {
            return MATCH_FOUND;
        }
    }
    return REG_NOMATCH;
}


// Compiles each regex pattern
// Returns COMPILE_SUCCESS (0) if no errors, otherwise returns error code > 0.
int parser_init(void) {

    int reti = COMPILE_SUCCESS;
    for (int i = 0; i < NUM_PATTERNS; i++) {
        reti = regcomp(&patterns[i].compiled_regex, patterns[i].pattern_string, REG_ICASE | REG_EXTENDED);
        if ( reti != COMPILE_SUCCESS) {
            char msgbuf[100];
            regerror(reti, &patterns[i].compiled_regex, msgbuf, sizeof(msgbuf));
            fprintf(stderr, "Regex compilation failed for '%s': %s\n", patterns[i].pattern_string, msgbuf);
            return reti; // Exit early on compilation error
        }
    }

    //copy direction patterns to their own array
    for (int i = 0; i < 6; ++i) {
        direction_patterns[i] = patterns[i + REGEX_NORTH_INDEX];
    }
    is_initialized = true;
    return COMPILE_SUCCESS;
}

// Free all the compiled regex patterns.
// Must be called at the end of app execution
void parser_free_resources() {
    for (int i = 0; i < NUM_PATTERNS; i++) {
        regfree(&patterns[i].compiled_regex);
    }
    is_initialized = false;
}

struct ParsedCommand parse_cmd_string( char const * str) {
    struct ParsedCommand pc = {};
    int result = try_match(NUM_PATTERNS, patterns, str, &pc);
    if ( result != MATCH_FOUND ) {
        pc.verb_command = CMD_NO_MATCH;
    } else {
        // for move commands, capture the verb_object_command for the direction
        if (pc.verb_command == CMD_MOVE) {
            struct ParsedCommand dpc = {};
            int match_result =     try_match(6, direction_patterns, pc.verb_object, &dpc);
            if (match_result == MATCH_FOUND) {
                strcpy(pc.verb_object, dpc.verb);
                pc.verb_object_command = dpc.verb_command;
            }
        } else if ( pc.verb_command >= CMD_NORTH && pc.verb_command <= CMD_DOWN ) {
            //primary verb was a direction, transform to a CMD_MOVE with a direction object
            strcpy(pc.verb_object, pc.verb);
            pc.verb_object_command = pc.verb_command;
            strcpy(pc.verb, "go");
            pc.verb_command = CMD_MOVE;
        }
        // printf("match for '%s': verb_command:%d, verb_object_command:%d,   verb:'%s', verb_object:'%s'\n",
        //     tests[i], pc.verb_command, pc.verb_object_command, pc.verb, pc.verb_object);
    }
    pc.has_verb_object = strlen(pc.verb_object) ? true : false;
    return pc;
}

int num_patterns() {
    return NUM_PATTERNS;
}