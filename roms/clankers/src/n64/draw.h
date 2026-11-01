// N64 drawing for Clankers.
#ifndef CK_DRAW_H
#define CK_DRAW_H

#include <stdbool.h>

#include "ck.h"

void draw_init(void);                                  // fonts
void draw_frame(const ck_game *G, bool scanlines);     // the whole screen

#endif
