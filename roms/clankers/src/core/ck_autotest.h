// A scripted tour of the game for the self-playing test ROM (clankers-test.z64) and the host
// tests: it presses the buttons, plays 1-1 with a simple bot, and reports checkpoints, PASS or
// FAIL. It drives ck_game_step itself, so the frontend calls ck_autotest_step instead.
#ifndef CLANKERS_CK_AUTOTEST_H
#define CLANKERS_CK_AUTOTEST_H

#include "ck.h"

enum { AT_NONE, AT_CHECK, AT_PASS, AT_FAIL };

typedef struct {
    int step;          // where the tour is
    int stepFrames;    // game frames spent on this step
    int frames;        // game frames since the start
    int hold;          // frames left to hold still (for a screenshot); the game does not step
    int pressed;       // a button to press for one frame, then let go
    int node;          // 1-1's map node
    bool sawPause;
    char msg[96];      // the checkpoint's name, or why it failed
} ck_autotest;

// The bot that plays a level: run right, jump at walls and gaps, boost at the top of a jump.
uint16_t ck_bot(const ck_level *lv);

void ck_autotest_init(ck_autotest *A, const ck_game *G);
// One frame: steps the game (unless holding) and returns AT_CHECK with A->msg set when a
// checkpoint is reached, AT_PASS at the end of the tour, or AT_FAIL with the reason.
int ck_autotest_step(ck_autotest *A, ck_game *G);
// A short name for a scene, for the log.
const char *ck_scene_name(int scene);

#endif
