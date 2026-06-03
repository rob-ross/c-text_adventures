// string.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/23 20:05:40 PDT

// Jambooty
// Created by Rob Ross on 5/23/26.
//

#include "string.h"

#include <ctype.h>
#include <string.h>


// Returns true if str strats with prefix, ignoring case.
bool string_starts_with_ignore_case(const char *prefix, const char *str) {
    // If partial_string is NULL, it cannot be a prefix of anything.
    if ( !prefix || !str ) {
        return false;
    }

    // If partial_string is empty, it is not considered a prefix.
    if (*prefix == '\0') {
        return false;
    }

    // If full_string is empty and partial_string is not empty, then partial_string cannot be a prefix.
    if (*str == '\0') {
        return false;
    }

    // Now both strings are guaranteed to be non-NULL and non-empty.
    size_t i = 0;
    while (prefix[i] != '\0' && str[i] != '\0') {
        // Safely convert char to unsigned char before passing to toupper
        if (toupper((unsigned char)prefix[i]) != toupper((unsigned char)str[i])) {
            return false; // Mismatch found
        }
        i++;
    }

    // If we reached the end of partial_string, it means all characters matched.
    // So, full_string starts with partial_string.
    return prefix[i] == '\0';
}


void string_trim(char *s) {
    if (s == NULL || *s == '\0') {
        return;
    }

    size_t len = strlen(s);
    char *p = s;

    // Trim trailing whitespace
    while (len > 0 && isspace((unsigned char)p[len - 1])) {
        p[--len] = 0;
    }

    // Trim leading whitespace
    while (*p && isspace((unsigned char)*p)) {
        ++p;
        --len;
    }

    // Move the trimmed string back to the start
    memmove(s, p, len + 1);
}