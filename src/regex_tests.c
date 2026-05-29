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
    -std=c23 -o regex_tests.out regex_tests.c common/console_utils.c common/string.c parser.c

*/

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

constexpr int MATCH_FOUND = 0;


int match_one_pattern(const RegexPattern rp, char const * str, struct ParsedCommand *pc_out) {
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

int try_match(size_t len, const RegexPattern rp[restrict len], char const * str, struct ParsedCommand *pc_out) {
    for (int pattern_index = 0; pattern_index < len; ++pattern_index ) {
        RegexPattern pattern = rp[pattern_index];
        if (match_one_pattern(pattern, str, pc_out) == MATCH_FOUND) {
            return MATCH_FOUND;
        }
    }
    return REG_NOMATCH;
}


int main(void) {
    int error = parser_init();
    if ( error ) {
        printf("Failed to init parser. error:%d. Exiting\n", error);
        return 1;
    }

    // char * tests[] = {
    //     "quit", "bribe", "bribe dwarf", "drop foo", "drop chest", "drop chest of iron",
    //     "go", "go ", "go e", "go east", "e", "east", " east", "east ", " east ",
    //     " north", "south ", "  west  ", "N", " S", "E ", " D ", // Added more direction tests
    //     "go fuck yourself", " fuck yourself",  "no match pattern",
    //
    //     nullptr  // sentinel value, end of array
    // };
    char * tests[] = {
        " n ", "s", " e", "w ", "U", "D",
        "north", "NORTH", " north", "norht", "nor", "north ", "  north   ",

        nullptr  // sentinel value, end of array
    };



    int num_tests = sizeof(tests) / sizeof(tests[0]) - 1;

    // test each string
    for (int i = 0; i < num_tests; ++i) {
        struct ParsedCommand pc = {};
        pc = parse_cmd_string( tests[i] );
        printf("parsed '%s':\n    verb_command:%d, verb_object_command:%d,\n    verb:'%s', verb_object:'%s'\n",
                   tests[i], pc.verb_command, pc.verb_object_command, pc.verb, pc.verb_object);
    }


    parser_free_resources();
    return 0;
}
