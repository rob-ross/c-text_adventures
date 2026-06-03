// files.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/02 22:18:43 PDT


#pragma once


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <_string.h>

#include "string.h"


typedef struct value_pos_s {
    LenStr value;
    const char* file_name;
    int    start_char_pos;
    int    end_char_pos;
    int    line_num_start;
    int    col_start;
    int    line_num_end;
    int    col_end;
} ValuePos;

typedef int (*file_process_action)( FILE *fptr, void **result_ptr);

int create_string_array(FILE *fptr, void **result_out);
int process_file(string file_name, file_process_action function, void **result_ptr);
void free_LenStrArray(LenStrArray *lsa);

constexpr char QUOTATION_MARK = '"';
