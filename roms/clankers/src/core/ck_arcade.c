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
// The table: walls round the top, a launch lane down the right, two guides into the flippers.
const ck_seg ck_pin_walls[CK_PIN_WALLS] = {
    { 70, 168, 70, 44 }, { 70, 44, 98, 16 }, { 98, 16, 222, 16 }, { 222, 16, 250, 44 },
    { 250, 44, 250, 232 }, { 232, 232, 250, 232 }, { 232, 232, 232, 76 },
    { 70, 168, 128, 206 }, { 232, 168, 192, 206 },
};
const ck_bump ck_pin_bumps[CK_PIN_BUMPS] = {
    { 124, 72, 11, 100 }, { 196, 72, 11, 100 }, { 160, 104, 11, 100 }, { 96, 136, 6, 50 }, { 206, 136, 6, 50 },
};
const ck_seg ck_pin_gate = { 250, 58, 232, 76 };
const float ck_pin_lights[CK_PIN_LIGHTS][2] = { { 122, 32 }, { 160, 32 }, { 198, 32 } };
const float ck_pin_pivot[2][2] = { { 128, 206 }, { 192, 206 } };
#define FLIP_REST 0.5f
#define FLIP_UP (-0.45f)
#define LANE_X 241
#define LANE_Y 227

void ck_pin_flipper(int side, float a, float *x0, float *y0, float *x1, float *y1) {
    *x0 = ck_pin_pivot[side][0]; *y0 = ck_pin_pivot[side][1];
    *x1 = *x0 + (side ? -1 : 1) * cosf(a) * CK_PIN_FLIP_LEN;
    *y1 = *y0 + sinf(a) * CK_PIN_FLIP_LEN;
}
static void pin_new_ball(ck_pinball *P) { P->x = LANE_X; P->y = LANE_Y; P->vx = P->vy = 0; }
static void pin_reset(ck_pinball *P) {
    int best = P->best;
    memset(P, 0, sizeof *P);
    P->best = best;
    P->balls = CK_PIN_BALLS;
    P->flip[0] = P->flip[1] = FLIP_REST;
    pin_new_ball(P);
}
void ck_pin_start(ck_game *G) { ck_house_game_heroes(G); pin_reset(&G->house.pin); ck_go(G, SC_PINBALL); }

// Push the ball out of a fat line from a to b and bounce it. A flipper's line turns about its
// pivot at w radians a frame (side picks the way it turns), which flicks the ball.
static bool pin_seg(ck_pinball *P, float ax, float ay, float bx, float by, float rr, float e, int side, float w) {
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

void ck_pin_update(ck_game *G) {
    ck_pinball *P = &G->house.pin;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { ck_house_back(G); return; }
    if (P->over) {
        if (++P->overT > 60 && ANYHIT(G, CK_A)) { pin_reset(P); ck_sfx(G, SFX_CONFIRM); }
        return;
    }
    // the flippers
    bool hold[2] = { any_held(G, CK_LEFT | CK_B | CK_L), any_held(G, CK_RIGHT | CK_A | CK_R) };
    float was[2];
    for (int s = 0; s < 2; s++) {
        was[s] = P->flip[s];
        if (hold[s]) P->flip[s] = ck_maxf(FLIP_UP, P->flip[s] - 0.32f);
        else P->flip[s] = ck_minf(FLIP_REST, P->flip[s] + 0.18f);
        if (hold[s] && was[s] == FLIP_REST) ck_sfx(G, SFX_CLANK);
        P->flipW[s] = P->flip[s] - was[s];
    }
    // the launch lane: A fires a waiting ball up the lane
    if (P->x > 232 && P->y > 218 && fabsf(P->vy) < 1 && ANYHIT(G, CK_A)) {
        P->vy = -8.6f - ck_rand(G) * 0.4f;
        ck_sfx(G, SFX_SPRING);
    }
    for (int sub = 0; sub < 4; sub++) {
        P->vy += 0.14f / 4;
        P->x += P->vx / 4; P->y += P->vy / 4;
        for (int k = 0; k < CK_PIN_WALLS; k++) {
            const ck_seg *w = &ck_pin_walls[k];
            pin_seg(P, w->x0, w->y0, w->x1, w->y1, 1, 0.5f, 0, 0);
        }
        // the lane's gate only lets the ball out: falling onto it, it rolls off onto the table
        if (P->vy > 0) pin_seg(P, ck_pin_gate.x0, ck_pin_gate.y0, ck_pin_gate.x1, ck_pin_gate.y1, 1, 0.4f, 0, 0);
        for (int s = 0; s < 2; s++) {
            float a = was[s] + P->flipW[s] * (sub + 1) / 4, x0, y0, x1, y1;
            ck_pin_flipper(s, a, &x0, &y0, &x1, &y1);
            pin_seg(P, x0, y0, x1, y1, 3, 0.3f, s, P->flipW[s]);
        }
        for (int k = 0; k < CK_PIN_BUMPS; k++) {
            const ck_bump *b = &ck_pin_bumps[k];
            float nx = P->x - b->x, ny = P->y - b->y, d = sqrtf(nx * nx + ny * ny);
            if (d >= b->r + CK_PIN_R || d < 0.0001f) continue;
            nx /= d; ny /= d;
            P->x = b->x + nx * (b->r + CK_PIN_R); P->y = b->y + ny * (b->r + CK_PIN_R);
            float vn = P->vx * nx + P->vy * ny;
            if (vn < 0) { P->vx -= 2 * vn * nx; P->vy -= 2 * vn * ny; }
            float kick = b->score >= 100 ? 2.6f : 1.8f;   // bumpers kick the ball away
            P->vx += nx * kick; P->vy += ny * kick;
            if (P->flash[k] < 8) { P->score += b->score; ck_sfx(G, b->score >= 100 ? SFX_BOING : SFX_HOP); ck_sparks(G, b->x + nx * b->r, b->y + ny * b->r, 4, 0xFFD23F); }
            P->flash[k] = 12;
        }
        float sp = sqrtf(P->vx * P->vx + P->vy * P->vy);
        if (sp > 9) { P->vx *= 9 / sp; P->vy *= 9 / sp; }
    }
    for (int k = 0; k < CK_PIN_BUMPS; k++) if (P->flash[k] > 0) P->flash[k]--;
    // the lights along the top: light all three for a bonus
    for (int k = 0; k < CK_PIN_LIGHTS; k++) {
        if (P->lit & (1 << k) || hypotf(P->x - ck_pin_lights[k][0], P->y - ck_pin_lights[k][1]) > 8) continue;
        P->lit |= 1 << k; P->score += 250; ck_sfx(G, SFX_STAR1);
        if (P->lit == (1 << CK_PIN_LIGHTS) - 1) { P->score += 1000; P->lit = 0; P->litT = 90; ck_sfx(G, SFX_FLAG); ck_confetti(G, 160, 30, 30); }
    }
    if (P->litT > 0) P->litT--;
    // down the middle: that ball is lost
    if (P->y > 250) {
        ck_sfx(G, SFX_OUCH);
        if (--P->balls <= 0) { P->over = true; P->overT = 0; if (P->score > P->best) P->best = P->score; }
        else pin_new_ball(P);
    }
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
        ck_sfx(G, B->frame ? SFX_STEP : SFX_TICK);
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
    { "Minnow", 0xA9C4D6, 7, 1.2f, 0.6f, 1 },
    { "Goldfish", 0xFF8A3D, 9, 0.9f, 0.9f, 2 },
    { "Big bass", 0x5E8C4A, 15, 0.7f, 1.5f, 3 },
    { "Golden fish", 0xFFD23F, 11, 1.6f, 1.8f, 10 },
    { "Old boot", 0x6F4630, 10, 0, 0.2f, 0 },
};
void ck_rod_tip(int i, float *x, float *y) { *x = 46 + i * 14; *y = 62 + i * 6; }
static void pond_fish(ck_game *G, ck_pfish *f) {
    float r = ck_rand(G);
    f->kind = r < 0.35f ? CATCH_MINNOW : r < 0.65f ? CATCH_GOLDFISH : r < 0.85f ? CATCH_BASS : r < 0.95f ? CATCH_BOOT : CATCH_GOLDEN;
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
                    else { R->state = ROD_BITE; R->t = 45; ck_sfx(G, SFX_BUTTON); }
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
        case ROD_GOT:
        case ROD_SNAP:
            if (--R->t <= 0 || (R->t < 110 && HIT(G, s, CK_A))) R->state = ROD_IDLE;
            break;
        }
    }
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
