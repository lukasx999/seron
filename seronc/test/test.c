#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define test(expr, expected) do {                                                 \
    testcount++;                                                                  \
    int value = (expr);                                                           \
    if (value == (expected)) {                                                    \
        passcount++;                                                              \
    } else {                                                                      \
        fprintf(stderr, "`" #expr " == " #expected "` failed, got %d\n", value ); \
    }                                                                             \
} while (0)

int test_add(int, int);
int test_sub(int, int);
int test_mul(int, int);
int test_div(int, int);

int test_id(int);
bool test_neg(bool);
bool test_eq(int, int);
bool test_neq(int, int);

int test_add_many(int, int, int, int, int, int, int, int, int);

int test_deref(int*);
int *test_deref_double(int**);
void test_deref_write(int*, int);

int test_cond(int, int, bool);

int test_loop(int);

int test_fptr(int(*)(void));
static int fptr(void) { return 45; }

int test_fptr_args(int(*)(int, int), int, int);
static int fptr_add(int a, int b) { return a + b; }



int main(void) {
    int passcount = 0, testcount = 0;
    printf("Testing Compiler\n");

    test(test_add(1, 2), 3);
    test(test_sub(7, 2), 5);
    test(test_mul(5, 2), 10);
    test(test_div(10, 2), 5);

    test(test_add_many(1, 2, 3, 4, 5, 6, 7, 8, 9), 45);

    test(test_id(1), 1);

    test(test_neg(1), 0);
    test(test_neg(0), 1);
    test(test_neg(15), 0);

    test(test_eq(15, 3), false);
    test(test_eq(15, 15), true);
    test(test_neq(15, 3), true);
    test(test_neq(15, 15), false);

    int a = 45;
    test(test_deref(&a), a);

    int *ap = &a;
    assert(test_deref_double(&ap) == &a);

    int b = 1;
    test_deref_write(&b, 123);
    test(b, 123);

    test(test_cond(25, 35, true), 25);
    test(test_cond(25, 35, false), 35);

    test(test_loop(5), 5);

    test(test_fptr(fptr), 45);
    test(test_fptr_args(fptr_add, 1, 2), 3);

    printf("\n%d out of %d tests passed\n", passcount, testcount);
    return passcount != testcount;
}
