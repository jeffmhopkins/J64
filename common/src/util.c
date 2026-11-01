#include "j64/util.h"

int j64_clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int j64_wrapi(int v, int lo, int hi) {
    int span = hi - lo;
    if (span <= 0) return lo;
    int r = (v - lo) % span;
    if (r < 0) r += span;
    return lo + r;
}

int j64_deadzone(int axis, int dead) {
    if (axis > dead) return axis - dead;
    if (axis < -dead) return axis + dead;
    return 0;
}
