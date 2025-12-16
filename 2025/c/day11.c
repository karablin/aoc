#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "common.h"

// its just ascii 3-bytes
#define SVR_NAME 0x737672
#define FFT_NAME 0x666674
#define DAC_NAME 0x646163
#define YOU_NAME 0x796F75
#define OUT_NAME 0x6F7574

DARRAY_DEFINE_TYPE(String, char);
DARRAY_DEFINE_TYPE(U32Array, uint32_t);
DARRAY_DEFINE_TYPE(IdxArray, size_t);
DARRAY_DEFINE_TYPE(IdxArrayArray, IdxArray);

typedef struct {
    size_t end_node;      // index to names/childs arrays
    IdxArrayArray childs;
    U32Array names;
    IdxArray path;
    size_t path_count;
    IdxArray unreachable; 
} WireGraph;

static size_t count_paths(WireGraph *graph, uint32_t start_name, uint32_t end_name);
static void traverse(WireGraph *graph, size_t node);
static bool is_sequence_possible(WireGraph *graph, size_t seq[], size_t len);
static void reset_unreachable_flag(WireGraph *graph);
static void flag_unreachable_nodes(WireGraph *graph, size_t node);
static size_t get_node_idx_by_name(WireGraph *graph, uint32_t node_name);
static size_t store_node(WireGraph *graph, uint32_t node_name);
static void print_node(WireGraph* graph, size_t idx);
static bool parse_input(WireGraph *graph, FILE *f);
static bool read_line(String *buf, FILE *f);
static uint32_t parse3c (char c[]);

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

    WireGraph graph = {0};
    if(!parse_input(&graph, f)) {
        goto end;
    }
    DARRAY_RESIZE(graph.unreachable, graph.names.length); // same size as other node arrays

    // PART 1
    size_t answer1 = count_paths(&graph, YOU_NAME, OUT_NAME);
    printf("answer1: %zu\n\n", answer1);
    
    // PART 2
    // final answer is multiplication of paths counts 
    // e.g.: count(svr -> fft) * count(fft -> dac) * count(dac -> out)
    
    size_t sequences[][4] = {
        {SVR_NAME, DAC_NAME, FFT_NAME, OUT_NAME},
        {SVR_NAME, FFT_NAME, DAC_NAME, OUT_NAME}
    };

    uint64_t answer2 = 1;
    for (size_t seq_idx = 0; seq_idx < ARRAY_LENGTH(sequences); ++seq_idx) {
        // only one of sequences valid, graph is directional and w/o cycles (at least I didnt noticed cycles)
        if (is_sequence_possible(&graph, sequences[seq_idx], ARRAY_LENGTH(sequences[seq_idx]))) {
            printf("seq %zu is valid\n", seq_idx);
            for (size_t i = 0; i < ARRAY_LENGTH(sequences[seq_idx]) - 1; ++i) {
                answer2 *= count_paths(&graph, sequences[seq_idx][i], sequences[seq_idx][i+1]);
            }
        }
    }
    printf("answer 2: %"PRIu64"\n", answer2);

end:
    free(graph.names.data);
    for (size_t i = 0; i < graph.childs.length; ++i)
    {
        free(graph.childs.data[i].data);
    }
    free(graph.childs.data);
    free(graph.path.data);
    free(graph.unreachable.data);
    fclose(f);
    return 0;
}

// count all possible paths from node 'start_name' to node 'end_name'
static size_t count_paths(WireGraph *graph, uint32_t start_name, uint32_t end_name) 
{
    size_t start = get_node_idx_by_name(graph, start_name);
    size_t end = get_node_idx_by_name(graph, end_name);
    print_node(graph, start); printf(" -> "); print_node(graph, end); printf("\n");

    graph->path_count = 0;
    reset_unreachable_flag(graph);
    flag_unreachable_nodes(graph, end);
    graph->end_node = end;
    traverse(graph, start);
    printf("count %zu\n", graph->path_count);
    return graph->path_count;
}

// main path counting function.
// recursive DFS with pruning
static void traverse(WireGraph *graph, size_t node) 
{
    // is this correct path?
    if (node == graph->end_node) {
        ++graph->path_count;
        return;
    }

    // no more childs
    if (graph->childs.data[node].length == 0) {
        return;
    }

    DARRAY_PUSH(graph->path, node);
    IdxArray childs = graph->childs.data[node];
    for (size_t i = 0; i < childs.length; ++i) {
        if (graph->unreachable.data[childs.data[i]]) {
            // skip unreachable paths
            continue;
        }
        traverse(graph, childs.data[i]);
    }
    DARRAY_REMOVE(graph->path, graph->path.length-1);
}

// check if path can contain intermediate nodes, e.g.:
// svr -> fft -> dac -> out or
// svr -> dac -> fft -> out
// by checking each pair unreachable nodes
static bool is_sequence_possible(WireGraph *graph, size_t seq[], size_t len)
{
    for (size_t i = 0; i < len - 1; ++i) {
        size_t start = get_node_idx_by_name(graph, seq[i]);
        size_t end = get_node_idx_by_name(graph, seq[i+1]);
        // mark all nodes unreachable after end node
        reset_unreachable_flag(graph);
        flag_unreachable_nodes(graph, end); 
        if (graph->unreachable.data[start]) {
            // if start node is unreachable then whole sequence is impossible
            return false;
        }
    }
    return true;
}

/////////////////////////////////////////////////////
// node functions
//

// mark all nodes after specified node as unreachable
static void flag_unreachable_nodes(WireGraph *graph, size_t node)
{
    IdxArray childs = graph->childs.data[node];

    for (size_t i = 0; i < childs.length; ++i) {
        if (graph->unreachable.data[childs.data[i]]) {
            continue;
        }
        graph->unreachable.data[childs.data[i]] = 1;
        flag_unreachable_nodes(graph, childs.data[i]);
    }
}

// reset 'unreachable' flag for all nodes
static void reset_unreachable_flag(WireGraph *graph)
{
    for (size_t i = 0; i < graph->unreachable.length; ++i) {
        graph->unreachable.data[i] = 0;
    }
}

static size_t get_node_idx_by_name(WireGraph *graph, uint32_t node_name) 
{
    for (size_t i = 0; i < graph->names.length; ++i) {
        if (graph->names.data[i] == node_name) {
            return i;
        }
    }
    return SIZE_MAX;
}

static size_t store_node(WireGraph *graph, uint32_t node_name) 
{
    size_t idx = get_node_idx_by_name(graph, node_name);
    if (idx < SIZE_MAX) {
        return idx;
    }
    DARRAY_PUSH(graph->names, node_name);
    IdxArray childs = {0};
    DARRAY_PUSH(graph->childs, childs);
    return graph->names.length - 1;
}

/////////////////////////////////////////////////////
// parsing input
// 
// state machine just for fun
typedef enum {
    PS_NODE, 
    PS_COLON,
    PS_SPACE,
    PS_CHILD
} ParseState;

static bool parse_input(WireGraph *graph, FILE *f) 
{
    String buf = {0};
    bool parsed = true;
    // ccc: ddd eee fff
    while (read_line(&buf, f)) {
        ParseState state = PS_NODE;
        size_t current_node = 0;
        for (size_t start = 0, end = 0; end < buf.length; ++end) {
            switch (state) {
                case PS_NODE:
                    if (!isalpha(buf.data[end])) {
                        printf("Expected latin letter in node name but '%c' found\n", buf.data[end]);
                        parsed = false;
                        goto end;
                    }
                    if (end - start == 2) {
                        current_node = store_node(graph, parse3c(&buf.data[start]));
                        state = PS_COLON;
                        start = end + 1;
                    }
                    break;
                case PS_COLON:
                    if (buf.data[end] != ':') {
                        printf("Expected ':' but '%c' found\n", buf.data[end]);
                        parsed = false;
                        goto end;
                    }
                    state = PS_SPACE;
                    start = end + 1;
                    break;
                case PS_SPACE:
                    if (buf.data[end] != ' ') {
                        printf("Expected space but '%c' found\n", buf.data[end]);
                        parsed = false;
                        goto end;                        
                    }
                    state = PS_CHILD;
                    start = end + 1;
                    break;
                case PS_CHILD:
                    if (!isalpha(buf.data[end])) {
                        printf("Expected latin letter in child name but '%c' found\n", buf.data[end]);
                        parsed = false;
                        goto end;
                    }
                    if (end - start == 2) {
                        size_t idx = store_node(graph, parse3c(&buf.data[start]));
                        DARRAY_PUSH(graph->childs.data[current_node], idx);
                        state = PS_SPACE;
                        start = end + 1;
                    }                    
                    break;
            }
        }

        buf.length = 0;
    }
end:
    free(buf.data);
    return parsed;
}

static bool read_line(String* buf, FILE* f) 
{
    for (char c = getc(f); c != EOF && c != '\n'; c = getc(f)) {
        if (c == '\r') {
            continue;
        }
        DARRAY_PUSH(*buf, c);
    }
    return buf->length > 0;
}

static uint32_t parse3c (char c[])
{
    return c[2] | c[1] << 8 | c[0] << 16;
}

/////////////////////////////////////////////////////
// pretty printing
//

static void print_node(WireGraph* graph, size_t idx) 
{
    if (idx >= graph->names.length) {
        printf("print_node(): out of bounds at %zu\n", idx);
        return;
    }
    uint32_t n = graph->names.data[idx];
    printf("%c%c%c", (n >> 16) & 0xFF, (n >> 8) & 0xFF, (n) & 0xFF);
}