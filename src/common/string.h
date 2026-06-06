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

typedef const char* string;

typedef struct len_str_s {
    size_t len;
    string s;
} LenStr;

typedef struct len_str_array_s {
    size_t size;
    LenStr array[]; // flexible array
} LenStrArray;

// Returns true if `str` starts with `prefix`, ignoring case.
bool string_starts_with_ignore_case(const char *prefix, const char *str);
// remove leading and trailing whitespace characters in-place, as defined by isspace()
void string_trim(char *s);