// The back garden's sandbox (SC_SAND): a falling-sand toy. Mirrors sandScene in web/sand.js.
//
// The tank is a grid of cells, stepped once a tick from the bottom row up (each row left to right
// one tick, right to left the next, so nothing drifts one way). Sand falls and slides off piles,
// sinking through water and acid; water and acid fall, slide and run sideways until they lie flat;
// acid bites the sand, clay and water it touches and is sometimes used up doing it; clay never
// moves. Gravity leans a little left and right over CK_SAND_SWAY frames: falling things drift with
// it, piles slide the way it leans, and water runs downhill and heaps up against the low side, so a
// pond sloshes back and forth. Kept cheap for the N64: one byte a cell, no floats in the loop, and
// a cell boxed in on every side is passed over after a few reads.
#include <math.h>
#include <string.h>

#include "ck_internal.h"

const char *const ck_sand_tool_names[CK_SAND_TOOLS] = { "Sand", "Water", "Acid", "Clay", "Rubber", "Sway", "Wipe" };

#define SW CK_SAND_W
#define SH CK_SAND_H
#define SPREAD 4          // how far water runs sideways in one tick
#define WALL SAND_CLAY    // outside the tank is as solid as clay

static inline uint32_t srng(ck_sandbox *S) {
    uint32_t x = S->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return S->rng = x;
}
static inline bool chance(ck_sandbox *S, int p256) { return (int)(srng(S) & 255) < p256; }
static inline int mat(const ck_sandbox *S, int x, int y) {
    if (x < 0 || x >= SW || y < 0 || y >= SH) return WALL;
    return S->cell[y * SW + x] & SAND_MAT;
}
static inline bool liquid(int m) { return m == SAND_WATER || m == SAND_ACID; }

uint32_t ck_sand_hex(int c) {
    static const uint32_t base[SAND_KINDS] = { 0x232A3D, 0xE8C27A, 0x3F8FE8, 0x8CF04A, 0xB8653F };
    static const int spread[SAND_KINDS] = { 0, 2, 1, 1, 2 };
    int m = c & SAND_MAT;
    if (m >= SAND_KINDS) m = SAND_EMPTY;
    uint32_t h = base[m];
    if (m == SAND_EMPTY) return h;
    int k = 100 + (SAND_SHADE(c) - 8) * spread[m] * 2;   // a shade either side of the base colour
    int r = (int)(h >> 16 & 255) * k / 100, g = (int)(h >> 8 & 255) * k / 100, b = (int)(h & 255) * k / 100;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

int ck_sand_radius(int size) { return size <= 0 ? 1 : size == 1 ? 3 : 6; }

void ck_sand_clear(ck_sandbox *S) { memset(S->cell, 0, sizeof S->cell); }

void ck_sand_pour(ck_sandbox *S, int cx, int cy, int tool, int size) {
    if (!S->rng) S->rng = 0x2545F491u;
    int r = ck_sand_radius(size);
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            int x = cx + dx, y = cy + dy;
            if (dx * dx + dy * dy > r * r + r || x < 0 || y < 0 || x >= SW || y >= SH) continue;
            uint8_t *c = &S->cell[y * SW + x];
            if (tool == SANDTOOL_ERASE) { *c = 0; continue; }
            if (tool > SANDTOOL_CLAY || (*c & SAND_MAT)) continue;
            // clay goes on solid; the rest pours as a spray so it falls apart as it lands
            if (tool != SANDTOOL_CLAY && !chance(S, 64)) continue;
            *c = (uint8_t)((tool + 1) | (srng(S) & 0xF0));
        }
}

// Move the cell at (x, y) to (nx, ny), which is empty. A cell landing where this tick's sweep has
// not been yet (the row above, or further along this row) is flagged so it is not moved twice.
static inline void move(ck_sandbox *S, int x, int y, int nx, int ny, bool ahead) {
    uint8_t c = S->cell[y * SW + x];
    S->cell[ny * SW + nx] = (uint8_t)(ahead ? c | SAND_MOVED : c);
    S->cell[y * SW + x] = 0;
}
// Sand at (x, y) sinks into the liquid at (nx, ny) below; the liquid comes up in its place.
static inline void swap(ck_sandbox *S, int x, int y, int nx, int ny) {
    uint8_t c = S->cell[y * SW + x];
    S->cell[y * SW + x] = S->cell[ny * SW + nx];
    S->cell[ny * SW + nx] = c;
}

static void sand_cell(ck_sandbox *S, int x, int y, int side, int lean) {
    int below = mat(S, x, y + 1);
    if (below == SAND_EMPTY) {
        // falling: it drifts the way gravity leans
        if (lean && chance(S, lean) && mat(S, x + side, y + 1) == SAND_EMPTY) move(S, x, y, x + side, y + 1, false);
        else move(S, x, y, x, y + 1, false);
        return;
    }
    int a = chance(S, 128 + lean) ? side : -side;   // the downhill side first
    int ma = mat(S, x + a, y + 1), mb = mat(S, x - a, y + 1);
    if (ma == SAND_EMPTY) { move(S, x, y, x + a, y + 1, false); return; }
    if (mb == SAND_EMPTY) { move(S, x, y, x - a, y + 1, false); return; }
    // sink through water and acid, slower than through air
    if (liquid(below)) { if (chance(S, 120)) swap(S, x, y, x, y + 1); return; }
    if (liquid(ma)) { if (chance(S, 60)) swap(S, x, y, x + a, y + 1); return; }
    if (liquid(mb)) { if (chance(S, 60)) swap(S, x, y, x - a, y + 1); return; }
    // a steep lean tips a pile's top grains over sideways (a gentle one leaves piles be)
    if (lean > 80 && mat(S, x, y - 1) == SAND_EMPTY && mat(S, x + side, y) == SAND_EMPTY && mat(S, x + side, y + 1) != SAND_EMPTY && chance(S, (lean - 80) * 2))
        move(S, x, y, x + side, y, side > 0 ? S->tick & 1 : !(S->tick & 1));
}

// Acid looks at one neighbour a tick, and bites it if it is sand, clay or water. Returns true if
// that used the acid up.
static bool acid_bite(ck_sandbox *S, int x, int y) {
    static const signed char DX[4] = { 0, -1, 1, 0 }, DY[4] = { 1, 0, 0, -1 };
    int k = (int)(srng(S) >> 8 & 3), nx = x + DX[k], ny = y + DY[k];
    int n = mat(S, nx, ny);
    if (nx < 0 || ny < 0 || nx >= SW || ny >= SH) return false;
    if (n != SAND_SAND && n != SAND_CLAY && n != SAND_WATER) return false;
    if (!chance(S, n == SAND_CLAY ? 20 : 60)) return false;
    S->cell[ny * SW + nx] = 0;
    S->fizz++; S->fx = nx; S->fy = ny;
    if (chance(S, 128)) { S->cell[y * SW + x] = 0; return true; }
    return false;
}

static void liquid_cell(ck_sandbox *S, int x, int y, int m, int side, int lean, bool ltr) {
    if (m == SAND_ACID && acid_bite(S, x, y)) return;
    if (mat(S, x, y + 1) == SAND_EMPTY) {
        if (lean && chance(S, lean) && mat(S, x + side, y + 1) == SAND_EMPTY) move(S, x, y, x + side, y + 1, false);
        else move(S, x, y, x, y + 1, false);
        return;
    }
    // deep in a pond, boxed in on every side but the top: the quick way out (most cells)
    if (mat(S, x - 1, y + 1) && mat(S, x + 1, y + 1) && mat(S, x - 1, y) && mat(S, x + 1, y)) {
        if (lean && mat(S, x, y - 1) == SAND_EMPTY && mat(S, x + side, y - 1) == SAND_EMPTY && chance(S, lean * 3 / 2))
            move(S, x, y, x + side, y - 1, true);
        return;
    }
    int a = chance(S, 128 + lean) ? side : -side;
    if (mat(S, x + a, y + 1) == SAND_EMPTY) { move(S, x, y, x + a, y + 1, false); return; }
    if (mat(S, x - a, y + 1) == SAND_EMPTY) { move(S, x, y, x - a, y + 1, false); return; }
    // run sideways along the top of whatever is below; uphill is harder the more gravity leans
    for (int pass = 0; pass < 2; pass++, a = -a) {
        if (a != side && lean && !chance(S, 256 - lean * 2)) continue;
        int nx = x;
        for (int k = 1; k <= SPREAD; k++) {
            int tx = x + a * k;
            if (mat(S, tx, y) != SAND_EMPTY) break;
            nx = tx;
            if (mat(S, tx, y + 1) == SAND_EMPTY) break;   // over an edge: it falls from here next tick
        }
        if (nx != x) { move(S, x, y, nx, y, ltr ? nx > x : nx < x); return; }
    }
    // the top of a pond heaps up against the low side: climb onto the water beside, downhill
    if (lean && mat(S, x, y - 1) == SAND_EMPTY && mat(S, x + side, y - 1) == SAND_EMPTY && mat(S, x + side, y) != SAND_EMPTY && chance(S, lean * 3 / 2))
        move(S, x, y, x + side, y - 1, true);
}

void ck_sand_step(ck_sandbox *S) {
    if (!S->rng) S->rng = 0x2545F491u;
    S->tick++;
    S->fizz = 0;
    if (!S->still) S->swayT = (S->swayT + 1) % CK_SAND_SWAY;
    float want = S->still ? 0 : CK_SAND_LEAN * sinf(S->swayT * (2 * CK_PI / CK_SAND_SWAY));
    S->lean += (want - S->lean) * 0.05f;
    int side = S->lean < 0 ? -1 : 1;
    int lean = (int)(fabsf(S->lean) * 256);   // 0 .. about 107: the chance out of 256 of going with it
    bool ltr = S->tick & 1;
    for (int y = SH - 1; y >= 0; y--) {
        uint8_t *row = &S->cell[y * SW];
        for (int i = 0; i < SW; i++) {
            int x = ltr ? i : SW - 1 - i;
            uint8_t c = row[x];
            int m = c & SAND_MAT;
            if (m == SAND_EMPTY || m == SAND_CLAY) continue;
            if (c & SAND_MOVED) { row[x] = (uint8_t)(c & ~SAND_MOVED); continue; }   // it already moved this tick
            if (m == SAND_SAND) sand_cell(S, x, y, side, lean);
            else liquid_cell(S, x, y, m, side, lean, ltr);
        }
    }
}

// ---------------------------------------------------------------------------- the game
// A cursor each: the stick moves it (faster the longer it is held), A pours what it holds (or,
// on Sway and Wipe, turns the sway off and on or empties the tank), B rubs out, L and R change
// what it holds, C the brush size. START goes back into the house. The tank keeps what is in it
// until the console is switched off.
static int sdir(const ck_game *G, int s, int neg, int pos) { return (HELD(G, s, pos) ? 1 : 0) - (HELD(G, s, neg) ? 1 : 0); }

void ck_sand_start(ck_game *G) {
    ck_house *H = &G->house;
    ck_sandbox *S = &H->sand;
    ck_house_game_heroes(G);
    for (int i = 0; i < 2; i++) {
        ck_sandcur *c = &S->cur[i];
        c->x = CK_SAND_X + CK_SAND_W * CK_SAND_CELL * (i ? 0.7f : 0.3f);
        c->y = CK_SAND_Y + 30;
        c->down = false; c->armed = false; c->rep = 0;
        if (!S->ready || c->tool < 0 || c->tool >= CK_SAND_TOOLS || c->tool == SANDTOOL_CLEAR) c->tool = i ? SANDTOOL_WATER : SANDTOOL_SAND;
        if (!S->ready || c->size < 0 || c->size >= CK_SAND_SIZES) c->size = 1;
    }
    S->ready = true;
    ck_go(G, SC_SAND);
}

// a stroke from (x0, y0) to (x1, y1) on screen, a dab every cell or so
static void sand_stroke(ck_sandbox *S, float x0, float y0, float x1, float y1, int tool, int size) {
    int n = (int)ceilf(hypotf(x1 - x0, y1 - y0) / CK_SAND_CELL);
    if (n < 1) n = 1;
    for (int k = 1; k <= n; k++) {
        float x = x0 + (x1 - x0) * k / n, y = y0 + (y1 - y0) * k / n;
        ck_sand_pour(S, (int)floorf((x - CK_SAND_X) / CK_SAND_CELL), (int)floorf((y - CK_SAND_Y) / CK_SAND_CELL), tool, size);
    }
}

void ck_sand_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_sandbox *S = &H->sand;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { ck_house_back(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_sandcur *c = &S->cur[i];
        int s = H->gs[i].slot, dx = sdir(G, s, CK_LEFT, CK_RIGHT), dy = sdir(G, s, CK_UP, CK_DOWN);
        c->rep = dx || dy ? c->rep + 1 : 0;
        float sp = c->rep < 8 ? 1.0f : c->rep < 30 ? 2.0f : 3.2f;   // a tap nudges, holding speeds up
        if (dx && dy) sp *= 0.75f;
        c->x = ck_clampf(c->x + dx * sp, CK_SAND_X, CK_SAND_X + CK_SAND_W * CK_SAND_CELL - 1);
        c->y = ck_clampf(c->y + dy * sp, CK_SAND_Y, CK_SAND_Y + CK_SAND_H * CK_SAND_CELL - 1);
        if (HIT(G, s, CK_R)) { c->tool = (c->tool + 1) % CK_SAND_TOOLS; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_L)) { c->tool = (c->tool + CK_SAND_TOOLS - 1) % CK_SAND_TOOLS; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_C)) { c->size = (c->size + 1) % CK_SAND_SIZES; ck_sfx(G, SFX_TICK); }
        if (!HELD(G, s, CK_A) && !HELD(G, s, CK_B)) c->armed = true;   // the A that opened the sandbox pours nothing
        if (!c->armed) continue;
        int tool = HELD(G, s, CK_B) ? SANDTOOL_ERASE : c->tool;
        if (tool == SANDTOOL_TILT || tool == SANDTOOL_CLEAR) {
            c->down = false;
            if (!HIT(G, s, CK_A)) continue;
            if (tool == SANDTOOL_TILT) { S->still = !S->still; ck_sfx(G, S->still ? SFX_SHRINK : SFX_POWER); }
            else {
                ck_sand_clear(S);
                ck_sfx(G, SFX_WHOOSH);
                for (int k = 0; k < 12; k++) ck_puff(G, 20 + k * 25, CK_SAND_Y + 40 + (k % 3) * 60, 2, 0xFFFFFF, 220);
            }
            continue;
        }
        if (!HELD(G, s, CK_A) && !HELD(G, s, CK_B)) { c->down = false; continue; }
        if (!c->down) { c->lx = c->x; c->ly = c->y; c->down = true; }
        sand_stroke(S, c->lx, c->ly, c->x, c->y, tool, c->size);
        c->lx = c->x; c->ly = c->y;
    }
    ck_sand_step(S);
    if (S->fizz && S->tick % 8 == 0) {   // acid fizzing: a little green puff where it last bit
        if (S->tick % 32 == 0) ck_sfx(G, SFX_BLOW);
        ck_puff(G, CK_SAND_X + S->fx * CK_SAND_CELL, CK_SAND_Y + S->fy * CK_SAND_CELL - 2, 1, 0xB8FF7A, 200);
    }
}
