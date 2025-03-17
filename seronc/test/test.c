#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define test(expr, expected) do {                                             \
testcount++;                                                                  \
int value = (expr);                                                           \
if (value == (expected)) {                                                    \
    passcount++;                                                              \
} else {                                                                      \
    fprintf(stderr, "`" #expr " == " #expected "` failed, got %d\n", value ); \
}                                                                             \
} while (0)



// Exported functions from ./main.srn
int add(int, int);
int square(int);
int sum(int, int, int, int, int, int);
int sum_literals_21(void);
int sum_literals_3(void);
int sum_literals_many(void);
int cond(bool, int then, int else_);
int assign(int inital, int new);
int loop(int iters);
int sum_binop(void);



int main(void) {
    int passcount = 0, testcount = 0;
    printf("Running tests\n\n");

    test(add(0, 0), 0);
    test(add(1, 2), 3);
    test(add(90, 10), 100);
    test(add(-1, 2), 1);
    test(square(10), 100);
    test(square(100), 10000);
    test(square(5), 25);
    test(square(1), 1);
    test(square(0), 0);
    test(square(-5), 25);
    test(square(-1), 1);
    test(sum(0, 0, 0, 0, 0, 0), 0);
    test(sum(1, 2, 3, 4, 5, 6), 21);
    test(sum(10, 10, 10, 10, 10, 10), 60);
    test(sum(-5, 5, 3, -3, 10, 10), 20);
    test(cond(true, 1, 2), 1);
    test(cond(false, 1, 2), 2);
    test(assign(5, 1), 1);
    test(assign(1, 5), 5);
    test(assign(0, 0), 0);
    test(assign(5, 5), 5);
    test(assign(5, -1), -1);
    test(sum_literals_21(), 21);
    test(sum_literals_3(), 3);
    test(sum_literals_many(), 225);
    test(loop(5), 5);
    test(sum_binop(), 10);

    printf("\n%d out of %d tests passed\n", passcount, testcount);
    return passcount != testcount;
}
