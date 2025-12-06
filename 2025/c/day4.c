#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct {
    unsigned short width;
    unsigned short height;
    char *data;
} RollsMap;

void load_map(RollsMap *map, FILE* f);
void print_map(RollsMap *map);

int is_roll(RollsMap *map, int row, int col) {
    if (row < 0 || col < 0 || row >= map->height || col >= map->width) {
        return 0;
    }
    return map->data[row * map->width + col] == '.' ? 0 : 1;
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

    RollsMap map = {0};
    load_map(&map, f);
    // print_map(&map);

    int answer1 = 0, answer2 = 0, rolls_found;
    bool first_pass = true;

    do {
        rolls_found = 0;
        // search for rolls to remove
        for (int row = 0; row < map.height; ++row) {
            for(int col = 0; col < map.width; ++col) {
                if (!is_roll(&map, row, col)) {
                    continue;
                }
                int rolls_cnt =
                    is_roll(&map, row - 1, col - 1) + is_roll(&map, row - 1, col) + is_roll(&map, row - 1, col + 1) +
                    is_roll(&map, row    , col - 1) +               0             + is_roll(&map, row    , col + 1) +
                    is_roll(&map, row + 1, col - 1) + is_roll(&map, row + 1, col) + is_roll(&map, row + 1, col + 1);

                if (rolls_cnt < 4) {
                    ++rolls_found;
                    map.data[row * map.width + col] = 'x'; // mark roll for removal on end of the pass
                }
            }
        }
        if (first_pass) {
            answer1 = rolls_found;
            first_pass = false;
        }
        answer2 += rolls_found;
        // print_map(&map);
        // remove 'x' rolls from map
        for(int pos = 0; pos < map.height * map.width; ++pos) {
            if (map.data[pos] == 'x') map.data[pos] = '.';
        }
    } while (rolls_found);

    printf("answer1: %d\n", answer1);
    printf("answer2: %d\n", answer2);

    free(map.data);
    fclose(f);
    return 0;
}

void load_map(RollsMap *map, FILE* f)
{
    // get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    // allocate large enough buffer
    map->data = malloc(file_size);

    fread(map->data, 1, file_size, f);
    // remove newline, spaces, etc from data, and calculate width and height
    unsigned short calc_width = 0;
    int dst = 0;
    for (int src = 0; src < file_size; ++src) {
        if (!map->width && isspace(map->data[src])) {
            map->width = calc_width;
        }
        ++calc_width;
        if (!isspace(map->data[src])) {
            map->data[dst++] = map->data[src];
        }
    }
    map->height = dst / map->width;
}

void print_map(RollsMap *map) {
    for (size_t row = 0; row < map->height; ++row) {
        fwrite(map->data + row * map->width, 1, map->width, stdout);
        puts("");
    }
}
