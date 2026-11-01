// Minimal unit-test helpers for host tests. No dependencies, plain C.
#ifndef J64_TEST_H
#define J64_TEST_H

#include <stdio.h>

extern int test_failures;

#define EXPECT_EQ_INT(actual, expected)                                          \
    do {                                                                         \
        long long a_ = (long long)(actual), e_ = (long long)(expected);          \
        if (a_ != e_) {                                                          \
            fprintf(stderr, "%s:%d: %s == %lld, expected %lld\n", __FILE__,      \
                    __LINE__, #actual, a_, e_);                                  \
            test_failures++;                                                     \
        }                                                                        \
    } while (0)

#endif
