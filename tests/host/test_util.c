#include "j64/util.h"
#include "test.h"

void test_util(void) {
    EXPECT_EQ_INT(j64_clampi(5, 0, 10), 5);
    EXPECT_EQ_INT(j64_clampi(-3, 0, 10), 0);
    EXPECT_EQ_INT(j64_clampi(42, 0, 10), 10);

    EXPECT_EQ_INT(j64_wrapi(-1, 0, 10), 9);
    EXPECT_EQ_INT(j64_wrapi(10, 0, 10), 0);
    EXPECT_EQ_INT(j64_wrapi(23, 0, 10), 3);
    EXPECT_EQ_INT(j64_wrapi(5, 5, 5), 5);

    EXPECT_EQ_INT(j64_deadzone(4, 8), 0);
    EXPECT_EQ_INT(j64_deadzone(-8, 8), 0);
    EXPECT_EQ_INT(j64_deadzone(20, 8), 12);
    EXPECT_EQ_INT(j64_deadzone(-20, 8), -12);
}
