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
            printf("something went wrong when parsing ids range: %s\n", buf);
            break;
        }

        for (uint64_t id = min_id; id <= max_id; ++id) {
            // convert ID to string
            char id_str[32];
            size_t id_len = snprintf(id_str, ARRAY_LENGTH(id_str), "%"PRIu64, id);

            // process number for each integer number of groups (e.g. for 1234567890 - group count will be 2,5, and 10)
            bool found_fake_id = false; 
            for (uint64_t num_groups = 2; num_groups <= id_len; ++num_groups) {
                // only if divides without remainder
                if (id_len % num_groups != 0) {
                    continue;
                }
                uint64_t group_chunk_len = id_len / num_groups;
                // compare groups in pairs, from first to last-1
                // if all pairs is equal, then ID is fake
                bool all_groups_equal = true;
                for (size_t group_idx = 0; group_idx < (num_groups-1)*group_chunk_len; group_idx += group_chunk_len) {
                    // compare strings of two groups
                    if (strncmp(&id_str[group_idx], &id_str[group_idx + group_chunk_len], group_chunk_len) != 0) {
                        all_groups_equal = false;
                        break;
                    }
                }
                if (all_groups_equal) {
                    found_fake_id = true;
                    break;
                }
            }
            if (found_fake_id) {
                answer += id;
                printf("%" PRIu64 "\n", id);                
            }
        }
    }

    printf("answer: %" PRIu64 "\n", answer);
    fclose(f);
    return 0;
}