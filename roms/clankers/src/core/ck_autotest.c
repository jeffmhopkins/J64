#include "ck_autotest.h"

#include <stdio.h>
#include <string.h>

#define HOLD 60                 // one second still at each checkpoint
#define STEP_LIMIT (60 * 240)   // a step that takes over four minutes has failed

uint16_t ck_bot(const ck_level *lv) {
    const ck_player *p = &lv->players[0];
    int tx = (int)((p->x + p->w + 6) / 16), ty = (int)((p->y + p->h + 2) / 16);
    int wy = (int)((p->y + p->h - 4) / 16);
    if (ty > 14) ty = 14;
    if (ty < 0) ty = 0;
    if (wy > 14) wy = 14;
    if (wy < 0) wy = 0;
    char ahead = tx < lv->w ? lv->tiles[ty][tx] : '#';
    char wall = tx < lv->w ? lv->tiles[wy][tx] : '.';
    uint16_t pad = CK_RIGHT;
    bool gap = ahead == '.' && p->ground;
    bool blocked = wall == '#' || wall == '=' || wall == 'X';
    if ((gap || blocked) && p->ground) pad |= CK_A;
    if (!p->ground && p->vy > -1) pad |= (lv->frame / 2) % 2 ? CK_A : 0;
    if (!p->ground && p->vy < 0) pad |= CK_A;
    if (blocked && wall == 'X' && lv->frame % 30 == 0) pad |= CK_B;
    return pad;
}

const char *ck_scene_name(int scene) {
    static const char *names[] = { "title", "select", "map", "shop", "level", "jet", "pause", "intro" };
    return scene >= 0 && scene < (int)(sizeof names / sizeof *names) ? names[scene] : "?";
}

void ck_autotest_init(ck_autotest *A, const ck_game *G) {
    memset(A, 0, sizeof *A);
    A->node = ck_world_node(&G->world, "l1");
}

// The tour, one step at a time.
enum {
    T_INTRO_NIGHT, T_INTRO_STORM, T_INTRO_BLOW, T_SKIP_INTRO, T_TITLE, T_SELECT, T_MAP, T_WALK,
    T_AT_L1, T_LEVEL, T_PAUSE, T_RESUME, T_PLAY, T_BACK, T_DONE,
};

static int check(ck_autotest *A, const char *name) {
    snprintf(A->msg, sizeof A->msg, "%s", name);
    A->hold = HOLD;
    A->step++;
    A->stepFrames = 0;
    return AT_CHECK;
}
static int fail(ck_autotest *A, const ck_game *G, const char *why) {
    snprintf(A->msg, sizeof A->msg, "%s (scene %s, step %d)", why, ck_scene_name(G->scene), A->step);
    A->step = T_DONE;
    return AT_FAIL;
}
static void next(ck_autotest *A) { A->step++; A->stepFrames = 0; }

int ck_autotest_step(ck_autotest *A, ck_game *G) {
    if (A->step == T_DONE) return AT_NONE;
    if (A->hold > 0) { A->hold--; return AT_NONE; }

    // the buttons for this frame
    uint16_t pad = 0;
    if (A->pressed) { pad = (uint16_t)A->pressed; A->pressed = 0; }
    else if (A->step == T_PLAY && G->scene == SC_LEVEL && G->lv.nplayers) pad = ck_bot(&G->lv);
    else if (A->step == T_PLAY && G->scene == SC_JET) pad = CK_RIGHT | ((A->frames / 20) % 2 ? CK_A : 0);
    else if (A->step == T_PLAY && G->scene == SC_PAUSE) pad = CK_START;
    ck_game_step(G, pad, 0);
    A->frames++;
    A->stepFrames++;
    if (A->stepFrames > STEP_LIMIT) return fail(A, G, "timed out");

    switch (A->step) {
    case T_INTRO_NIGHT: if (G->t >= 2.6f) return check(A, "intro: night"); break;
    case T_INTRO_STORM: if (G->t >= 6.4f) return check(A, "intro: storm"); break;
    case T_INTRO_BLOW: if (G->t >= 8.9f) return check(A, "intro: babies blown away"); break;
    case T_SKIP_INTRO:
        if (G->scene == SC_TITLE) return check(A, "title");
        if (A->stepFrames == 2) A->pressed = CK_A;
        if (A->stepFrames > 10) return fail(A, G, "A did not skip the intro");
        break;
    case T_TITLE:
        if (G->scene == SC_SELECT) return check(A, "player select");
        if (A->stepFrames == 2) A->pressed = CK_START;
        if (A->stepFrames > 10) return fail(A, G, "START did not leave the title");
        break;
    case T_SELECT:
        if (G->scene == SC_MAP) return check(A, "map");
        if (A->stepFrames == 2) A->pressed = CK_A;
        if (A->stepFrames > 10) return fail(A, G, "A did not leave player select");
        break;
    case T_MAP:
        if (A->stepFrames == 2) A->pressed = CK_RIGHT;
        if (A->stepFrames > 3) {
            if (!G->map.walking) return fail(A, G, "RIGHT did not walk from home");
            next(A);
        }
        break;
    case T_WALK:
        if (!G->map.walking) {
            if (ck_map_here(G) != A->node) return fail(A, G, "the walk did not reach 1-1");
            return check(A, "map: at 1-1");
        }
        if (A->stepFrames > 600) return fail(A, G, "the walk to 1-1 never ended");
        break;
    case T_AT_L1:
        if (G->scene == SC_LEVEL) { next(A); break; }
        if (A->stepFrames == 2) A->pressed = CK_A;
        if (A->stepFrames > 10) return fail(A, G, "A did not start 1-1");
        break;
    case T_LEVEL:
        if (G->scene != SC_LEVEL) return fail(A, G, "left 1-1 while standing still");
        if (A->stepFrames == 60) return check(A, "1-1: start");
        break;
    case T_PAUSE:
        if (G->scene == SC_PAUSE) return check(A, "pause");
        if (A->stepFrames == 2) A->pressed = CK_START;
        if (A->stepFrames > 10) return fail(A, G, "START did not pause");
        break;
    case T_RESUME:
        if (G->scene == SC_LEVEL) { next(A); break; }
        if (A->stepFrames == 2) A->pressed = CK_START;
        if (A->stepFrames > 10) return fail(A, G, "START did not resume");
        break;
    case T_PLAY:
        if (G->scene == SC_MAP) {
            if (!ck_node_beaten(G, A->node)) return fail(A, G, "the bot ran out of tries on 1-1");
            snprintf(A->msg, sizeof A->msg, "map: 1-1 cleared (lives %d)", G->prog.lives);
            A->hold = HOLD;
            next(A);
            return AT_CHECK;
        }
        break;
    case T_BACK:
        A->step = T_DONE;
        snprintf(A->msg, sizeof A->msg, "tour finished in %d frames", A->frames);
        return AT_PASS;
    }
    return AT_NONE;
}
