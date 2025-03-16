#include <stdio.h>
#include <assert.h>

// Exported functions from ./main.srn
int add(int, int);
int square(int);


int main(void) {
    puts("Running tests");

    assert(add(1, 2) == 3);
    assert(add(90, 10) == 100);

    assert(square(10) == 100);
    assert(square(5) == 25);

    puts("Tests passed");
    return 0;
}
