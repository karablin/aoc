#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "common.h"

DARRAY_DEFINE_TYPE(U16Array, uint16_t)
DARRAY_DEFINE_TYPE(U8Array, uint8_t)
DARRAY_DEFINE_TYPE(U8ArrayArray, U8Array)
DARRAY_DEFINE_TYPE(String, char)

// lights and buttons encoded in bits
// lights is set of bits, one bit per light
// each bit_button is set of bits, each bit position connects to light
// e.g. bit 0 - lights bit 0, bit 1 - lights bit 1, etc...
typedef struct {
    uint16_t lights;
    U16Array bit_buttons;      // for task 1
    U8ArrayArray byte_buttons; // for task 2
    U16Array joltages;
} SchemaConfig;

DARRAY_DEFINE_TYPE(SchemaConfigArray, SchemaConfig)

bool read_schema_line(SchemaConfig* schema, FILE* f);
void print_schema(const SchemaConfig *schema);

// TASK 1
// ok, bits everywhere :)
// generate all possible combinations as N-bit numbers
// where N is the count of buttons in this schema
// and search for minimum
size_t task_1(SchemaConfig schema) 
{
    size_t min_buttons = schema.bit_buttons.length;
    // uint16_t found_combination = 0;

    size_t max_number = (1 << schema.bit_buttons.length) - 1;
    for (uint16_t btn_comb = 1; btn_comb < max_number; ++btn_comb) {
        // check if that combo producing correct lights setup
        // in btn_comb each bit represents button in schema.buttons
        size_t pressed_buttons_cnt = 0;
        uint16_t button_bits = btn_comb;
        uint16_t lights = 0;
        uint16_t button_idx = 0;
        while (button_bits) {
            if (button_bits & 1) {
                lights ^= schema.bit_buttons.data[button_idx];  // push XOR button
                ++pressed_buttons_cnt;
            }
            ++button_idx;
            button_bits >>= 1;
        }
        if (lights == schema.lights) {
            if (pressed_buttons_cnt < min_buttons) {
                min_buttons = pressed_buttons_cnt;
            }
        }
    }

    return min_buttons;
}

// this is state, used by recursive search
typedef struct {
    // input
    const SchemaConfig *schema;
    U16Array global_constraints;
    // local
    U16Array button_presses;
    // output
    U16Array btn_press_solution;
    size_t btn_press_min;
    uint64_t iterations_count;
} Task2State;


// TASK 2
//
// PHEW... this was a long jorney...
// execution took 1h 34m 39s on i7 12700 (compiled with gcc -O3 -march=native)
// 
// task 2 main recursive code, depth-first search with pruning
// 
// faster alternatives to this approach is:
// - use ILP solver library. very fast, do not want 3rt party dependencies. 
//   unreal to ipmlement this method from scratch.
// - Gauss-Jordan elimination. can reduce search space by fixing some variables, 
//   so you only need to enumberate much less variables after reduction.
//   doable from scratch.
void task2_search(Task2State* s, size_t btn_idx, U16Array *current_joltages)
{
    ++s->iterations_count; // just for stats

    // all button processed, check for result
    if (btn_idx == s->schema->byte_buttons.length) {
        if (0 == memcmp(current_joltages->data, s->schema->joltages.data, DARRAY_BYTE_SIZE(*current_joltages))) {
            size_t btn_press_sum = 0;
            for (size_t i = 0; i < s->button_presses.length; ++i) {
                btn_press_sum += s->button_presses.data[i];
            }
            // print each match, to make sure it doing something :)
            time_t t = time(NULL);
            printf("%.24s", ctime(&t));
            printf(" | FOUND on %"PRIu64" iteration. Button presses: ", s->iterations_count);
            for (size_t i = 0; i < s->button_presses.length; ++i) {
                printf(" %" PRIu16, s->button_presses.data[i]);
            }
            printf(" (total: %zu), current joltages: ", btn_press_sum);
            for (size_t i = 0; i < current_joltages->length; ++i) {
                printf(" %" PRIu16, current_joltages->data[i]);
            }
            printf("\n");
            if (s->btn_press_min > btn_press_sum) {
                s->btn_press_min = btn_press_sum;
                memcpy(s->btn_press_solution.data, s->button_presses.data, DARRAY_BYTE_SIZE(s->button_presses));
            }
        }
        return;
    }

    U8Array curr_btn = s->schema->byte_buttons.data[btn_idx];
    int local_max_k = INT_MAX;
    
    // try to reduce search by pruning unreachable paths, and dynamically adjusting limit for button
    for (size_t j = 0; j < s->schema->joltages.length; ++j) {
        int jolt_diff = s->schema->joltages.data[j] - current_joltages->data[j];
        // check if it possible to reach target joltages at all on this branch
        int max_contribution = 0;
        for (size_t b = btn_idx; b < s->schema->byte_buttons.length; ++b) {
            max_contribution += s->schema->byte_buttons.data[b].data[j] * s->global_constraints.data[b];
        }
        if (max_contribution < jolt_diff)  {
            // not possible to reach
            return;
        }
        // also calculate local limit for current button (how much pushes left for it)
        if (curr_btn.data[j] > 0) {
            local_max_k = MIN(jolt_diff, local_max_k); 
        }
    }

    // save joltages state 
    uint16_t tmp_sum[current_joltages->length];
    memcpy(tmp_sum, current_joltages->data, DARRAY_BYTE_SIZE(*current_joltages));

    for (size_t k = 0; k <= local_max_k; ++k) {
        s->button_presses.data[btn_idx] = k;
        // calculate joltages for next button
        for (size_t i = 0; i < curr_btn.length; ++i) {
            current_joltages->data[i] = tmp_sum[i] + k * curr_btn.data[i];
        }
        task2_search(s, btn_idx + 1, current_joltages);
    }
}

// task 2 entry point. setup and run recursion
size_t task_2(const SchemaConfig *schema) {
    Task2State state = {0};
    state.schema = schema;
    state.btn_press_min = SIZE_MAX;
    DARRAY_RESIZE(state.button_presses, schema->byte_buttons.length);
    DARRAY_RESIZE(state.btn_press_solution, schema->byte_buttons.length);

    // setup global constraints (now much each button can be pushed)
    for (size_t i = 0; i < schema->byte_buttons.length; ++i) {  
        DARRAY_PUSH(state.global_constraints, UINT16_MAX);
        U8Array button = schema->byte_buttons.data[i];
        for (size_t joltage_idx = 0; joltage_idx < button.length; ++joltage_idx) {
            if (button.data[joltage_idx]) {
                state.global_constraints.data[i] = MIN(
                    schema->joltages.data[joltage_idx],
                    state.global_constraints.data[i]);                
            }
        }
    }
    printf("Max possible button pushes: ");
    for (size_t i = 0; i < state.global_constraints.length; ++i) {
        printf(" %"PRIu16, state.global_constraints.data[i]);
    }
    printf("\n");

    DARRAY_NEW(U16Array, current_joltages, schema->joltages.length);
    // DARRAY_NEW(U16Array, button_presses, schema->byte_buttons.length);

    task2_search(&state, 0, &current_joltages);

    free(state.global_constraints.data);
    free(state.button_presses.data);
    free(state.btn_press_solution.data);
    // free(button_presses.data);
    free(current_joltages.data);

    return state.btn_press_min;
}

// button sorting function
static SchemaConfig *cmp_shema = 0;
int cmp_sort_by_constraints(const void *p1, const void *p2) {
    U8Array *btn1 = (U8Array *)p1;
    U8Array *btn2 = (U8Array *)p2;

    size_t btn1_constraint = SIZE_MAX;
    size_t btn2_constraint = SIZE_MAX;

    for (size_t joltage_idx = 0; joltage_idx < btn1->length; ++joltage_idx) {
        if (btn1->data[joltage_idx]) {
            btn1_constraint = MIN(
                cmp_shema->joltages.data[joltage_idx],
                btn1_constraint);
        }
        if (btn2->data[joltage_idx]) {
            btn2_constraint = MIN(
                cmp_shema->joltages.data[joltage_idx],
                btn2_constraint);
        }
    }
    if (btn1_constraint < btn2_constraint) return -1;
    if (btn1_constraint > btn2_constraint) return 1;
    return 0;
}


int main(int argv, char* argc[]) {
    if (argv != 2) {
        printf("no input file specified!\n");
        return -1;
    }

    FILE* f = fopen(argc[1], "r");
    if (!f) {
        printf("can't open file %s\n", argc[1]);
        return -1;
    }

    SchemaConfigArray schemas = {0};
    while (1) {
        SchemaConfig tmp = {0};
        if (!read_schema_line(&tmp, f)) {
            break;
        }
        DARRAY_PUSH(schemas, tmp);
    }

    size_t answer1 = 0;
    size_t answer2 = 0;

    for (size_t schema_idx = 0; schema_idx < schemas.length; ++schema_idx) {
        SchemaConfig schema = schemas.data[schema_idx];

        // sort buttons list by how many pushes allowed, this speedups task2 search significantly.
        U8ArrayArray btns = schema.byte_buttons;
        cmp_shema = &schema;
        qsort(btns.data, btns.length, sizeof(btns.data[0]), cmp_sort_by_constraints);
        
        printf("=======================================================================\n");
        printf("#%zu\n", schema_idx);
        time_t t = time(NULL);
        printf("%s", ctime(&t), schema_idx);
        printf("=======================================================================\n");
        
        print_schema(&schema);

        answer1 += task_1(schema);
        answer2 += task_2(&schema);
    }

    printf("answer 1: %zu\n", answer1);
    printf("answer 2: %zu\n", answer2);

    size_t max_buttons = 0;
    for (size_t i = 0; i < schemas.length; ++i) {
        max_buttons = MAX(schemas.data[i].bit_buttons.length, max_buttons);
    }
    printf("max buttons: %zu\n", max_buttons);

    for (size_t i = 0; i < schemas.length; ++i) {
        free(schemas.data[i].bit_buttons.data);
        free(schemas.data[i].joltages.data);
        for (size_t j = 0; j < schemas.data[i].byte_buttons.length; ++j) {
            free(schemas.data[i].byte_buttons.data[j].data);
        }
        free(schemas.data[i].byte_buttons.data);
    }
    free(schemas.data);

    fclose(f);
    return 0;
}

// input parser
bool read_schema_line(SchemaConfig* schema, FILE* f) {
    String buf = {0};
    bool retval = false;

    for (int c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
        if (isspace(c)) {
            continue;
        }
        DARRAY_PUSH(buf, c);
    }
    if (buf.length == 0) {
        goto exit_deinit;
    }

    size_t buf_pos = 0;
    // parse lights
    if (buf.data[buf_pos] != '[') {
        printf("Expeced '[' but got '%c' instead\n", buf.data[buf_pos]);
        goto exit_deinit;
    }
    size_t bit_no = 0, lights_count = 0;
    ++buf_pos;
    for (char c = buf.data[buf_pos]; c != ']' && buf_pos < buf.length; ++buf_pos, c = buf.data[buf_pos]) {
        if (c == '#') {
            schema->lights |= 1 << bit_no;
        }
        ++bit_no;
        ++lights_count;
    }
    ++buf_pos;
    // parse list of buttons
    while (buf.data[buf_pos] == '(' && buf_pos < buf.length) {
        DARRAY_NEW(U8Array, byte_btn, lights_count);
        DARRAY_PUSH(schema->byte_buttons, byte_btn);
        DARRAY_PUSH(schema->bit_buttons, 0);
        // each button is single digits separated by commas
        for (; buf.data[buf_pos] != ')'; ++buf_pos) {
            if (!isdigit(buf.data[buf_pos])) {
                continue;
            }
            size_t light_bit_no = buf.data[buf_pos] - '0';
            size_t button_idx = schema->bit_buttons.length - 1;
            schema->bit_buttons.data[button_idx] |= 1 << light_bit_no;
            schema->byte_buttons.data[button_idx].data[light_bit_no] = 1;
        }
        ++buf_pos;
    }
    // parse joltages
    if (buf.data[buf_pos] != '{') {
        printf("Expeced '{' but got '%c' instead\n", buf.data[buf_pos]);
        goto exit_deinit;
    }
    ++buf_pos;
    uint16_t joltage = 0;
    for (; buf.data[buf_pos] != '}' && buf_pos < buf.length; ++buf_pos) {
        char c = buf.data[buf_pos];
        if (isdigit(c)) {
            joltage = joltage * 10 + c - '0';
        } else {
            DARRAY_PUSH(schema->joltages, joltage);
            joltage = 0;
        }
    }
    DARRAY_PUSH(schema->joltages, joltage);
    retval = true;

exit_deinit:
    free(buf.data);
    return retval;
}

void print_schema(const SchemaConfig *schema)
{
    uint16_t lights = schema->lights;
    printf("Lights:");
    for (size_t bits_cnt = 0; bits_cnt < schema->joltages.length; ++bits_cnt) {
        char c = '0' + (lights & 1);
        printf(" %c", c);
        lights >>= 1;
    }
    printf("\n");

    printf("Joltages:");
    for (size_t j = 0; j < schema->joltages.length; ++j) {
        printf(" %"PRIu16, schema->joltages.data[j]);
    }
    printf("\n");

    for (size_t i = 0; i < schema->byte_buttons.length; ++i) {
        printf("Button %zu:", i);
        U8Array btn = schema->byte_buttons.data[i];
        for (size_t j = 0; j < btn.length; ++j) {
            printf(" %"PRIu8, btn.data[j]);
        }
        printf("\n");
    }
}
