#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
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
    
    char buf[64];        // reasonable buffer, to avoid dynamic allocation and related complexity
    uint64_t answer = 0; // answer doesn't fit into 32-bit integers, using 64-bit

    // process ranges one by one
    while(read_until(',', buf, ARRAY_LENGTH(buf), f)) {
        // parse min/max IDs
        uint64_t min_id, max_id;
        if (sscanf(buf, "%"SCNu64"-%"SCNu64, &min_id, &max_id) != 2) {
            printf("something went wrong when parsing id range: %s\n", buf);
            break;
        }

        for (uint64_t id = min_id; id <= max_id; ++id) {
            // convert ID to string
            char id_str[32];
            size_t id_len = snprintf(id_str, ARRAY_LENGTH(id_str), "%"PRIu64, id);
            
            // skip if count of digits is uneven
            if (id_len % 2 != 0) {
                continue;
            }

            // compare left and right parts of the number
            bool equal = true;
            for(size_t left_idx = 0, right_idx = id_len / 2; left_idx < id_len / 2; ++left_idx, ++right_idx) {
                if (id_str[left_idx] != id_str[right_idx]) {
                    equal = false;
                    break;
                }
            }
            if (equal) {
                answer += id;
                printf("%" PRIu64 "\n", id);
            }
        }
    }

    printf("answer: %" PRIu64 "\n", answer);
    fclose(f);
    return 0;
}
