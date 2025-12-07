#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

#include "common.h"

enum {
    TREE_START = -1,
    TREE_SPLIT = -2,
};

DARRAY_DEFINE_TYPE(IntArray, int64_t);

// read and parse into int array (allows to count combinations inplace)
bool read_line(IntArray *a, FILE *f)
{
    a->length = 0;
    for (int c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
        int64_t v = 0;
        if (c == 'S') {
            v = TREE_START;
        } else if (c == '^') {
            v = TREE_SPLIT;
        }
        DARRAY_PUSH(*a, v);
    }
    return a->length;
}

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

    IntArray first_line = {0}, second_line = {0};
    uint64_t answer1 = 0, answer2 = 0;

    /*
        propagate combinations down. '^' adds number above to the left and right, 
        sum of values in last line is the answer for part 2. answer 1 is '^' count.

        .......S.......
        .......1.......
        ......1^1......
        ......1.1......
        .....1^2^1.....
        .....1.2.1.....
        ....1^3^3^1....
        ....1.3.3.1....
        ...1^4^331^1...
        ...1.4.331.1...
    */

    read_line(&first_line, f);
    while(read_line(&second_line, f)) {
        for (size_t i = 0; i < first_line.length; ++i) {
            int64_t v = first_line.data[i];
            if (v == TREE_START) {
                second_line.data[i] = 1;
            } else if (v > 0) {
                if (second_line.data[i] == TREE_SPLIT) {
                    second_line.data[i-1] = second_line.data[i-1] + v;
                    second_line.data[i+1] = second_line.data[i+1] + v;
                    ++answer1; // beam split
                } else {
                    second_line.data[i] = second_line.data[i] + v;
                }
            }
        }
        // swap lines
        IntArray tmp = first_line;
        first_line = second_line;
        second_line = tmp;
    }
    // this is final line, calcualte answer 2
    for(size_t i = 0; i < first_line.length; ++i) {
        answer2 += first_line.data[i];
    }

    printf("answer 1: %"PRIu64"\n", answer1);
    printf("answer 2: %"PRIu64"\n", answer2);

    free(first_line.data);
    free(second_line.data);

    fclose(f);
    return 0;
}
