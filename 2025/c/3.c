#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include "common.h"

#define BANK_SIZE 128

void calculate(const size_t max_battery_cnt, FILE *f)
{
    uint64_t answer = 0;
    char bank[BANK_SIZE];

    while (fgets(bank, ARRAY_LENGTH(bank), f)) {
        size_t bank_len = 0;
        while(isdigit(bank[bank_len])) ++bank_len; // avoid line ending and other non-digit stuff at the line end

        char result_str[BANK_SIZE]; 
        char *current_bank_pos = bank; 

        for (size_t bat_num = 0; bat_num < max_battery_cnt; ++bat_num) {
            // find max possible joltage for battery starting from left side (most significant) 
            // until rightmost limit (see below)
            for(char joltage = '9'; joltage > '0'; --joltage) {
                char *p = strchr(current_bank_pos, joltage);
                // check that max possible joltage was found between leftmost possible position (not taken 
                // by previos batteries) and rightmost possible pos (allowing space for rest of batteries)
                if(p && p < (bank + bank_len - (max_battery_cnt - 1) + bat_num)) {
                    result_str[bat_num] = joltage;
                    current_bank_pos = p + 1;
                    break;
                }
            }
        }
        result_str[max_battery_cnt] = '\0';
        // convert and add
        uint64_t result_joltage;
        sscanf(result_str, "%"SCNu64, &result_joltage);
        answer += result_joltage;
    }
    
    printf("answer: %"PRIu64"\n", answer);
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

    printf("part 1");
    calculate(2, f);

    printf("part 2");
    rewind(f);
    calculate(12, f);

    fclose(f);
    return 0;
}
