#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "common.h"

DARRAY_DEFINE_TYPE(String, char);
DARRAY_DEFINE_TYPE(StringList, String);

bool read_line(String *s, FILE *f)
{
    s->length = 0;
    for (int c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
        DARRAY_PUSH(*s, c);
    }
    return s->length;
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

    String first_line = {0};
    String second_line = {0};
    int answer1 = 0;

    read_line(&first_line, f);
    while(read_line(&second_line, f)) {
        fwrite(first_line.data, 1, first_line.length, stdout);
        puts("");
        // do checks
        for (size_t i = 0; i < first_line.length; ++i) {
            char c = first_line.data[i];
            switch (c) {
                case 'S':
                    second_line.data[i] = '|';
                    break;
                case '|':
                    if (second_line.data[i] == '^') {
                        second_line.data[i-1] = '|';
                        second_line.data[i+1] = '|';
                        ++answer1; // beam split
                    } else {
                        second_line.data[i] = '|';
                    }
                    break;
            }
        }
        // swap lines
        String tmp = first_line;
        first_line = second_line;
        second_line = tmp;
    }
    // this is final line
    fwrite(first_line.data, 1, first_line.length, stdout);
    puts("");

    printf("answer 1: %d\n", answer1);

    free(first_line.data);
    free(second_line.data);

    fclose(f);
    return 0;
}
