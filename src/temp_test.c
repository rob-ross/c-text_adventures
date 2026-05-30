// temp_test.c
//
// Copyright (c) Rob Ross 2026.
//
//
// Created 2026/05/29 14:30:52 PDT

#include <stdio.h>
#include <string.h>

int main(void) {
    char const *a = "str1";
    char const *b = "str2";

    int result = strcmp(a, b);
    printf("strcmp('%s', '%s') = %2d\n", a, b, strcmp(a, b));
    // ouput: strcmp('str1', 'str2') = -1  -> "str1" < "str2"


    printf("strcmp('%s', '%s') = %2d\n", b, a, strcmp(b, a));
    // output: strcmp('str2', 'str1') =  1  -> "str2" > "str1

    printf("strcmp('%s', '%s') = %2d\n", a, a, strcmp(a, a));
    // output: strcmp('str1', 'str1') =  0  -> "str1" == "str1"


    printf("strncmp('%s', '%s', %d) = %2d\n", a, b, 3, strncmp(a, b, 3));
    // ouput: strncmp('str1', 'str2', 3) =  0  -> first 3 chars are equal

    printf("strncmp('%s', '%s', %d) = %2d\n", a, b, 1, strncmp(a, b, 1));
    // output: strncmp('str1', 'str2', 1) =  0  -> first characters are equal

    printf("strncmp('%s', '%s', %d) = %2d\n", "", b, 1, strncmp("", b, 1));
    // output: strncmp('', 'str2', 1) = -115

    printf("strncmp('%s', '%s', %d) = %2d\n", "", b, 2, strncmp("", b, 2));



    printf("strncmp('%s', '%s', %d) = %2d\n", "", b, 0, strncmp("", b, 0));
    // output: strncmp('', 'str2', 0) =  0  -> empty strings are equal.
}
