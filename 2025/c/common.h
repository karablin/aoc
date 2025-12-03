#ifndef AOC_COMMON_H
#define AOC_COMMON_H 1

#include <stdio.h>

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

/*
    read from file stream into the array, up to max_count-1,
    until EOF or compare_char found. compare_char is skipped 
    and not included in the result. a '\0' character is 
    appended to the end of the string
 */
size_t read_until(char compare_char, char *array, size_t max_count, FILE *f)
{
    size_t len = 0;
    while (--max_count) {
        int chr = fgetc(f);
        if (chr == EOF || chr == compare_char) {
            array[len] = '\0';
            break;
        }
        array[len++] = chr;
    }
    return len;
}

#endif