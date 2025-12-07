#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

#include "common.h"

DARRAY_DEFINE_TYPE(UInt64Array, uint64_t);
DARRAY_DEFINE_TYPE(String, char);
DARRAY_DEFINE_TYPE(NumberLines, UInt64Array);
DARRAY_DEFINE_TYPE(StringLines, String);

int main(int argv, char* argc[])
{
    if (argv != 2) {
        printf("no input file specified!\n");
        return -1;
    }

    FILE *f = fopen(argc[1], "r");
    if (!f) {
        printf("can't open file %s\n", argc[1]);
        return -1;
    }

    StringLines file_lines = {0};
    size_t max_len = 0;
    // read lines
    while(true) {
        int c;
        String tmp = {0};

        for (c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
            DARRAY_PUSH(tmp, c);
        }
        DARRAY_PUSH(file_lines, tmp);
        max_len = MAX(max_len, tmp.length);
        if (c == EOF) {
            break;
        }
    }
    // make all strings the same length, simplifying next step
    for (size_t i = 0; i < file_lines.length; ++i) {
        while(file_lines.data[i].length < max_len + 1) {
            DARRAY_PUSH(file_lines.data[i], ' ');
        }
    }
    // why we should go from right to left, as task suggests?
    // it works in both directions, i prefer to do it from left to right
    uint64_t answer1 = 0;
    uint64_t answer2 = 0;

    char current_operator;

    UInt64Array tsk1_numbers = {0};
    for (size_t line_idx = 0; line_idx < file_lines.length - 1; ++line_idx) {
        DARRAY_PUSH(tsk1_numbers, 0);
    }

    UInt64Array tsk2_numbers = {0};

    for (size_t col = 0; col <= max_len; ++col) {
        // all spaces means data for operation received
        bool all_spaces = true;

        uint64_t tsk2_curr_number = 0;

        for (size_t line_idx = 0; line_idx < file_lines.length; ++line_idx) {
            char c = file_lines.data[line_idx].data[col];
            
            if (line_idx != (file_lines.length - 1)) {
                // digits line
                if (isdigit(c)) {
                    tsk1_numbers.data[line_idx] = tsk1_numbers.data[line_idx] * 10 + (c - '0');
                    tsk2_curr_number = tsk2_curr_number * 10 + (c - '0');
                }
            } else {
                // operators line
                current_operator = c != ' ' ? c : current_operator;
            } 
            // check, if column consists only of spaces
            if (c != ' ') {
                all_spaces = false;
            }
        }
        if (!all_spaces) {
            DARRAY_PUSH(tsk2_numbers, tsk2_curr_number);
        } else {
            // time to do math
            uint64_t result;
            // task 1
            result = tsk1_numbers.data[0];
            for (size_t i = 1; i< tsk1_numbers.length; ++i) {
                result = current_operator == '+' ? result + tsk1_numbers.data[i] : result * tsk1_numbers.data[i];
            }
            answer1 += result;
            // task 2
            result = tsk2_numbers.data[0];
            for (size_t i = 1; i < tsk2_numbers.length; ++i) {
                result = current_operator == '+' ? result + tsk2_numbers.data[i] : result * tsk2_numbers.data[i];
            }
            answer2 += result;
            // cleanup for the next iteration
            tsk2_numbers.length = 0; 
            for (size_t i = 0; i < tsk1_numbers.length; ++i) tsk1_numbers.data[i] = 0;
        }
    }

    printf("answer 1: %"PRIu64"\n", answer1);
    printf("answer 2: %"PRIu64"\n", answer2);

    for (size_t i = 0; i < file_lines.length; ++i) {
        free(file_lines.data[i].data);
    }

    free(file_lines.data);
    free(tsk1_numbers.data);
    free(tsk2_numbers.data);

    fclose(f);
    return 0;
}
