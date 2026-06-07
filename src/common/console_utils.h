// console_utils.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 20:01:31 PDT



#pragma once


#include <stdint.h>
#include <stdarg.h> // Added for variadic functions
#include "string.h"

// usleep() takes argument in microseconds
// these are equivalent milliseconds
constexpr uint32_t  _1ms =  1'000;  // NOLINT(*-reserved-identifier)
constexpr uint32_t _10ms = 10'000;  // NOLINT(*-reserved-identifier)
constexpr uint32_t _15ms = 15'000;  // NOLINT(*-reserved-identifier)
constexpr uint32_t _30ms = 30'000;  // NOLINT(*-reserved-identifier)

// extern bool GLOBAL_silent_mode;
// extern uint32_t GLOBAL_char_sleep_duration;

void cls();
void flush_input(void);
void set_silent_mode(bool silent);
uint32_t set_char_sleep(uint32_t microseconds);
void char_sleep(int32_t microseconds);
void display(char const* msg );
void display_line(char const* msg );
// Add the format attribute here
void vdisplay( const char * restrict format, ... ) __attribute__((format(printf, 1, 2)));
void vdisplay_line( const char * restrict format, ...) __attribute__((format(printf, 1, 2)));
void display_paginated(char const* msg, int num_columns);
CharBuffer * get_char_buffer(char const *  prompt);
char get_command_char(char const *  prompt, char const *  valid_chars, char const *  err_msg);
int get_int(char const * prompt, int min, int max);
