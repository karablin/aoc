#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

DARRAY_DEFINE_TYPE(U16Array, uint16_t)
DARRAY_DEFINE_TYPE(String, char)

// lights and buttons encoded in bits
// lights is set of bits, one bit per light
// each button is also set of bits, each bit position connects to light
// e.g. bit 0 - lights bit 0, bit 1 - lights bit 1, etc...
typedef struct {
    uint16_t lights;
    U16Array bit_buttons;
    U16Array joltages;
} SchemaConfig;

DARRAY_DEFINE_TYPE(SchemaConfigArray, SchemaConfig)

void print_binary_u16(uint16_t num);
bool read_schema_line(SchemaConfig* schema, FILE* f);

size_t task_1(SchemaConfig schema) {
    ////////////////////////////
    // TASK 1
    // ok, bits everywhere :)
    // generate all possible combinations as N-bit numbers
    // where N is the count of buttons in this schema
    // and search for minimum
    size_t min_buttons = schema.bit_buttons.length;
    // uint16_t found_combination = 0;

    size_t max_number = (1 << schema.bit_buttons.length) - 1;
    for (uint16_t btn_comb = 1; btn_comb < max_number; ++btn_comb) {
        // check if that combo is producing corrent lights setup
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

size_t task_2(SchemaConfig schema) {
    ////////////////////////////
    // TASK 2
    // for each button calculate constraints (max allowed button presses,
    // based on Joltage requirements and wiring)

    // TODO: convert bit buttons to array
    U16Array button_constraints = {0};
    for (size_t i = 0; i < schema.bit_buttons.length; ++i) {  // TODO: resize macro
        DARRAY_PUSH(button_constraints, UINT16_MAX);
        uint16_t button = schema.bit_buttons.data[i];
        size_t joltage_idx = 0;
        while (button) {
            if (button & 1) {
                button_constraints.data[i] = MIN(
                    schema.joltages.data[joltage_idx],
                    button_constraints.data[i]);
            }
            ++joltage_idx;
            button >>= 1;
        }
    }

    printf("Button Constraints:");
    for (size_t i = 0; i < schema.joltages.length; ++i) {
        printf("%" PRIu16 " ", schema.joltages.data[i]);
    }
    printf("\n");
    for (size_t i = 0; i < button_constraints.length; ++i) {
        printf("Button %zu limit %" PRIu16 " pushes\n", i, button_constraints.data[i]);
    }
    printf("\n");

    // lets try to bruteforce, with help of constraints above
    U16Array button_presses = {0};  // contains ongoing combination of button presses
    size_t min_button_pressed = SIZE_MAX;
    U16Array best_button_presses = {0};  // best result so far
    for (size_t i = 0; i < schema.bit_buttons.length; ++i) {
        DARRAY_PUSH(button_presses, 0);
        DARRAY_PUSH(best_button_presses, 0);
    }
    U16Array iter_result = {0};  // result for iteration
    for (size_t i = 0; i < schema.joltages.length; ++i) {
        DARRAY_PUSH(iter_result, 0);
    }

    uint64_t iter_count = 0;
    while (true) {
        ++iter_count;
        printf("I: %" PRIu64 "\r", iter_count);
        // get next combination (kinda varying-base number)
        bool overflow = false;
        for (size_t i = 0; i < button_presses.length; ++i) {
            ++button_presses.data[i];
            if (button_presses.data[i] > button_constraints.data[i]) {
                // overflow in this position, carry to next one
                button_presses.data[i] = 0;
                overflow = true;
            } else {
                overflow = false;
                break;
            }
        }
        if (overflow) {
            // no more combinations left
            break;
        }
        // produce result based on presses
        size_t btn_press_cnt = 0;
        for (size_t btn_idx = 0; btn_idx < button_presses.length; ++btn_idx) {
            size_t press_count = button_presses.data[btn_idx];
            btn_press_cnt += press_count;
            if (press_count) {
                uint16_t button = schema.bit_buttons.data[btn_idx];
                size_t result_idx = 0;
                while (button) {
                    if (button & 1) {
                        iter_result.data[result_idx] += press_count;
                    }
                    button >>= 1;
                    ++result_idx;
                }
            }
        }
        if (0 == memcmp(iter_result.data, schema.joltages.data, DARRAY_BYTES(schema.joltages))) {
            printf("Found!!! result = ");
            for (size_t i = 0; i < iter_result.length; ++i) {
                printf(" %" PRIu16, iter_result.data[i]);
            }
            printf(" Combo: ");
            for (size_t i = 0; i < button_presses.length; ++i) {
                printf(" %" PRIu16, button_presses.data[i]);
            }
            printf("\n");
            // save best result
            if (btn_press_cnt < min_button_pressed) {
                min_button_pressed = btn_press_cnt;
                memcpy(best_button_presses.data, button_presses.data, DARRAY_BYTES(button_presses));
            }
        }
        // clean result for next iteration
        memset(iter_result.data, 0, DARRAY_BYTES(iter_result));
    }

    printf("MIN presses: %zu Combo:", min_button_pressed);
    for (size_t i = 0; i < best_button_presses.length; ++i) {
        printf("%" PRIu16 ",", best_button_presses.data[i]);
    }
    printf("\n");

    free(iter_result.data);
    free(best_button_presses.data);
    free(button_presses.data);
    free(button_constraints.data);

    return min_button_pressed;
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

        answer1 += task_1(schema);
        answer2 += task_2(schema);
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
    }
    free(schemas.data);

    fclose(f);
    return 0;
}

// [.##.] (3) (1,3) (2) (2,3) (0,2) (0,1) {3,5,4,7}
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
    // parse [.##.]
    if (buf.data[buf_pos] != '[') {
        printf("Expeced '[' but got '%c' instead\n", buf.data[buf_pos]);
        goto exit_deinit;
    }
    size_t bit_no = 0;
    ++buf_pos;
    for (char c = buf.data[buf_pos]; c != ']' && buf_pos < buf.length; ++buf_pos, c = buf.data[buf_pos]) {
        if (c == '#') {
            schema->lights |= 1 << bit_no;
        }
        ++bit_no;
    }
    ++buf_pos;
    // printf("required lights: ");
    // print_binary_u16(schema->lights);
    // printf("\n");
    // parse list of buttons
    while (buf.data[buf_pos] == '(' && buf_pos < buf.length) {
        DARRAY_PUSH(schema->bit_buttons, 0);
        // each button is single digits separated by commas
        for (; buf.data[buf_pos] != ')'; ++buf_pos) {
            if (!isdigit(buf.data[buf_pos])) {
                continue;
            }
            char light_bit_no = buf.data[buf_pos] - '0';
            schema->bit_buttons.data[schema->bit_buttons.length - 1] |= 1 << light_bit_no;
        }
        ++buf_pos;
    }
    //
    // printf("Buttons: ");
    // for (size_t i = 0; i < schema->buttons.length; ++i) {
    //     print_binary_u16(schema->buttons.data[i]);
    //     printf(", ");
    // }
    // printf("\n");
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
    // printf("Joltages: ");
    // for (size_t i = 0; i < schema->joltages.length; ++i) {
    //     printf("%"PRIu8", ", schema->joltages.data[i]);
    // }
    // printf("\n");
    retval = true;

exit_deinit:
    free(buf.data);
    return retval;
    // what a mess...
}

void print_binary_u16(uint16_t num) {
    // Lookup table for each 4-bit nibble
    static const char* nibbles[] = {
        "0000", "0001", "0010", "0011",
        "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011",
        "1100", "1101", "1110", "1111"};
    // Print each nibble (4 bits at a time)
    printf("%s %s %s %s",
           nibbles[(num >> 12) & 0xF],
           nibbles[(num >> 8) & 0xF],
           nibbles[(num >> 4) & 0xF],
           nibbles[num & 0xF]);
}
