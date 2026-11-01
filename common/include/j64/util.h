// Small pure helpers shared by every ROM.
//
// Code in common/ must not include libdragon.h, so it can be unit tested on the
// host (tests/host/). Hardware-facing code belongs in each ROM's src/.
#ifndef J64_UTIL_H
#define J64_UTIL_H

// Clamp v into [lo, hi].
int j64_clampi(int v, int lo, int hi);

// Wrap v into [lo, hi), e.g. j64_wrapi(-1, 0, 10) == 9.
int j64_wrapi(int v, int lo, int hi);

// Apply a dead zone to an analog stick axis (-127..127). Values inside
// [-dead, dead] become 0; values outside are rescaled to start from 0.
int j64_deadzone(int axis, int dead);

#endif
