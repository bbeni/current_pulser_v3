// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "mma.h"

#define TEST_START() bool did_fail = false
#define EQF(a, b, tol) if(fabs((a)-(b)) > (tol)){printf("SUBTEST FAILED: %s:%d: %.20f (have) == %.20f (should have)\n", __FILE__, __LINE__, (a), (b)); did_fail = true;}
#define EQU8(a, b) if( (a) != (b) ){printf("SUBTEST FAILED: %s:%d: 0x%02X (have) == 0x%02X (should have)\n", __FILE__, __LINE__, (a), (b)); did_fail = true;}
#define NEQU8(a, b) if( (a) == (b) ){printf("SUBTEST FAILED: %s:%d: 0x%02X (have) == 0x%02X (should have)\n", __FILE__, __LINE__, (a), (b)); did_fail = true;}
#define EQU8i(a, b, i) if( (a) != (b) ){printf("SUBTEST(%zu) FAILED: %s:%d: 0x%02X (have) == 0x%02X (should have)\n", (i), __FILE__, __LINE__, (a), (b)); did_fail = true;}
#define NEQU8i(a, b, i) if( (a) == (b) ){printf("SUBTEST(%zu) FAILED: %s:%d: 0x%02X (have) == 0x%02X (should have)\n", (i), __FILE__, __LINE__, (a), (b)); did_fail = true;}
#define TEST_END() return !did_fail

//void mma_spline_cubic_natural_ab(double *x, double *y, size_t n_in, double *a_out, double *b_out)
bool test_mma_spline_cubic_natural_ab_1() {
    double x[] = {-1.0, 0.0, 3.0};
    double y[] = {0.5, 0.0, 3.0};
    double a[2] = {0xCD, 0xCD};
    double b[2] = {0xCD, 0xCD};
    mma_spline_cubic_natural_ab(x, y, 3, a, b);

    TEST_START();
    EQF(a[0], -0.1875, 0.000000000000001);
    EQF(b[0], -0.375, 0.000000000000001);
    EQF(a[1], -3.375, 0.000000000000001);
    EQF(b[1], -1.6875, 0.000000000000001);
    TEST_END();
}

//void mma_spline_cubic_natural_ab(double *x, double *y, size_t n_in, double *a_out, double *b_out)
bool test_mma_spline_cubic_natural_ab_2() {
    // overallocate memory with padding to see if boundary is crossed
    size_t n = 120312;
    size_t padding = 64; // bytes
    double *x_padded = malloc(n*sizeof(double) + padding);
    double *y_padded = malloc(n*sizeof(double) + padding);
    double *a_padded = malloc((n-1)*sizeof(double) + padding);
    double *b_padded = malloc((n-1)*sizeof(double) + padding);

    // set memory to hex CDCDCDCD...
    memset(x_padded, 0xCD, n*sizeof(*x_padded) + padding);
    memset(y_padded, 0xCD, n*sizeof(*y_padded) + padding);
    memset(a_padded, 0xCD, (n-1)*sizeof(*a_padded) + padding);
    memset(b_padded, 0xCD, (n-1)*sizeof(*b_padded) + padding);

    double *x = (double *)((unsigned char*)x_padded + padding/2);
    double *y = (double *)((unsigned char*)y_padded + padding/2);
    double *a = (double *)((unsigned char*)a_padded + padding/2);
    double *b = (double *)((unsigned char*)b_padded + padding/2);

    mma_spline_cubic_natural_ab(x, y, n, a, b);

    TEST_START();
    for (size_t i = 0; i < padding/2; i++) {
        EQU8i(((unsigned char*)x_padded)[i], 0xCD, i);
        EQU8i(((unsigned char*)y_padded)[i], 0xCD, i);
        EQU8i(((unsigned char*)a_padded)[i], 0xCD, i);
        EQU8i(((unsigned char*)b_padded)[i], 0xCD, i);
    }

    // first elements present
    NEQU8(((unsigned char*)&a[0])[0], 0xCD);
    NEQU8(((unsigned char*)&b[0])[0], 0xCD);

    // last elements present
    NEQU8(((unsigned char*)&a[n-2])[0], 0xCD);
    NEQU8(((unsigned char*)&b[n-2])[0], 0xCD);

    for (size_t i = 0; i < padding/2; i++) {
        EQU8i(((unsigned char*)&x[n])[i], 0xCD, i);
        EQU8i(((unsigned char*)&y[n])[i], 0xCD, i);
        EQU8i(((unsigned char*)&a[n-1])[i], 0xCD, i);
        EQU8i(((unsigned char*)&b[n-1])[i], 0xCD, i);
    }

    TEST_END();
}

int main() {

    bool (*tests[])() = {
        test_mma_spline_cubic_natural_ab_1,
        test_mma_spline_cubic_natural_ab_2,
    };

    size_t n_tests = sizeof tests / sizeof tests[0];
    size_t passed = 0;

    for (size_t i = 0; i < n_tests; i++) {
        if (tests[i]()) passed++;
    }

    printf("%zu/%zu tests passed.\n", passed, n_tests);
    return 0;
}