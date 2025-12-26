#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#define MATRIX_ELEM_TYPE int16_t
#define MATRIX_H_IMPL
#include "matrix.h"

DARRAY_DEFINE_TYPE(U16Array, uint16_t)
DARRAY_DEFINE_TYPE(U8Array, uint8_t)
DARRAY_DEFINE_TYPE(U8ArrayArray, U8Array)
DARRAY_DEFINE_TYPE(String, char)
DARRAY_DEFINE_TYPE(I16Array, int16_t)

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

static bool read_schema_line(SchemaConfig* schema, FILE* f);
static void print_schema(const SchemaConfig *schema);
static size_t get_btn_press_count(const Matrix *m, const U16Array *button_counts, const I16Array *current_free_vars);
static Matrix* gauss_jordan(SchemaConfig *schema);
static void print_matrix(const Matrix *m);
static bool is_basic_variable(const Matrix *m, size_t col);
static void mat_cleanup(Matrix *m);
static int16_t gcd(int16_t a, int16_t b);
static int16_t mat_row_gcd(const Matrix *m, size_t row);

// TASK 1
// ok, bits everywhere :)
// generate all possible combinations as N-bit numbers
// where N is the count of buttons in this schema
// and search for minimum
static size_t task_1(SchemaConfig schema)
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

// TASK 2
// solve linear equations:
// use Gauss-Jordan elimination,
// and then iterate over free variables, if any, to (probably) get solutions
// if there is no free variables - only one solution exists
static size_t task_2(SchemaConfig *schema) {
    size_t btn_press_min = SIZE_MAX;
    Matrix *m = gauss_jordan(schema);
    U16Array global_constraints = {0};

    // after Gauss-Jordan elimination, linear equation matrix looks like this:
    // ┌──┬──┬──┬──┬────────────── basic variables
    // v  v  v  v  v  v──v──────── free variables
    //                        v─── constant term
    // 1  0  0  0  0 -1 -1 | -15
    // 0  1  0  0  0  1  1 | 17
    // 0  0  1  0  0  0 -1 | 13
    // 0  0  0  1  0  0  2 | 14
    // 0  0  0  0 -1  0  1 | -171
    //
    // basic variables can be found by iterating over free variable values

    // how many free variables in resulting matrix
    size_t free_var_cnt = m->cols - m->rows - 1;
    printf("free var count: %zu\n", free_var_cnt);

    // setup global constraints, only for free variables
    for (size_t i = m->rows; i < schema->byte_buttons.length; ++i) {
        uint16_t constraint = UINT16_MAX;
        U8Array button = schema->byte_buttons.data[i];
        for (size_t joltage_idx = 0; joltage_idx < button.length; ++joltage_idx) {
            if (button.data[joltage_idx]) {
                constraint = MIN(
                    schema->joltages.data[joltage_idx],
                    constraint);
            }
        }
        DARRAY_PUSH(global_constraints, constraint);
    }
    printf("Max possible button pushes: ");
    for (size_t i = 0; i < global_constraints.length; ++i) {
        printf(" %"PRIu16, global_constraints.data[i]);
    }
    printf("\n");

    // only for printing combo
    DARRAY_NEW(U16Array, button_counts, schema->byte_buttons.length);

    if (free_var_cnt == 0) {
        // only one solution
        I16Array no_free_vars = {0};
        btn_press_min = get_btn_press_count(m, &button_counts, &no_free_vars);
    } else {
        DARRAY_NEW(I16Array, current_free_vars, free_var_cnt);
        // iterate over all possible combinations of free variables
        while (true) {
            size_t btn_press_cnt = get_btn_press_count(m, &button_counts, &current_free_vars);
            if (btn_press_cnt > 0) {
                btn_press_min = MIN(btn_press_cnt, btn_press_min);
            }
            // increment free vars carry overflows until there is no room, meaning all combinations was tried
            bool carry = false;
            for (size_t vi = 0; vi < current_free_vars.length; ++vi) {
                const int16_t v_next = current_free_vars.data[vi] + 1;
                if (v_next > global_constraints.data[vi]) {
                    current_free_vars.data[vi] = 0;
                    carry = true;
                } else {
                    current_free_vars.data[vi] = v_next;
                    carry = false;
                    break;
                }
            }
            if (carry) {
                break;
            }
        }
        free(current_free_vars.data);
    }
    // free memory
    mat_free(m);
    free(global_constraints.data);
    free(button_counts.data);
    return btn_press_min;
}

// returns 0 if there is no non-negative integer solution
static size_t get_btn_press_count(const Matrix *m, const U16Array *button_counts, const I16Array *current_free_vars)
{
    size_t btn_press_cnt = 0;
    size_t free_var_start = m->rows;
    size_t last_column = m->cols - 1;

    bool have_solution = true;
    for (size_t row_idx = 0; row_idx < m->rows; ++row_idx) {
        int16_t k_x = mat_get(m, row_idx, row_idx);
        int16_t answer = mat_get(m, row_idx, last_column);
        for (size_t fv_idx = 0; fv_idx < current_free_vars->length; ++fv_idx) {
            answer -= current_free_vars->data[fv_idx] * mat_get(m, row_idx, free_var_start + fv_idx);
        }
        if (answer % k_x != 0) {
            have_solution = false;
            break;
        }
        answer /= k_x;
        if (answer < 0) {
            have_solution = false;
            break;
        }
        button_counts->data[row_idx] = answer;
        btn_press_cnt += answer;
    }
    // copy free vars to button counts
    for (size_t i = 0; i < current_free_vars->length; ++i) {
        button_counts->data[i + free_var_start] = current_free_vars->data[i];
        btn_press_cnt += current_free_vars->data[i];
    }

    if (!have_solution) {
        return 0;
    }

    printf("FOUND:");
    for (size_t i = 0; i < button_counts->length; ++i) {
        printf(" %" PRIu16, button_counts->data[i]);
    }
    printf(" (%zu total)\n", btn_press_cnt);

    return btn_press_cnt;
}

// Gauss-Jordan elimination
static Matrix* gauss_jordan(SchemaConfig *schema)
{
    const size_t columns_cnt = schema->byte_buttons.length + 1;

    // make matrix and fill it with initial values
    Matrix *m = mat_new(schema->joltages.length, columns_cnt);
    for (size_t r = 0; r < schema->joltages.length; ++r) {
        for (size_t c = 0; c < columns_cnt - 1; ++c) {
            mat_set(m, r, c, schema->byte_buttons.data[c].data[r]);
        }
        mat_set(m, r, columns_cnt - 1, (int16_t)schema->joltages.data[r]);
    }

    puts("original:");
    print_matrix(m);

    for (size_t start_row_idx = 0; start_row_idx < m->rows; ++start_row_idx) {
        size_t col = MIN(start_row_idx, m->cols - 1);
        // check for zero on main diagonal
        if (mat_get(m, start_row_idx, col) == 0) {
            // find one from below
            bool fixed = false;
            for (size_t row_idx = start_row_idx; row_idx < m->rows; ++row_idx) {
                if (mat_get(m, row_idx, col) != 0) {
                    // swap rows
                    mat_swap_rows(m, start_row_idx, row_idx);
                    fixed = true;
                    break;
                }
            }
            if (!fixed) {
                // on this column all zeroes below,  pick next non-zero column
                while (mat_get(m, start_row_idx, col) == 0 && col < m->cols) ++col;
            }
        }
        // elimination
        for (size_t row_idx = 0; row_idx < m->rows; ++row_idx) {
            if (row_idx == start_row_idx) {
                continue;
            }
            int16_t v1 = mat_get(m, row_idx, col);
            if (v1 != 0) {
                int16_t v0 = mat_get(m, start_row_idx, col);
                if (abs(v1) != abs(v0)) {
                    int16_t lcm = abs(v0 * v1) / gcd(v0, v1);
                    mat_mul_row(m, row_idx, lcm);
                }
                int16_t k = mat_get(m, row_idx, col) / v0;
                mat_sub_rows_with_k(m, row_idx, start_row_idx, k);
            }
        }
        mat_cleanup(m);
    }
    // fix column order
    for (size_t row_idx = 0; row_idx < m->rows; ++row_idx) {
        size_t current_col = row_idx;
        if (is_basic_variable(m, current_col)) {
            continue;
        }
        bool found_repl = false;
        for (size_t col_idx = current_col + 1; col_idx < m->cols; ++col_idx) {
            if (mat_get(m, row_idx, col_idx) != 0 && is_basic_variable(m, col_idx)) {
                // swap columns
                mat_swap_columns(m, current_col, col_idx);
                // and also swap buttons
                U8Array tmp = schema->byte_buttons.data[current_col];
                schema->byte_buttons.data[current_col] = schema->byte_buttons.data[col_idx];
                schema->byte_buttons.data[col_idx] = tmp;

                found_repl = true;
                break;
            }
        }
        if (!found_repl) {
            printf("CANNOT FIX column order\n");
            exit(0);
        }
    }
    printf("final:\n");
    print_matrix(m);

    return m;
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
        printf("=======================================================================\n");
        printf("#%zu\n", schema_idx);
        printf("=======================================================================\n");

        print_schema(&schema);

        answer1 += task_1(schema);
        answer2 += task_2(&schema);
    }

    printf("answer 1: %zu\n", answer1);
    printf("answer 2: %zu\n", answer2);

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

static int16_t gcd(int16_t a, int16_t b)
{
    while (b != 0) {
        const int16_t rem = a % b;
        a = b;
        b = rem;
    }
    return ABS(a);
}

int16_t mat_row_gcd(const Matrix* m, const size_t row)
{
    if (m->cols == 0) return 0;

    int16_t result = abs(mat_get(m, row, 0));
    for (size_t i = 1; i < m->cols; ++i) {
        result = gcd(result, mat_get(m, row, i));
        if (result == 1) break;
    }
    return result;
}

static void mat_cleanup(Matrix *m)
{
    // remove zero rows
    for (int row_idx = (int)m->rows - 1; row_idx >= 0; --row_idx) {
        bool zero_row = true;
        for (size_t col_idx = 0; col_idx < m->cols; ++col_idx) {
            if (mat_get(m, row_idx, col_idx)) {
                zero_row = false;
                break;
            }
        }
        if (zero_row) {
            mat_remove_row(m, row_idx);
        }
    }
    // divide rows by gcd
    for (size_t row_idx = 0; row_idx < m->rows; ++row_idx) {
        int16_t row_gcd = mat_row_gcd(m, row_idx);
        if (row_gcd > 1) {
            mat_div_row(m, row_idx, row_gcd);
        }
    }
    // remove duplicates.
    for (size_t row1_idx = 0; row1_idx < m->rows - 1; ++row1_idx) {
        for (size_t row2_idx = m->rows - 1; row2_idx > row1_idx; --row2_idx) {
            if (memcmp(&m->data[row1_idx * m->cols], &m->data[row2_idx * m->cols], m->cols * sizeof(m->data[0])) == 0) {
                mat_remove_row(m, row2_idx);
            }
        }
    }
}

static bool is_basic_variable(const Matrix *m, size_t col)
{
    size_t non_z_cnt = 0;
    for (size_t row_idx = 0; row_idx < m->rows; ++row_idx) {
        if (mat_get(m, row_idx, col) != 0) {
            ++non_z_cnt;
        }
    }
    return non_z_cnt == 1;
}

static void print_matrix(const Matrix *m)
{
    puts("-------------------------------");
    for (size_t row_idx = 0; row_idx < m->rows; ++row_idx) {
        for (size_t col_idx = 0; col_idx < m->cols - 1; ++col_idx) {
            printf("%2"PRIi16" ", mat_get(m, row_idx, col_idx));
        }
        printf("| %"PRIi16"\n", mat_get(m, row_idx, m->cols - 1));
    }
    printf("%zu rows\n", m->rows);
}


// input parser
static bool read_schema_line(SchemaConfig* schema, FILE* f) {
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

static void print_schema(const SchemaConfig *schema)
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
