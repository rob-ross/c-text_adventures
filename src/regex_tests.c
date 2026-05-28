// regex.c
//
//
// regex pattern strings for chateau_gaillard parser
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/27 23:35:29 PDT


// make :
// cd /Users/robross/Documents/Development/CLionProjects/text_adventures/src

/*
 * DEBUG:
clang -g  -fsanitize=address -fsanitize=leak -Wall -Werror \
    -Wno-unused-const-variable -Wno-unused-variable -Wno-unused-function \
    -std=c23 -o regex_tests.out regex_tests.c common/console_utils.c common/string.c

*/

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chateau_gaillard.h"

// Define a structure to hold regex patterns and their compiled forms
typedef struct {
    const char *pattern_string;
    regex_t compiled_regex;
    const char *name; // For easier identification in output
    enum Command cmd;
} RegexPattern;

const char * const REGEX_HELP   = "HELP";
const char * const REGEX_QUIT   = "QUIT";
const char * const REGEX_UNLOCK = "UNLOCK";
const char * const REGEX_OPEN   = "OPEN";
const char * const REGEX_READ   = "READ";

const char * const REGEX_NORTH  = "^[[:space:]]*(NORTH|N)[[:space:]]*$";
const char * const REGEX_SOUTH  = "^[[:space:]]*(SOUTH|S)[[:space:]]*$";
const char * const REGEX_EAST   = "^[[:space:]]*(EAST|E)[[:space:]]*$";
const char * const REGEX_WEST   = "^[[:space:]]*(WEST|W)[[:space:]]*$";
const char * const REGEX_UP     = "^[[:space:]]*(UP|U)[[:space:]]*$";
const char * const REGEX_DOWN   = "^[[:space:]]*(DOWN|D)[[:space:]]*$";

const char * const REGEX_BRIBE = "(BRIBE|PAY)(.*)";
const char * const REGEX_DRINK = "(DRINK|SWALLOW)(.*)";

const char * const REGEX_DROP = "(DROP|PUT|THROW|BREAK)(.*)";
const char * const REGEX_TAKE = "(TAKE|GET|STEAL|LIFT)(.*)";

const char * const REGEX_MOVE      = "(GO|MOVE|CLIMB|RUN|WALK)(.*)";
// Changed \\s* to literal ' *' to explicitly match spaces
const char * const REGEX_DIRECTION = "^[[:space:]]*(NORTH|SOUTH|EAST|WEST|UP|DOWN|N|S|E|W|U|D)[[:space:]]*$";

const char * const REGEX_FIGHT = "(FIGHT|STAB|KILL|KICK|PUNCH|SLAY|ATTACK)(.*)";

// Note: REGEX_VERB_OBJECT currently requires leading whitespace.
// If you intend to match a verb followed by an object without requiring leading space,
// you might want to adjust this pattern (e.g., remove '^\\s+').
const char * const REGEX_VERB_OBJECT = "^[[:space:]]+(.*)";

constexpr int MATCH_FOUD = 0;

void test_parsing(RegexPattern patterns[], char const * test_string) {
    RegexPattern *p = &patterns[0];
    bool did_match = false;
    char const * verb;   // capture group 1
    char const * object;
    struct ParsedCommand pc = {};
    while (p->pattern_string) {
        // Maximum number of capture groups to report
        // The first element (0) is the entire match, subsequent elements are capture groups
        regmatch_t pmatch[10]; // Assuming max 9 capture groups + full match
        int reti = regexec(&p->compiled_regex, test_string, 10, pmatch, 0);
        if ( reti == MATCH_FOUD) {
            did_match = true;
            pc.command = p->cmd;
            if (pmatch[1].rm_so != -1) {
                // capture group 1
                int len = (int)(pmatch[1].rm_eo - pmatch[1].rm_so);
                // pc.verb is 0 initialized, so the character after the last copied is 0
                strncpy(pc.verb,  test_string + pmatch[1].rm_so, len);
            }
            break;
        }
        p++;
    }

    if (! did_match) {
        printf("No match for '%s'\n", test_string);
    } else {
        printf("match for '%s': command:%d, verb;%s\n", test_string, pc.command, pc.verb);
    }

}

int mult_test(void) {
    char * tests[] = {
        "quit", "bribe", "bribe dwarf", "drop foo", "drop chest", "drop chest of iron",
        "go e", "go east", "e", "east", " east", "east ", " east ",
        " north", "south ", "  west  ", "N", " S", "E ", " D ", // Added more direction tests
        "go fuck yourself", " fuck yourself",  "no match pattern",

        nullptr  // sentinel value, end of array
    };
    int num_tests = sizeof(tests) / sizeof(tests[0]) - 1;

    RegexPattern patterns[] = {
        {REGEX_HELP,        {}, "HELP",      CMD_HELP  },
        {REGEX_QUIT,        {}, "QUIT",      CMD_QUIT  },
        {REGEX_UNLOCK,      {}, "UNLOCK"},
        {REGEX_OPEN,        {}, "OPEN"},
        {REGEX_READ,        {}, "READ"},
        {REGEX_BRIBE,       {}, "BRIBE"},
        {REGEX_DRINK,       {}, "DRINK"},
        {REGEX_DROP,        {}, "DROP",       CMD_DROP  },
        {REGEX_TAKE,        {}, "TAKE",       CMD_TAKE  } ,
        {REGEX_MOVE,        {}, "MOVE",       CMD_MOVE  },
        {REGEX_DIRECTION,   {}, "DIRECTION"},
        {REGEX_FIGHT,       {}, "FIGHT",      CMD_FIGHT },
        {REGEX_VERB_OBJECT, {}, "VERB_OBJECT"           }   ,

        {nullptr, {}, nullptr, CMD_NONE}  // sentinel value, end of array
    };
    int num_patterns = sizeof(patterns) / sizeof(patterns[0]) - 1;

    // Compile each regex pattern
    for (int i = 0; i < num_patterns; i++) {
        int reti = regcomp(&patterns[i].compiled_regex, patterns[i].pattern_string, REG_ICASE | REG_EXTENDED);
        if (reti) {
            char msgbuf[100];
            regerror(reti, &patterns[i].compiled_regex, msgbuf, sizeof(msgbuf));
            fprintf(stderr, "Regex compilation failed for '%s': %s\n", patterns[i].pattern_string, msgbuf);
            return 1; // Exit on compilation error
        }
    }

    // Test each string
    for (int i = 0; i < num_tests; i++) {
        printf("Testing string: '%s'\n", tests[i]);
        int match_found = 0;

        for (int j = 0; j < num_patterns; j++) {
            // Maximum number of capture groups to report
            // The first element (0) is the entire match, subsequent elements are capture groups
            regmatch_t pmatch[10]; // Assuming max 9 capture groups + full match

            int reti = regexec(&patterns[j].compiled_regex, tests[i], 10, pmatch, 0);
            if (!reti) { // Match found
                printf("  Matches pattern: %s ('%s')\n", patterns[j].name, patterns[j].pattern_string);
                match_found = 1;

                // Print capture groups
                for (int k = 0; k < 10; k++) {
                    if (pmatch[k].rm_so == -1) {
                        break; // No more capture groups
                    }
                    int len = pmatch[k].rm_eo - pmatch[k].rm_so;
                    if (len > 0) {
                        printf("    Group %d: '%.*s'\n", k, len, tests[i] + pmatch[k].rm_so);
                    }
                }
                break; // Move to the next test string once a match is found
            } else if (reti != REG_NOMATCH) {
                char msgbuf[100];
                regerror(reti, &patterns[j].compiled_regex, msgbuf, sizeof(msgbuf));
                fprintf(stderr, "Regex match failed for '%s' with pattern '%s': %s\n", tests[i], patterns[j].pattern_string, msgbuf);
            }
        }

        if (!match_found) {
            printf("  No pattern matched for '%s'\n", tests[i]);
        }
        printf("\n");
    }



    // Free compiled regex patterns
    for (int i = 0; i < num_patterns; i++) {
        regfree(&patterns[i].compiled_regex);
    }

    return 0;
}

int main(void) {
    char * tests[] = {
        "quit", "bribe", "bribe dwarf", "drop foo", "drop chest", "drop chest of iron",
        "go e", "go east", "e", "east", " east", "east ", " east ",
        " north", "south ", "  west  ", "N", " S", "E ", " D ", // Added more direction tests
        "go fuck yourself", " fuck yourself",  "no match pattern",

        nullptr  // sentinel value, end of array
    };
    int num_tests = sizeof(tests) / sizeof(tests[0]) - 1;

    //
    // patterns: order of these matters. Single word direction commands need to be parsed before
    RegexPattern patterns[] = {
        {REGEX_HELP,        {}, "HELP",      CMD_HELP  },
        {REGEX_QUIT,        {}, "QUIT",      CMD_QUIT  },
        {REGEX_UNLOCK,      {}, "UNLOCK"},
        {REGEX_OPEN,        {}, "OPEN"},
        {REGEX_READ,        {}, "READ"},

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
        {REGEX_DIRECTION,   {}, "DIRECTION"},
        {REGEX_FIGHT,       {}, "FIGHT",      CMD_FIGHT },
        {REGEX_VERB_OBJECT, {}, "VERB_OBJECT"           }   ,

        {nullptr, {}, nullptr, CMD_NONE}  // sentinel value, end of array
    };
    int num_patterns = sizeof(patterns) / sizeof(patterns[0]) - 1;

    // Compile each regex pattern
    for (int i = 0; i < num_patterns; i++) {
        int reti = regcomp(&patterns[i].compiled_regex, patterns[i].pattern_string, REG_ICASE | REG_EXTENDED);
        if (reti) {
            char msgbuf[100];
            regerror(reti, &patterns[i].compiled_regex, msgbuf, sizeof(msgbuf));
            fprintf(stderr, "Regex compilation failed for '%s': %s\n", patterns[i].pattern_string, msgbuf);
            return 1; // Exit on compilation error
        }
    }

    // test each string
    for (int i = 0; i < num_tests; ++i) {
        test_parsing(patterns, tests[i]);
    }

    // Free compiled regex patterns
    for (int i = 0; i < num_patterns; i++) {
        regfree(&patterns[i].compiled_regex);
    }

    return 0;
}
