#ifndef AOC_COMMON_H
#define AOC_COMMON_H 1

#include <stdio.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

// dynamic arrays
#define DARRAY_DEFINE_TYPE(type_name, elem_type)\
    typedef struct {\
        size_t length;\
        size_t capacity;\
        elem_type *data;\
    } type_name;

#define DARRAY_PUSH(array, value)\
    do {\
        if ((array).length >= (array).capacity) {\
            if ((array).capacity) {\
                (array).capacity *= 2;\
            } else {\
                (array).capacity = 64;\
            }\
            (array).data = realloc((array).data, (array).capacity*sizeof((array).data[0]));\
        }\
        (array).data[(array).length++] = value;\
    } while(0)

#define DARRAY_REMOVE(array, idx)\
    do {\
        if (idx < (array).length) {\
            if (idx != (array).length - 1) {\
                memmove(&(array).data[idx], &(array).data[idx+1], sizeof((array).data[0]) * ((array).length - idx - 1));\
            }\
            --(array).length;\
        }\
    } while(0)

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