// The house's newer little games: the pinball table and the Bug Blaster cabinet in the arcade
// room, and fishing in the yard's pond. Mirrors pinballScene, blastScene and fishScene in
// web/arcade.js.
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ck_internal.h"

static int dir_x(const ck_game *G, int s) { return (HELD(G, s, CK_RIGHT) ? 1 : 0) - (HELD(G, s, CK_LEFT) ? 1 : 0); }
static int dir_y(const ck_game *G, int s) { return (HELD(G, s, CK_DOWN) ? 1 : 0) - (HELD(G, s, CK_UP) ? 1 : 0); }
static bool any_held(const ck_game *G, uint16_t b) { return HELD(G, 0, b) || HELD(G, 1, b); }

// ---------------------------------------------------------------------------- pinball
// The table: walls round the top, an orbit up the left, a launch lane down the right, slingshots
// and two guides into the flippers. The last two walls are the guides.
const ck_seg ck_pin_walls[CK_PIN_WALLS] = {
    { 10, 352, 10, 60 }, { 10, 60, 24, 28 }, { 24, 28, 52, 10 }, { 52, 10, 182, 10 }, { 182, 10, 212, 24 },
    { 212, 24, 230, 50 }, { 230, 50, 230, 440 }, { 212, 440, 230, 440 }, { 212, 440, 212, 96 },
    { 34, 250, 34, 84 },                        // the left orbit's inner wall
    { 95, 18, 95, 40 }, { 127, 18, 127, 40 },   // between the rollover lanes
    { 30, 316, 30, 346 }, { 30, 346, 58, 362 }, { 192, 316, 192, 346 }, { 192, 346, 164, 362 },   // the slingshots' backs
    { 194, 262, 212, 252 },                     // the upper flipper's bracket
    { 10, 352, 76, 398 }, { 212, 352, 146, 398 },
};
const ck_bump ck_pin_bumps[CK_PIN_BUMPS] = {
    { 82, 84, 11, 100 }, { 140, 84, 11, 100 }, { 111, 118, 11, 100 }, { 60, 160, 6, 50 }, { 168, 132, 6, 50 },
};
const ck_seg ck_pin_gate = { 230, 74, 212, 96 };
const ck_seg ck_pin_spinner = { 10, 150, 34, 150 };
const ck_seg ck_pin_slings[CK_PIN_SLINGS] = { { 30, 316, 58, 362 }, { 192, 316, 164, 362 } };
const ck_seg ck_pin_drops[CK_PIN_DROPS] = { { 205, 150, 205, 162 }, { 205, 166, 205, 178 }, { 205, 182, 205, 194 }, { 205, 198, 205, 210 } };
const float ck_pin_lights[CK_PIN_LIGHTS][2] = { { 80, 30 }, { 111, 30 }, { 142, 30 } };
const float ck_pin_saucer[2] = { 150, 176 };
const ck_flipdef ck_pin_flips[CK_PIN_FLIPS] = { { 78, 400, 27, 0 }, { 144, 400, 27, 1 }, { 194, 262, 22, 1 } };
#define FLIP_REST 0.5f
#define FLIP_UP (-0.45f)
#define LANE_X 221
#define LANE_Y 434
#define GRAV 0.12f
#define MAXV 10.0f

void ck_pin_flipper(int k, float a, float *x0, float *y0, float *x1, float *y1) {
    const ck_flipdef *f = &ck_pin_flips[k];
    *x0 = f->x; *y0 = f->y;
    *x1 = *x0 + (f->side ? -1 : 1) * cosf(a) * f->len;
    *y1 = *y0 + sinf(a) * f->len;
}
static void pin_new_ball(ck_pinball *P) {
    memset(P->ball, 0, sizeof P->ball);
    P->ball[0] = (ck_pball){ LANE_X, LANE_Y, 0, 0, true, 0 };
}
static void pin_reset(ck_pinball *P) {
    int best = P->best;
    memset(P, 0, sizeof *P);
    P->best = best;
    P->balls = CK_PIN_BALLS;
    for (int k = 0; k < CK_PIN_FLIPS; k++) P->flip[k] = FLIP_REST;
    P->cam = CK_PIN_H - CK_PIN_VIEW;
    pin_new_ball(P);
}
void ck_pin_start(ck_game *G) { ck_house_game_heroes(G); pin_reset(&G->house.pin); ck_go(G, SC_PINBALL); }
// a spot on the table on the screen, for sparks and confetti
static float pin_sx(float x) { return CK_PIN_OX + x * CK_PIN_ZOOM; }
static float pin_sy(const ck_pinball *P, float y) { return (y - P->cam) * CK_PIN_ZOOM; }
static void pin_add(ck_pinball *P, int n) { P->score += P->x2T > 0 ? n * 2 : n; }
static void pin_say(ck_pinball *P, int msg) { P->msg = msg; P->msgT = 90; }
static int pin_live(const ck_pinball *P) { int n = 0; for (int i = 0; i < CK_PIN_MULTI; i++) n += P->ball[i].on; return n; }
bool ck_pin_in_lane(const ck_pball *b) { return b->on && b->x > 212 && b->y > LANE_Y - 9 && fabsf(b->vy) < 1; }

// Push the ball out of a fat line from a to b and bounce it. A flipper's line turns about its
// pivot at w radians a frame (side picks the way it turns), which flicks the ball.
static bool pin_seg(ck_pball *P, float ax, float ay, float bx, float by, float rr, float e, int side, float w) {
    float dx = bx - ax, dy = by - ay, len2 = dx * dx + dy * dy;
    float t = len2 > 0 ? ck_clampf(((P->x - ax) * dx + (P->y - ay) * dy) / len2, 0, 1) : 0;
    float cx = ax + dx * t, cy = ay + dy * t, nx = P->x - cx, ny = P->y - cy, d = sqrtf(nx * nx + ny * ny);
    if (d >= CK_PIN_R + rr || d < 0.0001f) return false;
    nx /= d; ny /= d;
    P->x = cx + nx * (CK_PIN_R + rr); P->y = cy + ny * (CK_PIN_R + rr);
    float svx = 0, svy = 0;
    if (w != 0) {   // the flipper's speed where the ball touches it
        float rx = cx - ax, ry = cy - ay;
        if (side == 0) { svx = -w * ry; svy = w * rx; }
        else { svx = w * ry; svy = -w * rx; }
    }
    float rvx = P->vx - svx, rvy = P->vy - svy, vn = rvx * nx + rvy * ny;
    if (vn < 0) { P->vx -= (1 + e) * vn * nx; P->vy -= (1 + e) * vn * ny; }
    return true;
}

// one ball's quarter step against everything on the table
static void pin_ball(ck_game *G, ck_pinball *P, ck_pball *b, const float *was, int sub) {
    float py = b->y;
    b->vy += GRAV / 4;
    b->x += b->vx / 4; b->y += b->vy / 4;
    for (int k = 0; k < CK_PIN_WALLS; k++) {
        const ck_seg *w = &ck_pin_walls[k];
        pin_seg(b, w->x0, w->y0, w->x1, w->y1, 1, 0.5f, 0, 0);
    }
    // the lane's gate only lets the ball out: falling onto it, it rolls off onto the table
    if (b->vy > 0) pin_seg(b, ck_pin_gate.x0, ck_pin_gate.y0, ck_pin_gate.x1, ck_pin_gate.y1, 1, 0.4f, 0, 0);
    for (int s = 0; s < CK_PIN_FLIPS; s++) {
        float a = was[s] + P->flipW[s] * (sub + 1) / 4, x0, y0, x1, y1;
        ck_pin_flipper(s, a, &x0, &y0, &x1, &y1);
        pin_seg(b, x0, y0, x1, y1, 3, 0.3f, ck_pin_flips[s].side, P->flipW[s]);
    }
    // slingshots kick the ball back out
    for (int k = 0; k < CK_PIN_SLINGS; k++) {
        const ck_seg *w = &ck_pin_slings[k];
        float vx = b->vx, vy = b->vy;
        if (!pin_seg(b, w->x0, w->y0, w->x1, w->y1, 1, 0.5f, 0, 0)) continue;
        float nx = w->y1 - w->y0, ny = w->x0 - w->x1, l = sqrtf(nx * nx + ny * ny);
        nx /= l; ny /= l;
        if (k == 1) { nx = -nx; ny = -ny; }
        if (P->sling[k] < 6 && (vx * nx + vy * ny) < -0.5f) {
            b->vx += nx * 3.2f; b->vy += ny * 3.2f;
            pin_add(P, 10); P->sling[k] = 10; ck_sfx(G, SFX_KICK);
        }
    }
    // drop targets go down when hit; drop the whole bank for double scoring
    for (int k = 0; k < CK_PIN_DROPS; k++) {
        if (P->drops & (1 << k)) continue;
        const ck_seg *w = &ck_pin_drops[k];
        if (!pin_seg(b, w->x0, w->y0, w->x1, w->y1, 2, 0.5f, 0, 0)) continue;
        P->drops |= 1 << k; pin_add(P, 200); ck_sfx(G, SFX_CRATE);
        ck_sparks(G, pin_sx(w->x0), pin_sy(P, (w->y0 + w->y1) / 2), 4, 0xFF8A3D);
        if (P->drops == (1 << CK_PIN_DROPS) - 1) {
            pin_add(P, 1500); P->x2T = 60 * 20; P->dropT = 60; pin_say(P, PINMSG_DOUBLE); ck_sfx(G, SFX_POWER);
        }
    }
    for (int k = 0; k < CK_PIN_BUMPS; k++) {
        const ck_bump *u = &ck_pin_bumps[k];
        float nx = b->x - u->x, ny = b->y - u->y, d = sqrtf(nx * nx + ny * ny);
        if (d >= u->r + CK_PIN_R || d < 0.0001f) continue;
        nx /= d; ny /= d;
        b->x = u->x + nx * (u->r + CK_PIN_R); b->y = u->y + ny * (u->r + CK_PIN_R);
        float vn = b->vx * nx + b->vy * ny;
        if (vn < 0) { b->vx -= 2 * vn * nx; b->vy -= 2 * vn * ny; }
        float kick = u->score >= 100 ? 2.6f : 1.8f;   // bumpers kick the ball away
        b->vx += nx * kick; b->vy += ny * kick;
        if (P->flash[k] < 8) { pin_add(P, u->score); ck_sfx(G, u->score >= 100 ? SFX_BOING : SFX_HOP); ck_sparks(G, pin_sx(u->x + nx * u->r), pin_sy(P, u->y + ny * u->r), 4, 0xFFD23F); }
        P->flash[k] = 12;
    }
    // the spinner: going through it spins it, faster the faster the ball goes
    const ck_seg *sp = &ck_pin_spinner;
    if (b->x > sp->x0 && b->x < sp->x1 && (py - sp->y0) * (b->y - sp->y0) <= 0 && py != b->y) P->spin = ck_minf(P->spin + fabsf(b->vy) * 0.08f, 1.2f);
    float v = sqrtf(b->vx * b->vx + b->vy * b->vy);
    if (v > MAXV) { b->vx *= MAXV / v; b->vy *= MAXV / v; }
}

void ck_pin_update(ck_game *G) {
    ck_pinball *P = &G->house.pin;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { ck_house_back(G); return; }
    if (P->over) {
        if (++P->overT > 60 && ANYHIT(G, CK_A)) { pin_reset(P); ck_sfx(G, SFX_CONFIRM); }
        return;
    }
    // the flippers: left flips the left one, right the right one and the upper one
    bool hold[2] = { any_held(G, CK_LEFT | CK_B | CK_L), any_held(G, CK_RIGHT | CK_A | CK_R) };
    float was[CK_PIN_FLIPS];
    for (int s = 0; s < CK_PIN_FLIPS; s++) {
        bool h = hold[ck_pin_flips[s].side];
        was[s] = P->flip[s];
        if (h) P->flip[s] = ck_maxf(FLIP_UP, P->flip[s] - 0.32f);
        else P->flip[s] = ck_minf(FLIP_REST, P->flip[s] + 0.18f);
        if (h && was[s] == FLIP_REST && s < 2) ck_sfx(G, SFX_CLANK);
        P->flipW[s] = P->flip[s] - was[s];
    }
    // the launch lane: A fires a waiting ball up the lane
    for (int i = 0; i < CK_PIN_MULTI; i++)
        if (ck_pin_in_lane(&P->ball[i]) && ANYHIT(G, CK_A)) {
            P->ball[i].vy = -10.0f - ck_rand(G) * 0.4f;
            ck_sfx(G, SFX_SPRING);
        }
    for (int i = 0; i < CK_PIN_MULTI; i++) {
        ck_pball *b = &P->ball[i];
        if (!b->on) continue;
        if (b->held > 0) {   // sitting in the saucer
            if (--b->held == 0) { b->vx = -2.4f + ck_rand(G); b->vy = -4.2f; b->y -= 8; ck_sfx(G, SFX_BLOW); }
            continue;
        }
        for (int sub = 0; sub < 4; sub++) pin_ball(G, P, b, was, sub);
        // the saucer catches a slow ball; every second catch starts multiball
        float dx = b->x - ck_pin_saucer[0], dy = b->y - ck_pin_saucer[1];
        if (dx * dx + dy * dy < 36 && b->vx * b->vx + b->vy * b->vy < 30) {
            b->x = ck_pin_saucer[0]; b->y = ck_pin_saucer[1]; b->vx = b->vy = 0; b->held = 50;
            pin_add(P, 500); ck_sfx(G, SFX_GRAB);
            if (pin_live(P) == 1 && ++P->locks % 2 == 0) {
                for (int j = 0; j < CK_PIN_MULTI; j++) {
                    if (P->ball[j].on) continue;
                    P->ball[j] = (ck_pball){ ck_pin_saucer[0], ck_pin_saucer[1] - 8, j == 1 ? 2.2f : -1.0f, -5.0f, true, 0 };
                }
                pin_say(P, PINMSG_MULTI); ck_sfx(G, SFX_FLAG); ck_confetti(G, pin_sx(ck_pin_saucer[0]), pin_sy(P, ck_pin_saucer[1]), 30);
            } else if (pin_live(P) == 1) pin_say(P, PINMSG_LOCK);
        }
        // down the middle: that ball is lost
        if (b->y > CK_PIN_DRAIN) {
            b->on = false;
            if (pin_live(P) > 0) continue;
            ck_sfx(G, SFX_OUCH);
            if (--P->balls <= 0) { P->over = true; P->overT = 0; if (P->score > P->best) P->best = P->score; }
            else pin_new_ball(P);
        }
    }
    for (int k = 0; k < CK_PIN_BUMPS; k++) if (P->flash[k] > 0) P->flash[k]--;
    for (int k = 0; k < CK_PIN_SLINGS; k++) if (P->sling[k] > 0) P->sling[k]--;
    // the spinner scores each turn it makes
    if (P->spin > 0.01f) {
        int turns = (int)(P->spinA + P->spin) - (int)P->spinA;
        P->spinA += P->spin; P->spin *= 0.97f;
        if (turns > 0) { pin_add(P, 25 * turns); ck_sfx(G, SFX_TICK); }
    } else P->spin = 0;
    if (P->dropT > 0 && --P->dropT == 0) P->drops = 0;   // the bank pops back up
    if (P->x2T > 0) P->x2T--;
    if (P->msgT > 0) P->msgT--;
    // the lights along the top: light all three for a bonus
    for (int i = 0; i < CK_PIN_MULTI; i++) {
        const ck_pball *b = &P->ball[i];
        if (!b->on) continue;
        for (int k = 0; k < CK_PIN_LIGHTS; k++) {
            if (P->lit & (1 << k) || hypotf(b->x - ck_pin_lights[k][0], b->y - ck_pin_lights[k][1]) > 8) continue;
            P->lit |= 1 << k; pin_add(P, 250); ck_sfx(G, SFX_STAR1);
            if (P->lit == (1 << CK_PIN_LIGHTS) - 1) { pin_add(P, 1000); P->lit = 0; P->litT = 90; pin_say(P, PINMSG_BONUS); ck_sfx(G, SFX_FLAG); ck_confetti(G, 160, 30, 30); }
        }
    }
    if (P->litT > 0) P->litT--;
    // the view follows the lowest ball
    float low = -1;
    for (int i = 0; i < CK_PIN_MULTI; i++) if (P->ball[i].on) low = ck_maxf(low, P->ball[i].y);
    if (low >= 0) P->cam += (ck_clampf(low - 170, 0, CK_PIN_H - CK_PIN_VIEW) - P->cam) * 0.12f;
}

// ---------------------------------------------------------------------------- Bug Blaster
const float ck_shield_x[CK_SHIELDS] = { 52, 142, 232 };
void ck_bug_at(const ck_blaster *B, int k, float *x, float *y) { *x = B->bx + (k % CK_BUG_COLS) * CK_BUG_DX; *y = B->by + (k / CK_BUG_COLS) * CK_BUG_DY; }
static void blast_wave(ck_blaster *B) {
    B->bugs = 0xFFFFFFFFu;
    B->bx = 76; B->by = 40 + (B->wave < 5 ? B->wave : 5) * 6;
    B->dir = 1; B->stepT = 0;
    for (int k = 0; k < CK_BLAST_BOMBS; k++) B->bomb[k].on = false;
    B->shot[0].on = B->shot[1].on = false;
    B->waveT = 90;
}
static void blast_reset(ck_blaster *B) {
    int best = B->best;
    memset(B, 0, sizeof *B);
    B->best = best;
    B->lives = 3;
    B->gun[0] = 140; B->gun[1] = 180;
    for (int s = 0; s < CK_SHIELDS; s++) memset(B->shield[s], 1, sizeof B->shield[s]);
    blast_wave(B);
}
void ck_blast_start(ck_game *G) { ck_house_game_heroes(G); blast_reset(&G->house.blast); ck_go(G, SC_BLAST); }
static int count_bits(uint32_t v) { int n = 0; while (v) { v &= v - 1; n++; } return n; }
// does a shot or bomb at (x, y) hit a shield? It takes the cell away.
static bool shield_hit(ck_blaster *B, float x, float y) {
    for (int s = 0; s < CK_SHIELDS; s++) {
        int cx = (int)floorf((x - ck_shield_x[s]) / CK_SHIELD_CELL), cy = (int)floorf((y - CK_SHIELD_Y) / CK_SHIELD_CELL);
        if (cx < 0 || cy < 0 || cx >= CK_SHIELD_W || cy >= CK_SHIELD_H || !B->shield[s][cy * CK_SHIELD_W + cx]) continue;
        B->shield[s][cy * CK_SHIELD_W + cx] = 0;
        return true;
    }
    return false;
}
void ck_blast_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_blaster *B = &H->blast;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { ck_house_back(G); return; }
    if (B->over) {
        if (++B->overT > 60 && ANYHIT(G, CK_A)) { blast_reset(B); ck_sfx(G, SFX_CONFIRM); }
        return;
    }
    if (B->waveT > 0) B->waveT--;
    for (int i = 0; i < H->np; i++) {
        int s = H->gs[i].slot;
        B->gun[i] = ck_clampf(B->gun[i] + dir_x(G, s) * 2.4f, 14, CK_W - 14);
        if (B->inv[i] > 0) B->inv[i]--;
        if ((HIT(G, s, CK_A) || HIT(G, s, CK_B)) && !B->shot[i].on && B->waveT <= 0) {
            B->shot[i] = (ck_shot){ B->gun[i], CK_BLAST_Y - 10, true };
            ck_sfx(G, SFX_BLOW);
        }
        ck_shot *sh = &B->shot[i];
        if (!sh->on) continue;
        sh->y -= 6;
        if (sh->y < 10 || shield_hit(B, sh->x, sh->y)) { sh->on = false; continue; }
        for (int k = 0; k < CK_BUG_COLS * CK_BUG_ROWS; k++) {
            if (!(B->bugs & (1u << k))) continue;
            float bx, by;
            ck_bug_at(B, k, &bx, &by);
            if (fabsf(sh->x - bx) > 8 || fabsf(sh->y - by) > 7) continue;
            B->bugs &= ~(1u << k);
            sh->on = false;
            B->score += (CK_BUG_ROWS - k / CK_BUG_COLS) * 10;
            ck_sfx(G, SFX_POP);
            ck_sparks(G, bx, by, 6, k / CK_BUG_COLS == 0 ? 0xFF5A5A : k / CK_BUG_COLS == 1 ? 0xC28BFF : 0x49D17A);
            break;
        }
    }
    if (!B->bugs) {   // a new wave, a little lower and quicker
        B->wave++;
        ck_sfx(G, SFX_FLAG); ck_confetti(G, 160, 40, 30);
        blast_wave(B);
        return;
    }
    // the bugs march: fewer bugs, faster steps
    int alive = count_bits(B->bugs);
    if (B->waveT <= 0 && ++B->stepT >= 3 + alive / 3 - (B->wave < 3 ? B->wave : 3)) {
        B->stepT = 0; B->frame ^= 1;
        float lo = 999, hi = -999, low = 0;
        for (int k = 0; k < 32; k++) if (B->bugs & (1u << k)) { float x, y; ck_bug_at(B, k, &x, &y); lo = ck_minf(lo, x); hi = ck_maxf(hi, x); low = ck_maxf(low, y); }
        if ((B->dir > 0 && hi + 4 > CK_W - 12) || (B->dir < 0 && lo - 4 < 12)) { B->dir = -B->dir; B->by += 8; }
        else B->bx += B->dir * 4;
        // no march beep: the music carries the tension
        if (low + 8 >= CK_SHIELD_Y) {   // they reached the shields: game over
            B->over = true; B->overT = 0; if (B->score > B->best) B->best = B->score;
            ck_sfx(G, SFX_OUCH);
            return;
        }
    }
    // the lowest bug in a column drops a bomb now and then
    if (B->waveT <= 0 && ck_rand(G) < 0.02f + B->wave * 0.006f) {
        int col = (int)(ck_rand(G) * CK_BUG_COLS) % CK_BUG_COLS;
        for (int r = CK_BUG_ROWS - 1; r >= 0; r--) {
            int k = r * CK_BUG_COLS + col;
            if (!(B->bugs & (1u << k))) continue;
            for (int b = 0; b < CK_BLAST_BOMBS; b++) if (!B->bomb[b].on) { float x, y; ck_bug_at(B, k, &x, &y); B->bomb[b] = (ck_shot){ x, y + 6, true }; break; }
            break;
        }
    }
    for (int b = 0; b < CK_BLAST_BOMBS; b++) {
        ck_shot *bm = &B->bomb[b];
        if (!bm->on) continue;
        bm->y += 1.6f + B->wave * 0.15f;
        if (bm->y > CK_H || shield_hit(B, bm->x, bm->y)) { bm->on = false; continue; }
        for (int i = 0; i < H->np; i++) {
            if (B->inv[i] > 0 || fabsf(bm->x - B->gun[i]) > 10 || fabsf(bm->y - CK_BLAST_Y) > 7) continue;
            bm->on = false;
            B->inv[i] = 120;
            ck_sfx(G, SFX_CLANK);
            ck_puff(G, B->gun[i], CK_BLAST_Y, 8, 0xFFD23F, 230);
            if (--B->lives <= 0) { B->over = true; B->overT = 0; if (B->score > B->best) B->best = B->score; ck_sfx(G, SFX_OUCH); }
            break;
        }
    }
}

// ---------------------------------------------------------------------------- fishing
const ck_catch ck_catches[CATCH_COUNT] = {
    { "Minnow", 0xA9C4D6, 0xA9C4D6, 7, 1.2f, 0.6f, 1 },
    { "Goldfish", 0xFF8A3D, 0xFFC27A, 9, 0.9f, 0.9f, 2 },
    { "Big bass", 0x5E8C4A, 0xC9D98A, 15, 0.7f, 1.5f, 3 },
    { "Golden fish", 0xFFD23F, 0xFFF6A0, 11, 1.6f, 2.0f, 10 },
    { "Old boot", 0x6F4630, 0x3A2A20, 10, 0, 0.2f, 0 },
    { "Sardine", 0xBFC8D4, 0xBFC8D4, 6, 1.0f, 0.5f, 1 },
    { "Perch", 0xD9C24A, 0x3F5A2E, 10, 1.0f, 1.1f, 2 },
    { "Rainbow trout", 0x8FB8A8, 0xFF7AA8, 13, 1.3f, 1.6f, 4 },
    { "Catfish", 0x6A5E50, 0x3A3028, 16, 0.6f, 2.0f, 5 },
    { "Pike", 0x7A9A4A, 0xE6E0A0, 18, 1.5f, 2.4f, 7 },
    { "Koi", 0xFFF6E6, 0xFF5A2A, 14, 0.8f, 1.7f, 6 },
    { "Robo-fish", 0x9AA4B4, 0x5AD1FF, 14, 1.8f, 2.6f, 9 },
    { "King carp", 0xC98A3A, 0xFFD23F, 22, 0.6f, 3.0f, 12 },
};
const int ck_pond_level_at[CK_POND_LEVELS] = { 0, 6, 15, 30, 50 };
int ck_pond_level(int fished) {
    int l = 0;
    while (l + 1 < CK_POND_LEVELS && fished >= ck_pond_level_at[l + 1]) l++;
    return l;
}
// who lives in the pond at each level, and how many in a hundred
typedef struct { uint8_t kind, w; } pond_pop;
static const pond_pop pond_pops[CK_POND_LEVELS][8] = {
    { { CATCH_MINNOW, 45 }, { CATCH_SARDINE, 45 }, { CATCH_BOOT, 10 } },
    { { CATCH_MINNOW, 20 }, { CATCH_SARDINE, 10 }, { CATCH_GOLDFISH, 30 }, { CATCH_PERCH, 30 }, { CATCH_BOOT, 10 } },
    { { CATCH_GOLDFISH, 15 }, { CATCH_PERCH, 20 }, { CATCH_BASS, 20 }, { CATCH_TROUT, 20 }, { CATCH_CATFISH, 12 }, { CATCH_GOLDEN, 5 }, { CATCH_BOOT, 8 } },
    { { CATCH_BASS, 15 }, { CATCH_TROUT, 15 }, { CATCH_CATFISH, 15 }, { CATCH_PIKE, 20 }, { CATCH_KOI, 20 }, { CATCH_GOLDEN, 7 }, { CATCH_BOOT, 8 } },
    { { CATCH_TROUT, 10 }, { CATCH_PIKE, 15 }, { CATCH_KOI, 15 }, { CATCH_CATFISH, 10 }, { CATCH_ROBO, 22 }, { CATCH_KING, 15 }, { CATCH_GOLDEN, 8 }, { CATCH_BOOT, 5 } },
};
void ck_rod_tip(int i, float *x, float *y) { *x = 46 + i * 14; *y = 62 + i * 6; }
static void pond_fish(ck_game *G, ck_pfish *f) {
    const pond_pop *pop = pond_pops[G->house.pond.level];
    int r = (int)(ck_rand(G) * 100), k = 0;
    while (k < 7 && pop[k + 1].w && r >= pop[k].w) r -= pop[k++].w;
    f->kind = pop[k].kind;
    f->x = 110 + ck_rand(G) * 190;
    f->y = f->kind == CATCH_BOOT ? CK_POND_BED - 6 : CK_POND_TOP + 18 + ck_rand(G) * (CK_POND_BED - CK_POND_TOP - 30);
    f->vx = f->kind == CATCH_BOOT ? 0 : (ck_rand(G) < 0.5f ? -1 : 1) * ck_catches[f->kind].speed * 0.5f;
    f->scare = 0;
}
void ck_fish_start(ck_game *G) {
    ck_house *H = &G->house;
    ck_house_game_heroes(G);
    ck_pond *Pd = &H->pond;
    memset(Pd->rod, 0, sizeof Pd->rod);
    for (int i = 0; i < 2; i++) Pd->rod[i].fish = -1;
    Pd->level = ck_pond_level(G->saves.closet.fished); Pd->levelT = 0;
    for (int k = 0; k < CK_POND_FISH; k++) pond_fish(G, &Pd->fish[k]);
    Pd->last = -1;
    ck_go(G, SC_FISH);
}
static bool fish_taken(const ck_pond *Pd, int k) { return Pd->rod[0].fish == k || Pd->rod[1].fish == k; }
void ck_fish_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_pond *Pd = &H->pond;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { ck_house_back(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_rod *R = &Pd->rod[i];
        int s = H->gs[i].slot;
        float tx, ty;
        ck_rod_tip(i, &tx, &ty);
        switch (R->state) {
        case ROD_IDLE:
            if (HIT(G, s, CK_A)) { R->state = ROD_CHARGE; R->power = 0; R->t = 0; }
            break;
        case ROD_CHARGE:   // the power swings up and down while A is held
            R->t++;
            R->power = 0.5f - 0.5f * cosf(R->t * 0.06f);
            if (!HELD(G, s, CK_A)) {
                R->state = ROD_FLY; R->x = tx; R->y = ty;
                R->vx = 1.6f + R->power * 3.4f; R->vy = -2.6f;
                ck_sfx(G, SFX_WHOOSH);
            }
            break;
        case ROD_FLY:
            R->vy += 0.12f; R->x += R->vx; R->y += R->vy;
            if (R->x > CK_W - 8) { R->x = CK_W - 8; R->vx = 0; }
            if (R->y >= CK_POND_TOP) { R->y = CK_POND_TOP + 2; R->state = ROD_WAIT; R->fish = -1; R->nibble = 0; ck_sfx(G, SFX_POP); ck_puff(G, R->x, CK_POND_TOP, 5, 0xDCFAFF, 220); }
            break;
        case ROD_WAIT:
        case ROD_BITE:
            R->y = ck_clampf(R->y + dir_y(G, s) * 0.8f + 0.1f, CK_POND_TOP + 2, CK_POND_BED - 6);
            if (HIT(G, s, CK_B)) {   // reel the empty line back in
                if (R->fish >= 0) Pd->fish[R->fish].scare = 60;
                R->state = ROD_IDLE; R->fish = -1; ck_sfx(G, SFX_SELECT);
                break;
            }
            if (R->state == ROD_BITE) {
                if (HIT(G, s, CK_A)) { R->state = ROD_REEL; R->tension = 20; R->t = 0; ck_sfx(G, SFX_GRAB); }
                else if (--R->t <= 0) { Pd->fish[R->fish].scare = 90; R->fish = -1; R->state = ROD_WAIT; }   // too slow: it got away
                break;
            }
            if (R->fish < 0) {   // a fish near the hook comes to look
                for (int k = 0; k < CK_POND_FISH; k++) {
                    const ck_pfish *f = &Pd->fish[k];
                    if (f->scare > 0 || fish_taken(Pd, k) || hypotf(f->x - R->x, f->y - R->y) > 50) continue;
                    R->fish = k; R->nibble = 2 + (int)(ck_rand(G) * 3); R->t = 40;
                    break;
                }
            } else {
                ck_pfish *f = &Pd->fish[R->fish];
                float d = hypotf(f->x - R->x, f->y - R->y);
                if (d > 6) {
                    f->x += (R->x - f->x) * 0.04f; f->y += (R->y - f->y) * 0.04f;
                    f->vx = R->x > f->x ? fabsf(f->vx) + 0.001f : -fabsf(f->vx) - 0.001f;
                } else if (--R->t <= 0) {   // a nibble, then another, then a real bite
                    if (--R->nibble > 0) { R->t = 30 + (int)(ck_rand(G) * 40); ck_sfx(G, SFX_TICK); }
                    else { R->state = ROD_BITE; R->t = 50 - (int)(ck_catches[f->kind].pull * 6); ck_sfx(G, SFX_BUTTON); }   // big fish bite quick
                }
            }
            break;
        case ROD_REEL: {   // the fish pulls in bursts; hold A to reel, let go when the line is tight
            ck_pfish *f = &Pd->fish[R->fish];
            const ck_catch *c = &ck_catches[f->kind];
            R->t++;
            if (R->t % 50 == 0) R->pulling = ck_rand(G) < 0.6f;
            if (HELD(G, s, CK_A)) {
                float dx = tx - R->x, dy = ty - R->y, d = hypotf(dx, dy);
                R->x += dx / d * 1.1f; R->y += dy / d * 0.7f;
                R->tension += R->pulling ? 1.3f * c->pull : 0.25f;
                if (R->t % 8 == 0) ck_sfx(G, SFX_TICK);
            } else {
                R->tension -= 1.4f;
                if (R->pulling) R->x = ck_minf(CK_W - 8, R->x + 0.7f * c->pull);
            }
            if (R->pulling) R->y = ck_clampf(R->y + sinf(R->t * 0.2f) * 0.8f, CK_POND_TOP + 4, CK_POND_BED - 6);
            R->tension = ck_clampf(R->tension, 0, 100);
            f->x = R->x; f->y = ck_maxf(R->y, CK_POND_TOP + 4);
            if (R->tension >= 100) {   // snap!
                R->state = ROD_SNAP; R->t = 90; f->scare = 120; R->fish = -1;
                ck_sfx(G, SFX_NOPE);
            } else if (R->x <= CK_POND_BANK + 4) {   // landed it
                R->state = ROD_GOT; R->t = 150; Pd->last = f->kind;
                Pd->caught++; Pd->got[f->kind]++;
                G->prog.bolts += c->bolts;
                if (f->kind != CATCH_BOOT && G->saves.closet.fished < 60000) G->saves.closet.fished++;
                int lv = ck_pond_level(G->saves.closet.fished);
                if (lv > Pd->level) {   // the pond levels up: the fish not on a line make way for bigger ones
                    Pd->level = lv; Pd->levelT = 180;
                    for (int k = 0; k < CK_POND_FISH; k++) if (!fish_taken(Pd, k) && k != (int)(f - Pd->fish)) { pond_fish(G, &Pd->fish[k]); Pd->fish[k].x = CK_W - 10 - k * 6; }
                    ck_confetti(G, 160, 40, 40);
                }
                ck_save(G);
                ck_sfx(G, f->kind == CATCH_BOOT ? SFX_CLANK : SFX_GEAR);
                if (f->kind == CATCH_GOLDEN) ck_confetti(G, 160, 40, 40);
                ck_sparks(G, R->x, CK_POND_TOP - 10, 8, c->col);
                pond_fish(G, f);
                f->x = CK_W - 20; f->scare = 60;
                R->fish = -1;
            }
            break;
        }
        default: break;
        }
        // the bobber sits over the hook; with a fish on, it trails behind towards the rod, so the
        // line under it runs on a slant down to the fish
        if (R->state == ROD_REEL) R->bob += (R->x + (tx - R->x) * 0.3f - R->bob) * 0.05f;
        else R->bob = R->x;
        switch (R->state) {
        case ROD_GOT:
        case ROD_SNAP:
            if (--R->t <= 0 || (R->t < 110 && HIT(G, s, CK_A))) R->state = ROD_IDLE;
            break;
        }
    }
    if (Pd->levelT > 0) Pd->levelT--;
    // the fish swim about
    for (int k = 0; k < CK_POND_FISH; k++) {
        ck_pfish *f = &Pd->fish[k];
        if (f->scare > 0) f->scare--;
        if (fish_taken(Pd, k) || f->kind == CATCH_BOOT) continue;
        float sp = ck_catches[f->kind].speed * (f->scare > 0 ? 1.6f : 0.5f);
        f->vx = (f->vx < 0 ? -1 : 1) * sp;
        f->x += f->vx;
        f->y += sinf(G->t * 1.3f + k) * 0.2f;
        if (f->x < CK_POND_BANK + 20) { f->x = CK_POND_BANK + 20; f->vx = sp; }
        if (f->x > CK_W - 10) { f->x = CK_W - 10; f->vx = -sp; }
        if (ck_rand(G) < 0.004f) f->vx = -f->vx;
        f->y = ck_clampf(f->y, CK_POND_TOP + 12, CK_POND_BED - 8);
    }
}

// ---------------------------------------------------------------------------- Bolt Ball
static void bolt_ready(ck_boltball *B) {
    memset(B, 0, sizeof *B);
    B->state = BOLT_READY;
    B->who[0] = B->who[1] = -1;
    for (int s = 0; s < 2; s++) { B->py[s] = (CK_BOLT_TOP + CK_BOLT_BOT) / 2.0f; B->ph[s] = CK_BOLT_PH; }
    B->bx = CK_W / 2.0f; B->by = (CK_BOLT_TOP + CK_BOLT_BOT) / 2.0f;
    B->last = -1; B->win = -1;
}
void ck_bolt_start(ck_game *G) { ck_house_game_heroes(G); bolt_ready(&G->house.bolt); ck_go(G, SC_BOLT); }
static void bolt_serve(ck_boltball *B, int to) {
    B->state = BOLT_SERVE; B->stateT = 0;
    B->serveTo = to; B->rally = 0; B->last = -1;
    B->bx = CK_W / 2.0f; B->by = (CK_BOLT_TOP + CK_BOLT_BOT) / 2.0f;
    B->vx = B->vy = 0;
}
// start the match: who[] says which player has which side
static void bolt_begin(ck_game *G, int left, int right) {
    ck_boltball *B = &G->house.bolt;
    B->who[0] = left; B->who[1] = right;
    B->score[0] = B->score[1] = 0;
    ck_sfx(G, SFX_CONFIRM);
    ck_play(G, SONG_BOLTBALL);
    bolt_serve(B, ck_rand(G) < 0.5f ? 0 : 1);
}
// the paddle on side s: the face the ball bounces off, and which way it faces
static float bolt_face(int s) { return s ? CK_W - CK_BOLT_PX : CK_BOLT_PX; }
void ck_bolt_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_boltball *B = &H->bolt;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { ck_house_back(G); return; }
    B->stateT++;
    if (B->state == BOLT_READY) {
        for (int i = 0; i < H->np; i++)
            if (HIT(G, H->gs[i].slot, CK_A) && !B->ready[i]) { B->ready[i] = true; ck_sfx(G, SFX_SELECT); if (!B->readyT) B->readyT = CK_BOLT_WAIT; }
        if (H->np == 1) { if (B->ready[0]) bolt_begin(G, 0, -1); }
        else if (B->ready[0] && B->ready[1]) bolt_begin(G, 0, 1);
        else if (B->readyT > 0 && --B->readyT == 0) bolt_begin(G, B->ready[0] ? 0 : -1, B->ready[1] ? 1 : -1);   // nobody else came: play the computer
        return;
    }
    if (B->state == BOLT_WON) {
        if (B->stateT > 60 && ANYHIT(G, CK_A)) { bolt_ready(B); ck_sfx(G, SFX_CONFIRM); ck_play(G, SONG_BOLTBALL); }
        return;
    }
    // the paddles: players steer, the computer follows the ball when it is coming its way
    for (int s = 0; s < 2; s++) {
        if (B->big[s] > 0 && --B->big[s] == 0) ck_sfx(G, SFX_SHRINK);
        B->ph[s] += ((B->big[s] > 0 ? CK_BOLT_BIG : CK_BOLT_PH) - B->ph[s]) * 0.15f;
        float v;
        if (B->who[s] >= 0) v = dir_y(G, H->gs[B->who[s]].slot) * 3.4f;
        else {
            bool coming = B->state == BOLT_PLAY && (s ? B->vx > 0 : B->vx < 0);
            float want = coming ? B->by + B->aim : (CK_BOLT_TOP + CK_BOLT_BOT) / 2.0f;
            float top = ck_minf(2.1f + B->rally * 0.06f, 3.0f);
            v = ck_clampf((want - B->py[s]) * 0.2f, -top, top);
        }
        B->py[s] = ck_clampf(B->py[s] + v, CK_BOLT_TOP + B->ph[s] / 2, CK_BOLT_BOT - B->ph[s] / 2);
    }
    if (B->state == BOLT_SERVE) {
        if (B->stateT < 60) return;
        float a = (ck_rand(G) - 0.5f) * 1.0f;
        B->vx = (B->serveTo ? 1 : -1) * 2.6f * cosf(a); B->vy = 2.6f * sinf(a);
        B->aim = 0;
        B->state = BOLT_PLAY; B->stateT = 0;
        ck_sfx(G, SFX_BLOW);
    }
    // the bolt: it floats in the middle for a while once a rally gets going
    if (B->bolt) { if (--B->boltT <= 0) B->bolt = false; }
    else if (B->rally >= 3 && ck_rand(G) < 0.004f) {
        B->bolt = true; B->boltT = 600;
        B->boltX = 120 + ck_rand(G) * 80; B->boltY = CK_BOLT_TOP + 30 + ck_rand(G) * (CK_BOLT_BOT - CK_BOLT_TOP - 60);
    }
    for (int sub = 0; sub < 2; sub++) {   // two half steps so a fast ball can't skip a paddle
        B->bx += B->vx * 0.5f; B->by += B->vy * 0.5f;
        if (B->by < CK_BOLT_TOP + CK_BOLT_R) { B->by = CK_BOLT_TOP + CK_BOLT_R; B->vy = fabsf(B->vy); ck_sfx(G, SFX_TICK); }
        if (B->by > CK_BOLT_BOT - CK_BOLT_R) { B->by = CK_BOLT_BOT - CK_BOLT_R; B->vy = -fabsf(B->vy); ck_sfx(G, SFX_TICK); }
        for (int s = 0; s < 2; s++) {
            float face = bolt_face(s), d = s ? B->bx + CK_BOLT_R - face : face - (B->bx - CK_BOLT_R);
            if ((s ? B->vx <= 0 : B->vx >= 0) || d < 0 || d > CK_BOLT_PW + 4) continue;
            float off = (B->by - B->py[s]) / (B->ph[s] / 2 + CK_BOLT_R);
            if (fabsf(off) > 1) continue;
            // where it hits the paddle sets the angle; every hit is a little quicker
            float sp = ck_minf(sqrtf(B->vx * B->vx + B->vy * B->vy) + 0.2f, 6.5f), a = off * 1.0f;
            B->vx = (s ? -1 : 1) * sp * cosf(a); B->vy = sp * sinf(a);
            B->bx = s ? face - CK_BOLT_R : face + CK_BOLT_R;
            B->rally++; B->last = s;
            B->aim = (ck_rand(G) - 0.5f) * B->ph[!s] * 0.9f;   // the computer won't be perfect
            ck_sfx(G, SFX_BOING);
            ck_sparks(G, B->bx, B->by, 4, 0xFFD23F);
        }
        if (B->bolt && B->last >= 0 && fabsf(B->bx - B->boltX) < 10 && fabsf(B->by - B->boltY) < 12) {
            B->bolt = false; B->big[B->last] = 600;
            ck_sfx(G, SFX_POWER);
            ck_sparks(G, B->boltX, B->boltY, 10, 0x5AD1FF);
        }
    }
    if (B->bx < -10 || B->bx > CK_W + 10) {   // missed: the other side scores
        int s = B->bx < 0 ? 1 : 0;
        B->score[s]++;
        if (B->score[s] >= CK_BOLT_WIN) {
            B->state = BOLT_WON; B->stateT = 0; B->win = s;
            ck_sfx(G, SFX_FLAG); ck_confetti(G, 160, 40, 40); ck_play(G, SONG_VICTORY);
            return;
        }
        ck_sfx(G, SFX_POP);
        bolt_serve(B, !s);   // serve to whoever just lost the point
    }
}
