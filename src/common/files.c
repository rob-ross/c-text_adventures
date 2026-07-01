// files.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/06/02 22:18:43 PDT

#include "files.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>



// Acts like a resource guard to run the method in the function pointer argument
// between file open and close commands.
int process_file(char const * file_name, file_process_action function, void **result_ptr) {
    FILE *fptr = nullptr;

    fptr = fopen(file_name, "rb");
    if (!fptr) {
        printf("fopen failed for %s, error:%d, ", file_name, errno);
        perror(" ");
        return errno;
    }

    // printf("file opened: %s\n", file_name);

    int result = function(fptr, result_ptr);

    if (fclose(fptr) != 0) {
        printf("fclose failed for %s, error:%d, ", file_name, errno);
        perror(" ");
        return errno;
    }

    // printf("file closed: %s\n", file_name);
    return result;
}
