#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "common.h"

typedef struct {
    size_t x;
    size_t y;
} Point2D;
DARRAY_DEFINE_TYPE(Point2DArray, Point2D);

static bool read_point2d(Point2D *p, FILE *f) {
    size_t coord[2] = {0};
    size_t coord_idx = 0;
    for (int c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
        if (c == ',') {
            ++coord_idx;
            continue;
        }
        coord[coord_idx] = coord[coord_idx] * 10 + c - '0';
    }
    if (coord_idx != 1) {
        return false;
    }
    p->x = coord[0];
    p->y = coord[1];
    return true;
}

static bool point_inside_poly(Point2DArray points, Point2D p)
{
    // point is on the line segment. Or on the right side number of vertical line segments is even
    size_t lines_on_right = 0;
    for (size_t i = 0; i < points.length; ++i) {
        Point2D p1 = points.data[i];
        Point2D p2 = points.data[i+1 == points.length ? 0 : i+1];
        
        size_t x_left = MIN(p1.x, p2.x);
        size_t x_right = MAX(p1.x, p2.x);
        size_t y_top = MIN(p1.y, p2.y);
        size_t y_bottom = MAX(p1.y, p2.y);
        // point lays on line segment ?
        bool point_on_line = 
            (y_top == y_bottom && y_top == p.y && p.x >= x_left && p.x <= x_right) ||
            (x_left == x_right && x_left == p.x && p.y >= y_top && p.y <= y_bottom);
        if (point_on_line) {
            return true;
        }
        // count vertical line segments to the right.
        // ignoing top starting points, because in some cases they produce false positives
        if (x_left == x_right && p.y > y_top && p.y <= y_bottom && p.x < x_left) {
            ++lines_on_right;
        }
    }
    if (lines_on_right % 2 != 0) {
        return true;
    }
    return false;
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

    Point2D p;
    Point2DArray tiles = {0};

    while (read_point2d(&p, f)) {
        DARRAY_PUSH(tiles, p);
    }

    Point2D pm1 = {0}, pm2 = {0}; // for reporting only

    int64_t max1_square = 0, max2_square = 0;
    for (size_t a_idx = 0; a_idx < tiles.length - 1; ++a_idx) {
        for (size_t b_idx = a_idx + 1; b_idx < tiles.length; ++b_idx) {
            int dx = tiles.data[a_idx].x - tiles.data[b_idx].x;
            int dy = tiles.data[a_idx].y - tiles.data[b_idx].y;
            int64_t width = ABS(dx) + 1;
            int64_t height = ABS(dy) + 1;
       
            int64_t square = width * height;

            if (square > max2_square) {
                Point2D top_left = {
                    .x = MIN(tiles.data[a_idx].x, tiles.data[b_idx].x),
                    .y = MIN(tiles.data[a_idx].y, tiles.data[b_idx].y)
                };
                // for each point on perimeter of rectabgle, check if point inside of the shape
                bool rect_inside = true;
                for (size_t x = top_left.x; x < (top_left.x + width) && rect_inside; ++x) {
                    if (!point_inside_poly(tiles, (Point2D){ .x = x, .y = top_left.y }) ||
                        !point_inside_poly(tiles, (Point2D){ .x = x, .y = top_left.y + height - 1 })) 
                    {
                        rect_inside = false;
                    }

                }
                for (size_t y = top_left.y; y < (top_left.y + height) && rect_inside; ++y) {
                    if (!point_inside_poly(tiles, (Point2D){ .x = top_left.x, .y = y }) || 
                        !point_inside_poly(tiles, (Point2D){ .x = top_left.x + width - 1, .y = y })) 
                    {
                        rect_inside = false;
                    }
                }
                if (rect_inside) {
                    // printf("new max square %"PRIu64" - (%zu,%zu) - (%zu,%zu)\n", 
                    //     square, 
                    //     tiles.data[a_idx].x, tiles.data[a_idx].y,
                    //     tiles.data[b_idx].x, tiles.data[b_idx].y
                    // );
                    pm1.x = tiles.data[a_idx].x;
                    pm1.y = tiles.data[a_idx].y;
                    pm2.x = tiles.data[b_idx].x;
                    pm2.y = tiles.data[b_idx].y;

                    max2_square = square;
                }
            }
            max1_square = MAX(square, max1_square);
        }
    }

    printf("answer 1: %"PRIi64"\n", max1_square);
    printf("answer 2: %"PRIi64" (%zu,%zu) - (%zu,%zu)\n", max2_square, pm1.x, pm1.y, pm2.x, pm2.y);

    free(tiles.data);
    fclose(f);
    return 0;
}