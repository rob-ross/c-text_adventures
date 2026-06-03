// json_parser.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/02 01:37:49 PDT





#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stddef.h>

typedef enum {
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type;

typedef struct json_value json_value;
typedef struct json_object_entry json_object_entry;

struct json_value {
    json_type type;
    union {
        int boolean;
        double number;
        char *string;
        struct {
            json_value **elements;
            size_t count;
        } array;
        struct {
            json_object_entry *entries;
            size_t count;
        } object;
    } u;
};

struct json_object_entry {
    char *key;
    json_value *value;
};

typedef struct {
    const char *message;
    int line;
    int column;
} json_error;


constexpr char QUOTATION_MARK = '"';


/* Main parsing function */
json_value *json_parse(const char *json, json_error *error);

/* Recursive cleanup */
void json_value_free(json_value *value);

#endif // JSON_PARSER_H


