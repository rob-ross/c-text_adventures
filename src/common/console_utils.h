// console_utils.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 20:01:31 PDT



#pragma once


#include "../common/base_types.h"
#include "cu_string.h"

// char_sleep() takes argument in microseconds; these are equivalent milliseconds
constexpr u32  _1ms =  1'000;  // NOLINT(*-reserved-identifier)
constexpr u32 _10ms = 10'000;  // NOLINT(*-reserved-identifier)
constexpr u32 _15ms = 15'000;  // NOLINT(*-reserved-identifier)
constexpr u32 _30ms = 30'000;  // NOLINT(*-reserved-identifier)


void cls();
void flush_input(void);
void set_silent_mode(bool silent);
u32 set_char_sleep(u32 microseconds);
void char_sleep(s32 microseconds);
void display(char const* msg );
void display_line(char const* msg );
// Add the format attribute here
void displayf( const char * restrict format, ... ) __attribute__((format(printf, 1, 2)));
void display_linef( const char * restrict format, ...) __attribute__((format(printf, 1, 2)));
void display_paginated(char const* msg, int num_columns);
CharBuffer * get_char_buffer(char const *  prompt);
char get_command_char(char const *  prompt, char const *  valid_chars, char const *  err_msg);
int get_int(char const * prompt, int min, int max);
