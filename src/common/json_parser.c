// json_parser.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/02 01:37:49 PDT

/*
 *
*The "Master Prompt" for Generation
To get a high-quality, compliant parser from an AI, you need to be specific about the architecture and the edge cases. Here is the prompt I recommend:
"Act as a world-class systems engineer. Write a thread-safe, RFC 8259 compliant JSON parser in C.
Requirements:
1.
Architecture: Use a recursive descent parsing strategy.
2.
Data Structures: Define a json_value struct using a tagged union to represent Objects, Arrays, Strings, Numbers, Booleans, and Null.
3.
Memory: Provide a json_value_free function to recursively clean up the AST. Do not use global state.
4.
Strings: Correctly handle all escape sequences, including Unicode \uXXXX and UTF-8 encoding.
5.
Numbers: Support integers, fractions, and exponents as defined by the RFC.
6.
Error Handling: Return detailed error information, including the line number, column number, and a descriptive message.
7.
Constraints: Use only the C standard library (C99 or later).
8.
Interface: Provide a json_parse(const char *input) function that returns a pointer to the root json_value."
 **/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "json_parser.h"

typedef struct {
    const char *json;
    int line;
    int column;
} json_context;

static void skip_whitespace(json_context *ctx) {
    while (*ctx->json && isspace(*ctx->json)) {
        if (*ctx->json == '\n') {
            ctx->line++;
            ctx->column = 1;
        } else {
            ctx->column++;
        }
        ctx->json++;
    }
}

/* Implementation of specific parsers would go here (parse_string, parse_number, etc) */

static json_value *parse_value(json_context *ctx, json_error *error) {
    skip_whitespace(ctx);
    switch (*ctx->json) {
        case 'n': /* Handle null */
        case 't': /* Handle true */
        case 'f': /* Handle false */
        case '"': /* Handle string */
        case '[': /* Handle array */
        case '{': /* Handle object */
        case '-': case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': case '8': case '9':
            /* Handle number */
        default:
            if (error) {
                error->message = "Unexpected character";
                error->line = ctx->line;
                error->column = ctx->column;
            }
            return NULL;
    }
}

json_value *json_parse(const char *json, json_error *error) {
    if (!json) return NULL;

    json_context ctx = {json, 1, 1};
    return parse_value(&ctx, error);
}

void json_value_free(json_value *value) {
    if (!value) return;

    switch (value->type) {
        case JSON_STRING:
            free(value->u.string);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < value->u.array.count; i++) {
                json_value_free(value->u.array.elements[i]);
            }
            free(value->u.array.elements);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < value->u.object.count; i++) {
                free(value->u.object.entries[i].key);
                json_value_free(value->u.object.entries[i].value);
            }
            free(value->u.object.entries);
            break;
        default:
            break;
    }
    free(value);
}