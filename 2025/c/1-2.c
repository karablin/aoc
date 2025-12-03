#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "common.h"

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

    char cmd_buf[10];
    int answer = 0;
    int x = 50;

    while (fgets(cmd_buf, ARRAY_LENGTH(cmd_buf), f)) {
        int num, dir;
        dir = cmd_buf[0] == 'L' ? -1 : 1;
        sscanf(cmd_buf+1, "%d", &num);

        int old_x = x;
        int delta = num*dir;
        int full_circles = abs(delta) / 100;
        int remainder = delta % 100;

        // calculate new x (can be negative, but thats ok, only need to check for both -100 and +100 bounds later)
        x = (x + delta) % 100; 
        // each full circle passes through 0
        answer += full_circles;
        // check if non-full circle remainder passed through 0/+100/-100
        bool passed_through_zero =
            (old_x + remainder >= 100) ||
            (old_x + remainder <= -100) ||
            (old_x < 0 && old_x + remainder >= 0) ||
            (old_x > 0 && old_x + remainder <= 0);

        if (passed_through_zero) {
            answer++;
        }
    }
    
    printf("answer: %d\n", answer);
    fclose(f);
    return 0;
}
