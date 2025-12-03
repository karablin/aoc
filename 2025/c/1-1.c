#include <stdio.h>
#include <string.h>
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

        x = (x + (num * dir)) % 100;
        answer += (x == 0);
    }
    
    printf("answer: %d\n", answer);
    fclose(f);
    return 0;
}
