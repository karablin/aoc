#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>

#include "common.h"

typedef struct {
    uint64_t min;
    uint64_t max;
} Range;

DARRAY_DEFINE_TYPE(Ranges, Range)
DARRAY_DEFINE_TYPE(IDs, uint64_t)

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

    Ranges ranges = {0};
    IDs ids = {0};

    char tmp_str[128]; 
    // load ranges till empty line
    while (fgets(tmp_str, ARRAY_LENGTH(tmp_str), f)) {
        uint64_t min, max;
        if (sscanf(tmp_str, "%"SCNu64"-%"SCNu64, &min, &max) != 2) {
            break;
        }
        Range r = { .min = min, .max = max };
        DARRAY_PUSH(ranges, r);
    }
    // load ids till end of file
    while (fgets(tmp_str, ARRAY_LENGTH(tmp_str), f)) {
        uint64_t id;
        if (sscanf(tmp_str, "%"SCNu64, &id) != 1) {
            break;
        }
        DARRAY_PUSH(ids, id);
    }

    // merge ranges inplace, naively. second option can be sort and merge in one pass, 
    // but this is shorter and i'm not sure that sort version will be faster on small dataset
    for(bool merged = true; merged;) {
        merged = false;
        for (size_t first_range_idx = 0; first_range_idx < ranges.length-1; ++first_range_idx) {
            for (size_t second_range_idx = first_range_idx + 1; second_range_idx < ranges.length; ++second_range_idx) {
                Range first_range = ranges.data[first_range_idx];
                Range second_range = ranges.data[second_range_idx];

                if (second_range.min <= first_range.max && second_range.max >= first_range.min) {
                    Range merged_range = {
                        .min = first_range.min < second_range.min ? first_range.min : second_range.min,
                        .max = first_range.max > second_range.max ? first_range.max : second_range.max,
                    };
                    ranges.data[first_range_idx] = merged_range;
                    DARRAY_REMOVE(ranges, second_range_idx);
                    merged = true;
                }
            }
        }
    };
    // count valid ids for the first task. again sorted ids and binary search should be faster, overkill for this task
    uint64_t answer1 = 0;
    for (size_t id_idx = 0; id_idx < ids.length; ++id_idx) {
        for (size_t ranges_idx = 0; ranges_idx < ranges.length; ++ranges_idx) {
            if (ids.data[id_idx] >= ranges.data[ranges_idx].min && ids.data[id_idx] <= ranges.data[ranges_idx].max) {
                ++answer1;
                break;
            }
        }
    }
    // calculate answer for the second task
    uint64_t answer2 = 0;
    for (size_t ranges_idx = 0; ranges_idx < ranges.length; ++ranges_idx) {
        answer2 += ranges.data[ranges_idx].max - ranges.data[ranges_idx].min + 1;
    }

    printf("answer 1: %"PRIu64"\n", answer1);
    printf("answer 2: %"PRIu64"\n", answer2);

    free(ranges.data);
    free(ids.data);
    fclose(f);
    return 0;
}