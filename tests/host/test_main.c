// Host test runner. Add a new test file, declare its entry point here, call it.
#define _GNU_SOURCE
#include <fenv.h>
#include "test.h"

int test_failures = 0;

void test_util(void);
void test_clankers(void);

int main(void) {
    // trap bad float maths as the N64 does (it stops dead on an invalid operation or a divide by zero)
    feenableexcept(FE_INVALID | FE_DIVBYZERO);
    test_util();
    test_clankers();
    if (test_failures) {
        fprintf(stderr, "%d check(s) failed\n", test_failures);
        return 1;
    }
    puts("host tests passed");
    return 0;
}
