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

int main(void) {
    int passcount = 0, testcount = 0;
    printf("Testing Compiler\n\n");

    test(literals_11(), 11);
    test(cond(), 45);

    printf("\n%d out of %d tests passed\n", passcount, testcount);
    return passcount != testcount;
}
