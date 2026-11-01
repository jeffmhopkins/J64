// Host test runner. Add a new test file, declare its entry point here, call it.
#include "test.h"

int test_failures = 0;

void test_util(void);

int main(void) {
    test_util();
    if (test_failures) {
        fprintf(stderr, "%d check(s) failed\n", test_failures);
        return 1;
    }
    puts("host tests passed");
    return 0;
}
