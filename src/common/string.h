// string.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 20:05:40 PDT



#pragma once
#include <sys/_types/_size_t.h>

typedef struct CharBuffer {
    size_t length;
    char const buffer[]; // flexible array
} CharBuffer;


// Returns true if `str` strats with `prefix`, ignoring case.
bool string_starts_with_ignore_case(const char *prefix, const char *str);