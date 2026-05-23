// chateau_gaillard.c
//
// ported by Rob Ross
// from a BASIC text adventure by Tim Hartnell, 1983
//
//
// Created by Rob Ross on 5/22/26.


// make :
// cd /Users/robross/Documents/Development/CLionProjects/text_adventures/src

/*
 * DEBUG:
clang -g -DCHATEAU_GAILLARD_MAIN -fsanitize=address -fsanitize=leak -Wall -Werror \
    -Wno-unused-const-variable -Wno-unused-variable -Wno-unused-function \
    -std=c23 -o chateau_gaillard.out chateau_gaillard.c mersenne_twister.c

*/
#include "chateau_gaillard.h"

#include <stdlib.h>

void init_rooms(void) {
    // random text for rooms 4,
    // special code for room 5 QU=2, SC=50, room 13 CH=CH-1, room 29 QU=3.5, room 30 SC=10, QU=3.4, room 31 sc=20, QU=3
    // room 32 counts down from 10 to 1 as you die from a spider bite, SC=3, QU=5, room 37 SC=0  QU=3
}

int main_chateau_gaillard(void) {

    return EXIT_SUCCESS;
}

// main() is defined when running this TU stand-alone and including -DCHATEAU_GAILLARD_MAIN compiler flag.
#ifdef CHATEAU_GAILLARD_MAIN
int main(void) {
    return main_chateau_gaillard();
}
#endif