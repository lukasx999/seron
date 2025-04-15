#include <stdio.h>
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


// Exported functions from ./main.srn
int literals_11(void);
int cond(void);
int variable(void);
int complex(void);
int early_return(void);
int add(int, int, int, int, int, int);
int spill(int, int, int, int, int, int, int, int);

int main(void) {
    int passcount = 0, testcount = 0;
    printf("Testing Compiler\n\n");

    test(literals_11(), 11);
    test(cond(), 45);
    test(variable(), 50);
    test(complex(), 28);
    test(early_return(), 1);
    test(add(0, 0, 0, 0, 0, 0), 0);
    test(add(1, 2, -2, 3, -3, 1), 2);
    // test(spill(0, 0, 0, 0, 0, 0, 0, 0), 0); // TODO:


    printf("\n%d out of %d tests passed\n", passcount, testcount);
    return passcount != testcount;
}
