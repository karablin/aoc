#ifndef MATRIX_H
#define MATRIX_H

#ifndef MATRIX_ELEM_TYPE
#error must define MATRIX_ELEM_TYPE
#endif

#include <stddef.h>
#include <stdlib.h>

typedef struct {
    size_t rows;
    size_t cols;
    MATRIX_ELEM_TYPE data[];
} Matrix;

Matrix* mat_new(size_t rows, size_t cols);
void mat_free(Matrix* m);
void mat_swap_rows(Matrix* m, size_t row_a, size_t row_b);
void mat_swap_columns(Matrix* m, size_t col_a, size_t col_b);
void mat_remove_row(Matrix* m, size_t row);
void mat_mul_row(Matrix *m, size_t row, MATRIX_ELEM_TYPE val);
void mat_div_row(Matrix *m, size_t row, MATRIX_ELEM_TYPE val);
void mat_sub_rows_with_k(Matrix *m, size_t row_a, size_t row_b, MATRIX_ELEM_TYPE k);

static inline MATRIX_ELEM_TYPE mat_get(const Matrix* m, const size_t row, const size_t col)
{
    return m->data[row * m->cols + col];
}

static inline void mat_set(Matrix* m, const size_t row, const size_t col, const MATRIX_ELEM_TYPE val)
{
    m->data[row * m->cols + col] = val;
}

#ifdef MATRIX_H_IMPL
Matrix* mat_new(size_t rows, size_t cols)
{
    Matrix* m = malloc(sizeof(Matrix) + rows * cols * sizeof(MATRIX_ELEM_TYPE));
    if (m != NULL) {
        m->rows = rows;
        m->cols = cols;
    }
    return m;
}

void mat_free(Matrix* m)
{
    free(m);
}

void mat_swap_rows(Matrix* m, const size_t row_a, const size_t row_b)
{
    for (size_t c = 0; c < m->cols; ++c) {
        const MATRIX_ELEM_TYPE tmp = mat_get(m, row_a, c);
        mat_set(m, row_a, c, mat_get(m, row_b, c));
        mat_set(m, row_b, c, tmp);
    }
}

void mat_swap_columns(Matrix* m, const size_t col_a, const size_t col_b)
{
    for (size_t r = 0; r < m->rows; ++r) {
        const MATRIX_ELEM_TYPE tmp = mat_get(m, r, col_a);
        mat_set(m, r, col_a, mat_get(m, r, col_b));
        mat_set(m, r, col_b, tmp);
    }
}

void mat_remove_row(Matrix* m, const size_t row)
{
    if (m->rows == 0) return;

    if (row < m->rows - 1) {
        // no need to do this for last row
        size_t len = (m->rows * m->cols - (row + 1) * m->cols) * sizeof(MATRIX_ELEM_TYPE);
        memmove(&m->data[row * m->cols], &m->data[(row + 1) * m->cols], len);
    }
    --m->rows;
}

void mat_mul_row(Matrix *m, const size_t row, const MATRIX_ELEM_TYPE val)
{
    for (size_t c = 0; c < m->cols; ++c) {
        mat_set(m, row, c,
            mat_get(m, row, c) * val
        );
    }
}

void mat_div_row(Matrix *m, const size_t row, const MATRIX_ELEM_TYPE val)
{
    for (size_t c = 0; c < m->cols; ++c) {
        mat_set(m, row, c,
            mat_get(m, row, c) / val
        );
    }
}

void mat_sub_rows_with_k(Matrix *m, const size_t row_a, size_t row_b, MATRIX_ELEM_TYPE k)
{
    for (size_t c = 0; c < m->cols; ++c) {
        mat_set(m, row_a, c,
            mat_get(m, row_a, c) - mat_get(m, row_b, c) * k
        );
    }
}
#endif // MATRIX_H_IMPL

#endif
