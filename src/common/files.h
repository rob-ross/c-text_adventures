// files.h
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/02 22:18:43 PDT


#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


// declares a function pointer type, `file_process_action`
// the function takes a file pointer and a handle to a void to return the result of the
// function operation. The function returns an int error code, 0 if no error.
typedef int (*file_process_action)( FILE *fptr, void **result_ptr);

// resource guard function. Wraps the function call between calls to open and close
// to ensure the file is always cleaned up.
int process_file(char const * file_name, file_process_action function, void **result_ptr);

#ifdef __cplusplus
}
#endif


