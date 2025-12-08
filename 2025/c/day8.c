#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "common.h"

typedef struct {
    double x;
    double y;
    double z;
    size_t circuit_id;
} JunctionBox;

typedef struct {
    size_t a_idx;
    size_t b_idx;
} IdxPair;

typedef struct {
    size_t circuit_id;
    size_t count;
} CircuiInfo;

DARRAY_DEFINE_TYPE(JunctionBoxArray, JunctionBox);
DARRAY_DEFINE_TYPE(IdxPairs, IdxPair);
DARRAY_DEFINE_TYPE(CircuiInfoArray, CircuiInfo);

static bool contains_index_pair(IdxPairs *pairs, size_t a_idx, size_t b_idx) 
{
    for (size_t i = 0; i < pairs->length; ++i) {
        bool found =
            (pairs->data[i].a_idx == a_idx && pairs->data[i].b_idx == b_idx) ||
            (pairs->data[i].a_idx == b_idx && pairs->data[i].b_idx == a_idx);
        if (found) {
            return true;
        }
    }
    return false;
}

static bool read_point(JunctionBox *p, FILE *f) {
    int coord[3] = {0};
    size_t coord_idx = 0;
    for (int c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
        if (c == ',') {
            ++coord_idx;
            continue;
        }
        coord[coord_idx] = coord[coord_idx] * 10 + c - '0';
    }
    if (coord_idx != 2) {
        return false;
    }
    p->x = coord[0];
    p->y = coord[1];
    p->z = coord[2];
    return true;
}

static double point_distance(JunctionBox p1, JunctionBox p2)
{
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2));
}

static bool contains_index_pair(IdxPairs *pairs, size_t a_idx, size_t b_idx) 
{
    for (size_t i = 0; i < pairs->length; ++i) {
        bool found =
            (pairs->data[i].a_idx == a_idx && pairs->data[i].b_idx == b_idx) ||
            (pairs->data[i].a_idx == b_idx && pairs->data[i].b_idx == a_idx);
        if (found) {
            return true;
        }
    }
    return false;
}

static void connect_boxes(JunctionBoxArray *boxes, size_t a_idx, size_t b_idx)
{
    // set all b circuit ids to a circuit id
    size_t a_circuit_id = boxes->data[a_idx].circuit_id;
    size_t b_circuit_id = boxes->data[b_idx].circuit_id;

    for (size_t box_idx = 0; box_idx < boxes->length; ++box_idx) {
        if (boxes->data[box_idx].circuit_id == b_circuit_id) {
            boxes->data[box_idx].circuit_id = a_circuit_id;
        }
    }
}

static int compare_circuit_info(const void *a, const void *b)
{
    CircuiInfo *c_a = (CircuiInfo *)a;
    CircuiInfo *c_b = (CircuiInfo *)b;
    if (c_a->count < c_b->count) return 1;
    if (c_a->count > c_b->count) return -1;
    return 0;
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

    JunctionBox p;
    JunctionBoxArray boxes = {0};

    size_t circuit_id = 0;
    while (read_point(&p, f)) {
        p.circuit_id = circuit_id++;
        DARRAY_PUSH(boxes, p);
    }

    IdxPairs pairs = {0};
    size_t answer1 = 0, answer2 = 0;

    size_t max_connections = 1000; // 10 for test 1000 for full

    while(true) {
        double min_distance = INFINITY;
        size_t point_a, point_b;

        for (size_t point_a_idx = 0; point_a_idx < boxes.length - 1; ++point_a_idx) {
            double iter_min_dist = INFINITY;
            size_t min_distance_idx = point_a_idx+1;
            for (size_t point_b_idx = point_a_idx+1; point_b_idx < boxes.length; ++point_b_idx) {
                double distance = point_distance(boxes.data[point_a_idx], boxes.data[point_b_idx]);
                if (distance < iter_min_dist) {
                    if (!contains_index_pair(&pairs, point_a_idx, point_b_idx)) {
                        iter_min_dist = distance;
                        min_distance_idx = point_b_idx;
                    }
                }
            }
            if (iter_min_dist < min_distance) {
                point_a = point_a_idx;
                point_b = min_distance_idx;
                min_distance = iter_min_dist;
            }
        }
        if (min_distance < INFINITY) {
            connect_boxes(&boxes, point_a, point_b);
            // check if all in one circuit (this smells)
            bool single_circuit = true;
            for (size_t i = 1; i < boxes.length; ++i) {
                if (boxes.data[i].circuit_id != boxes.data[0].circuit_id) {
                    single_circuit = false;
                    break;
                }
            }
            if (single_circuit) {
                answer2 = boxes.data[point_a].x * boxes.data[point_b].x;
                break;
            }
            IdxPair p = {.a_idx = point_a, .b_idx = point_b};
            DARRAY_PUSH(pairs, p);
        }
        // answer 1
        --max_connections;
        if (max_connections == 0) {
            CircuiInfoArray circuits = {0};
            for (size_t p = 0; p < boxes.length; ++p) {
                size_t circuit_id = boxes.data[p].circuit_id; // doesn matter a or b
                bool found = false;
                for (size_t i = 0; i < circuits.length; ++i) {
                    if (circuits.data[i].circuit_id == circuit_id) {
                        ++circuits.data[i].count;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    CircuiInfo c = {.circuit_id = circuit_id, .count = 1};
                    DARRAY_PUSH(circuits, c);
                }
            }
            qsort(circuits.data, circuits.length, sizeof(CircuiInfo), compare_circuit_info);
            answer1 = circuits.data[0].count * circuits.data[1].count * circuits.data[2].count;
            free(circuits.data);
        }
    }

    printf("answer 1: %zu\n", answer1);
    printf("answer 1: %zu\n", answer2);

    free(boxes.data);
    free(pairs.data);
    
    fclose(f);
    return 0;
}
