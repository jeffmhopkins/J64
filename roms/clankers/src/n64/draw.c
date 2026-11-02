// Drawing the game on the N64 with rdpq. Everything here reads the core's state (ck_game) and
// draws it the way web/game.js does, using the sprite strips baked from the browser's drawing
// code (tools/bake_sprites.mjs) and Titan One for text.
#include <libdragon.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "ck.h"
#include "draw.h"
#include "sprites.h"

#define W  CK_W
#define H  CK_H
#define TS CK_TS
#define PI 3.14159265f

// ---------------------------------------------------------------------------- resources

static sprite_t *spr[SPR_COUNT];
static int big_group = -1;
enum { F_SMALL = 1, F_MED, F_BIG, F_HUGE };
enum { ST_CREAM, ST_YELLOW, ST_DARK, ST_RED, ST_BLUE, ST_PALE, ST_PLAIN, ST_GREEN, ST_WHITE, ST_GREY, ST_COUNT };

static sprite_t *sp(int id) {
    if (!spr[id]) spr[id] = sprite_load(SPR_INFO[id].file);
    return spr[id];
}
static bool waited;
static void drop(int id) {
    if (!spr[id]) return;
    if (!waited) { rspq_wait(); waited = true; }   // the RDP may still be drawing with it
    sprite_free(spr[id]);
    spr[id] = NULL;
}

static bool is_big(int id) {
    static const int big[] = { SPR_TITLE, SPR_SELECT_BG, SPR_MAP_W1, SPR_BG_FAR, SPR_BG_FAR_BOSS, SPR_BG_MID,
                               SPR_JET_SKY, SPR_JET_HILLS, SPR_CRANE, SPR_INTRO_NIGHT, SPR_INTRO_STORM, SPR_INTRO_DAWN };
    for (unsigned i = 0; i < sizeof big / sizeof *big; i++) if (big[i] == id) return true;
    return false;
}
static int hero_sprite(int hero, int col) { return (hero == HERO_ROBOT ? SPR_HERO_ROBOT_0 : SPR_HERO_OSTRICH_0) + (col & 7); }

static color_t hexc(uint32_t hex, int a) { return RGBA32((hex >> 16) & 255, (hex >> 8) & 255, hex & 255, a); }

void draw_init(void) {
    static const struct { uint32_t col, out; int outA; } styles[ST_COUNT] = {
        { 0xFFF6D8, 0x1B1426, 255 }, { 0xFFD23F, 0x1B1426, 255 }, { 0x2B2230, 0, 0 }, { 0xFF8A8A, 0x1B1426, 255 },
        { 0x8FD3FF, 0x1B1426, 255 }, { 0xD9D2E6, 0, 0 }, { 0xFFF6D8, 0, 0 }, { 0x49D17A, 0x1B1426, 255 },
        { 0xFFFFFF, 0, 0 }, { 0xA49DB4, 0, 0 },
    };
    static const char *files[4] = { "rom:/titan8.font64", "rom:/titan10.font64", "rom:/titan14.font64", "rom:/titan18.font64" };
    for (int f = 0; f < 4; f++) {
        rdpq_font_t *font = rdpq_font_load(files[f]);
        for (int s = 0; s < ST_COUNT; s++)
            rdpq_font_style(font, s, &(rdpq_fontstyle_t){ .color = hexc(styles[s].col, 255), .outline_color = hexc(styles[s].out, styles[s].outA) });
        rdpq_text_register_font(F_SMALL + f, font);
    }
}

// Free the big pictures and hero sheets the next scene does not need.
static void free_for(const ck_game *G) {
    int group = G->scene == SC_INTRO ? 4 : G->scene == SC_TITLE ? 0 : G->scene == SC_SELECT ? 1 : (G->scene == SC_MAP || G->scene == SC_SHOP) ? 2 : 3;
    waited = false;
    if (group != big_group) {
        big_group = group;
        for (int id = 0; id < SPR_COUNT; id++) if (is_big(id)) drop(id);
    }
    // only the hero sheets someone has picked stay loaded (each is about 230 KB)
    for (int c = 0; c < 8; c++)
        for (int h = 0; h < 2; h++) {
            int id = hero_sprite(h, c);
            bool keep = false;
            for (int s = 0; s < 2; s++) if (G->prog.joined[s] && G->prog.hero[s] == h && G->prog.col[s] == c) keep = true;
            if (!keep) drop(id);
        }
}

// ---------------------------------------------------------------------------- drawing helpers

enum { M_NONE, M_TEX, M_FLAT };
static int mode = M_NONE;
static float shakeY;   // world drawing offset while the screen shakes

static void mode_tex(void) {
    if (mode == M_TEX) return;
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER1((TEX0, 0, PRIM, 0), (TEX0, 0, PRIM, 0)));
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    mode = M_TEX;
}
static void mode_flat(void) {
    if (mode == M_FLAT) return;
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    mode = M_FLAT;
}

typedef struct { bool flip; float sx, sy, rot; color_t tint; bool tinted; } bopt;

static void blit_o(int id, int frame, float x, float y, bopt o) {
    const spr_info *I = &SPR_INFO[id];
    if (frame < 0 || frame >= I->n) frame = 0;
    mode_tex();
    rdpq_set_prim_color(o.tinted ? o.tint : RGBA32(255, 255, 255, 255));
    rdpq_sprite_blit(sp(id), x, y, &(rdpq_blitparms_t){
        .s0 = (frame % I->cols) * I->fw, .t0 = (frame / I->cols) * I->fh, .width = I->fw, .height = I->fh,
        .cx = I->ax, .cy = I->ay, .flip_x = o.flip, .scale_x = o.sx, .scale_y = o.sy, .theta = o.rot,
    });
}
static void blit(int id, int frame, float x, float y) { blit_o(id, frame, x, y, (bopt){ 0 }); }
// A white shape (dot, ring, star) in a colour, sized by radius.
static void shape(int id, float x, float y, float r, uint32_t hex, float alpha) {
    float base = id == SPR_DOT ? 7.5f : id == SPR_RING ? 10.5f : 9.5f;
    if (r <= 0 || alpha <= 0) return;
    if (alpha > 1) alpha = 1;
    blit_o(id, 0, x, y, (bopt){ .sx = r / base, .sy = r / base, .tinted = true, .tint = hexc(hex, (int)(alpha * 255)) });
}

static void rect(float x, float y, float w, float h, uint32_t hex, float alpha) {
    if (w <= 0 || h <= 0 || alpha <= 0) return;
    mode_flat();
    rdpq_set_prim_color(hexc(hex, (int)(alpha > 1 ? 255 : alpha * 255)));
    rdpq_fill_rectangle(x, y, x + w, y + h);
}
static void panel(float x, float y, float w, float h, uint32_t hex, float alpha) { rect(x, y, w, h, hex, alpha); }
static void dark_panel(float x, float y, float w, float h) { panel(x, y, w, h, 0x1B1426, 0.86f); }

// A straight line of width w (a stretched, rotated pixel).
static void line(float x0, float y0, float x1, float y1, float w, uint32_t hex, float alpha) {
    float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    if (fabsf(dx) < 0.01f || fabsf(dy) < 0.01f) {
        float xa = fminf(x0, x1), ya = fminf(y0, y1);
        rect(xa - (fabsf(dy) < 0.01f ? 0 : w / 2), ya - (fabsf(dx) < 0.01f ? 0 : w / 2),
             fabsf(dx) < 0.01f ? w : len, fabsf(dy) < 0.01f ? w : len, hex, alpha);
        return;
    }
    mode_tex();
    rdpq_set_prim_color(hexc(hex, (int)(alpha * 255)));
    rdpq_sprite_blit(sp(SPR_PX), x0, y0, &(rdpq_blitparms_t){ .cx = 0, .cy = 2, .scale_x = len / 4, .scale_y = w / 4, .theta = -atan2f(dy, dx) });
}

static void txt(int font, int style, float x, float y, int align, const char *fmt, ...) {
    char raw[96], buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(raw, sizeof raw, fmt, ap);
    va_end(ap);
    // rdpq_text reads $xx and ^xx as font and style changes; double them to print them as they are
    size_t n = 0;
    for (const char *c = raw; *c && n < sizeof buf - 2; c++) {
        if (*c == '$' || *c == '^') buf[n++] = *c;
        buf[n++] = *c;
    }
    buf[n] = 0;
    float x0 = align == ALIGN_CENTER ? x - 160 : align == ALIGN_RIGHT ? x - 320 : x;
    rdpq_text_print(&(rdpq_textparms_t){ .style_id = style, .width = align == ALIGN_LEFT ? 0 : 320, .align = align, .disable_aa_fix = true },
                    font, x0, y, buf);
    mode = M_NONE;
}

static const char *clock_str(int frames, char *buf) {
    int sec = frames / 60;
    sprintf(buf, "%d:%02d", sec / 60, sec % 60);
    return buf;
}

static void particles(const ck_game *G, float camX) {
    for (int i = 0; i < G->nparts; i++) {
        const ck_part *p = &G->parts[i];
        float a = (float)p->life / (float)p->max;
        if (a > 1) a = 1;
        float x = p->x - camX, y = p->y + shakeY;
        float base = p->alpha / 255.0f;
        if (p->kind == PART_STAR) shape(SPR_STAR, x, y, p->r * (0.5f + a), p->col, a);
        else if (p->kind == PART_CONF)
            blit_o(SPR_PX, 0, x, y, (bopt){ .sx = 0.75f, .sy = 0.5f, .rot = -p->rot, .tinted = true, .tint = hexc(p->col, 255) });
        else if (p->kind == PART_CHUNK) rect(x - 1.5f, y - 1.5f, 3, 3, p->col, a);
        else shape(SPR_DOT, x, y, p->r * (1.5f - a * 0.5f), p->col, a * base);
    }
}

static void battery(float x, float y, bool full) { blit(SPR_BATTERY, full ? 0 : 1, x, y); }
static void lives_hud(const ck_game *G, float x, float y) {
    dark_panel(x, y, 48, 15);
    for (int k = 0; k < 3; k++) battery(x + 3 + k * 15, y + 4, k < G->prog.lives);
}

static void baby(uint32_t hex, int frame, float x, float y, float scale) {
    char name[32];
    snprintf(name, sizeof name, "rom:/baby_%06lX.sprite", (unsigned long)hex);
    int id = SPR_BABY_FF8FC7;
    for (int i = 0; i < SPR_COUNT; i++) if (!strcmp(SPR_INFO[i].file, name)) { id = i; break; }
    blit_o(id, frame, x, y, (bopt){ .sx = scale, .sy = scale });
}

// ---------------------------------------------------------------------------- the story intro

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

// A sleepy "z" that grows and fades as it drifts up.
static void zee(float x, float y, float size, float alpha) {
    if (alpha <= 0) return;
    blit_o(SPR_ZEE, 0, x, y, (bopt){ .sx = size / 12, .sy = size / 12, .tinted = true, .tint = hexc(0xD9D2E6, (int)(clampf(alpha, 0, 1) * 255)) });
}

// introScene.draw in web/game.js. The scenery is three baked pictures (night, storm, dawn)
// cross-faded the way the browser mixes their colours.
static void draw_intro(const ck_game *G) {
    const ck_intro *I = &G->intro;
    float t = G->t;
    float dawn = clampf((t - 10.5f) / 3, 0, 1);
    float storm = clampf((t - 4.2f) / 1.2f, 0, 1) * (1 - clampf((t - 10.2f) / 1.2f, 0, 1));
    blit(SPR_INTRO_NIGHT, 0, 0, 0);
    if (storm > 0) blit_o(SPR_INTRO_STORM, 0, 0, 0, (bopt){ .sx = 1, .sy = 1, .tinted = true, .tint = RGBA32(255, 255, 255, (int)(storm * 255)) });
    if (dawn > 0) blit_o(SPR_INTRO_DAWN, 0, 0, 0, (bopt){ .sx = 1, .sy = 1, .tinted = true, .tint = RGBA32(255, 255, 255, (int)(dawn * 255)) });
    // twinkling stars, skipping the ones the moon and the garage roof would hide
    for (int i = 0; i < 40 && dawn < 1; i++) {
        float x = (float)((i * 97) % 320), y = (float)((i * 53) % 120), r = (float)((i % 3) + 1) * 0.5f;
        if ((x > 252 && x < 284 && y > 24 && y < 56) || (x > 56 && x < 184 && y > 96)) continue;
        if (sinf(t * 2 + x) > -0.3f) shape(SPR_DOT, x, y, r, 0xFFF6D8, 1 - dawn);
    }
    // baby bots: asleep, then startled, then blown away
    static const float bx[4] = { 104, 116, 128, 140 }, tx[4] = { -30, 360, 60, 300 }, ty[4] = { -40, -20, -60, -70 };
    for (int k = 0; k < 4; k++) {
        float fly = clampf((t - 8 - k * 0.15f) / 2.2f, 0, 1), shake = t > 5 && t < 8 ? sinf(t * 40 + k) * 1.2f : 0;
        if (fly >= 1) continue;
        float e = fly * fly, x = bx[k] + (tx[k] - bx[k]) * e + shake, y = 190 + (ty[k] - 190) * e - sinf(fly * PI) * 30;
        float sc = 1 - fly * 0.5f;
        blit_o(SPR_INTRO_BABY, (t < 4.8f ? 0 : 4) + k, x, y, (bopt){ .sx = sc, .sy = sc, .rot = -fly * 8 * (k % 2 ? 1 : -1) });
    }
    if (t < 4.6f)
        for (int k = 0; k < 3; k++) { float z = fmodf(t * 0.6f + k / 3.0f, 1); zee(122 + z * 14 + k * 4, 176 - z * 26, 6 + z * 6, 1 - z); }
    // the heroes: dozing beside the garage, then up and ready
    bool sleepy = t < 11.4f;
    float hop = t > 11.4f && t < 12.6f ? sinf((t - 11.4f) / 1.2f * PI) * 22 : 0;
    int half = t > 5 && t < 10.5f;
    int rf = sleepy ? half : (t > 12 && t < 12.6f) ? 4 : (sinf(t * 8) > 0 ? 3 : 2);
    int of = sleepy ? half : (fabsf(sinf(t * 12)) > 0.5f ? 3 : 2);
    blit(SPR_INTRO_ROBOT, rf, 210, 196 - hop);
    blit(SPR_INTRO_OSTRICH, of, 266, 196 - hop * 0.8f);
    if (t < 11 && t > 1)
        for (int k = 0; k < 2; k++) {
            float z = fmodf(t * 0.5f + k / 2.0f, 1);
            zee(226 + k * 46 + z * 10, 150 - z * 20, 6 + z * 5, (1 - z) * (t < 4.6f || t > 10.5f ? 1 : 0));
        }
    // the Big Glitch storm cloud
    if (storm > 0) {
        float cx = 380 - storm * 170 + sinf(t * 1.3f) * 6, cy = 44 + sinf(t * 2) * 3, a = fminf(1, storm * 1.5f);
        blit_o(SPR_INTRO_CLOUD, 0, cx, cy, (bopt){ .sx = 1, .sy = 1, .tinted = true, .tint = RGBA32(255, 255, 255, (int)(a * 255)) });
        if (t > 7.6f && t < 10.5f)
            for (int k = 0; k < 8; k++) {
                float y = 60 + k * 16, o = fmodf(t * 300 + k * 70, 380);
                line(340 - o, y, 310 - o, y + 2, 1, 0xDCD2FF, 0.5f * a);
            }
        if (I->boltLife > 0) {
            float x = I->boltX, y = cy + 20;
            for (int k = 0; k < CK_INTRO_BOLT && y < 196; k++) {
                float nx = x + I->boltDx[k], ny = y + 14;
                line(x, y, nx, ny, 2.5f, 0xFFE66B, 1);
                x = nx; y = ny;
            }
        }
    }
    if (I->flash > 0) rect(0, 0, W, H, 0xFFFFFF, I->flash * 0.6f);
    // captions (the text pops in once its panel is half faded in)
    const char *cap = t < 4.5f ? "One night in Junkyard Hills..." : t < 8 ? "...a Big Glitch storm rolled in!"
                    : t < 11 ? "Whoosh! The baby bots blew far away!" : "Time to bring them home!";
    float ct = t < 4.5f ? t : t < 8 ? t - 4.5f : t < 11 ? t - 8 : t - 11, ca = clampf(ct * 2, 0, 1);
    panel(30, 210, 260, 22, 0x1B1426, 0.86f * ca);
    if (ca > 0.5f) txt(F_MED, ST_CREAM, W / 2, 225, ALIGN_CENTER, "%s", cap);
    if (t > 14.4f) rect(0, 0, W, H, 0xFFF8E6, clampf((t - 14.4f) / 0.8f, 0, 1));
    if ((int)(t * 2) % 2 && t > 1) txt(F_SMALL, ST_PALE, W - 8, 14, ALIGN_RIGHT, "START to skip");
}

// ---------------------------------------------------------------------------- the title and select screens

static void draw_title(const ck_game *G) {
    blit(SPR_TITLE, 0, 0, 0);
    if ((int)(G->t * 1.6f) % 2 == 0) txt(F_MED, ST_CREAM, W / 2, 230, ALIGN_CENTER, "PRESS START");
}

static void hero_front(int hero, int col, int frame, float x, float y, float scale, bool flip) {
    blit_o(hero_sprite(hero, col), frame, x, y, (bopt){ .sx = scale, .sy = scale, .flip = flip });
}

static void draw_select(const ck_game *G) {
    blit(SPR_SELECT_BG, 0, 0, 0);
    txt(F_BIG, ST_CREAM, W / 2, 28, ALIGN_CENTER, "WHO'S PLAYING?");
    for (int i = 0; i < 2; i++) {
        float x = 18 + i * 152, y = 38, w = 132, h = 158;
        panel(x, y, w, h, 0x1B1426, G->prog.joined[i] ? 0.7f : 0.4f);
        txt(F_MED, i ? ST_BLUE : ST_YELLOW, x + w / 2, y + 16, ALIGN_CENTER, "PLAYER %d", i + 1);
        if (!G->prog.joined[i]) {
            txt(F_BIG, ST_CREAM, x + w / 2, y + 74, ALIGN_CENTER, "Press A");
            txt(F_BIG, ST_CREAM, x + w / 2, y + 92, ALIGN_CENTER, "to join");
            txt(F_SMALL, ST_GREY, x + w / 2, y + 112, ALIGN_CENTER, "controller 2");
            continue;
        }
        rect(x + 30, y + 118, w - 60, 8, 0xFFFFFF, 0.12f);
        bool ready = G->ready[i];
        int hero = G->prog.hero[i], col = G->prog.col[i];
        int frame = ready ? 25 + ((int)(G->t * 4) % 2) : (int)(G->t / (hero == HERO_ROBOT ? 0.52f : 0.3f)) % 4;
        hero_front(hero, col, frame, x + w / 2, y + 120, 2.6f, false);
        txt(F_MED, ST_CREAM, x + w / 2, y + 138, ALIGN_CENTER, hero == HERO_ROBOT ? "ROBOT" : "ROBOT OSTRICH");
        txt(F_SMALL, ST_PALE, x + w / 2, y + 151, ALIGN_CENTER, "< > hero   ^ v %s", ck_col_names[col]);
        if (ready) { rect(x + 30, y + 22, w - 60, 16, 0x49D17A, 1); txt(F_MED, ST_WHITE, x + w / 2, y + 34, ALIGN_CENTER, "READY!"); }
    }
    if ((int)(G->t * 2) % 2) txt(F_MED, ST_CREAM, W / 2, 220, ALIGN_CENTER, "A when ready  -  START to go");
}

// ---------------------------------------------------------------------------- the world map

static void road(const ck_road *r, float upto, bool open) {
    float cut = (upto < 0 ? 0 : upto > 1 ? 1 : upto) * r->total;
    for (int pass = 0; pass < (open ? 2 : 2); pass++) {
        float w = open ? (pass ? 7 : 10) : (pass ? 4 : 8);
        uint32_t col = open ? (pass ? 0xF2D7A6 : 0x7A4A2C) : (pass ? 0xFFF0D7 : 0x3C1E14);
        float alpha = open ? 1 : (pass ? 0.35f : 0.25f);
        for (int i = 1; i < r->npts; i++) {
            float x0 = r->pts[i - 1][0], y0 = r->pts[i - 1][1], x1 = r->pts[i][0], y1 = r->pts[i][1];
            if (r->lens[i - 1] >= cut) break;
            if (r->lens[i] > cut) { float k = (cut - r->lens[i - 1]) / (r->lens[i] - r->lens[i - 1]); x1 = x0 + (x1 - x0) * k; y1 = y0 + (y1 - y0) * k; }
            // square caps: extend by half the width
            float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
            if (l < 0.01f) continue;
            float ux = dx / l * w / 2, uy = dy / l * w / 2;
            line(x0 - ux, y0 - uy, x1 + ux, y1 + uy, w, col, alpha);
        }
    }
    if (!open) return;
    // stitches across the road every half tile, like paving
    for (float d = 4; d < cut; d += 8) {
        float x, y, x2, y2;
        ck_road_point(r, d / r->total, &x, &y);
        ck_road_point(r, fminf(1, (d + 1) / r->total), &x2, &y2);
        if (fabsf(x2 - x) > fabsf(y2 - y)) rect(x - 0.5f, y - 3, 1, 6, 0xD9B77F, 1);
        else rect(x - 3, y - 0.5f, 6, 1, 0xD9B77F, 1);
    }
}

static void star_icon(float x, float y, float r, uint32_t col, float a) { shape(SPR_STAR, x, y, r, col, a); }

static void draw_map(const ck_game *G) {
    const ck_world *Wd = &G->world;
    const ck_map *M = &G->map;
    float t = M->now;
    blit(SPR_MAP_W1, 0, 0, 0);
    for (int r = 0; r < Wd->nroads; r++) if (!ck_road_open(G, r)) road(&Wd->roads[r], 1, false);
    for (int r = 0; r < Wd->nroads; r++) {
        if (!ck_road_open(G, r)) continue;
        float upto = 1;
        for (int k = 0; k < M->nreveal; k++) if (M->reveal[k].road == r) upto = (t - M->reveal[k].t0) / M->reveal[k].dur;
        road(&Wd->roads[r], upto, true);
    }
    // pads, top to bottom
    int order[CK_MAXNODES], n = Wd->nnodes;
    for (int i = 0; i < n; i++) order[i] = i;
    for (int i = 1; i < n; i++) for (int j = i; j > 0 && Wd->nodes[order[j]].y < Wd->nodes[order[j - 1]].y; j--) { int s = order[j]; order[j] = order[j - 1]; order[j - 1] = s; }
    for (int oi = 0; oi < n; oi++) {
        int id = order[oi];
        const ck_node *nd = &Wd->nodes[id];
        bool beaten = ck_node_beaten(G, id), reach = ck_node_reachable(G, id);
        if (nd->kind == CK_NODE_HOME) continue;   // baked into the map picture
        if (nd->kind == CK_NODE_SHOP) { blit(SPR_MAP_SHOP, reach ? 0 : 1, nd->x, nd->y); continue; }
        if (nd->kind == CK_NODE_EXIT) { blit(SPR_MAP_EXIT, reach ? 0 : 1, nd->x, nd->y); continue; }
        if (nd->kind == CK_NODE_BOSS) blit(SPR_MAP_BOSS, beaten ? 1 : 0, nd->x, nd->y);
        float sz = nd->kind == CK_NODE_BOSS ? 16 : 14, x = nd->x - sz / 2, y = nd->y - sz / 2;
        rect(x - 1, y + 1, sz + 2, sz + 2, 0x1E0F14, 0.4f);
        rect(x - 1, y - 1, sz + 2, sz + 2, 0x2B2230, 1);
        uint32_t fill = nd->kind == CK_NODE_BONUS ? (reach ? 0xFFD23F : 0x8C8496) : beaten ? 0x49D17A : reach ? 0xFF9A3F : 0x8C8496;
        rect(x, y, sz, sz, fill, 1);
        rect(x + 1.5f, y + 1.5f, sz - 3, sz / 2 - 2, 0xFFFFFF, 0.28f);
        if (reach && !beaten && nd->kind != CK_NODE_BONUS && sinf(t * 5) > 0) {
            rect(x - 3, y - 3, sz + 6, 1, 0xFFF6D8, 1); rect(x - 3, y + sz + 2, sz + 6, 1, 0xFFF6D8, 1);
            rect(x - 3, y - 3, 1, sz + 6, 0xFFF6D8, 1); rect(x + sz + 2, y - 3, 1, sz + 6, 0xFFF6D8, 1);
        }
        if (nd->kind == CK_NODE_BONUS) blit_o(SPR_GEAR, 0, nd->x, nd->y, (bopt){ .sx = 0.7f, .sy = 0.7f, .rot = -t, .tinted = true, .tint = hexc(0x8A5D00, 255) });
        else if (!reach && nd->kind == CK_NODE_LEVEL) { rect(nd->x - 3.5f, nd->y - 1, 7, 5.5f, 0x5D566A, 1); rect(nd->x - 2.5f, nd->y - 4, 1.2f, 3, 0x5D566A, 1); rect(nd->x + 1.3f, nd->y - 4, 1.2f, 3, 0x5D566A, 1); rect(nd->x - 2.5f, nd->y - 4.5f, 5, 1.2f, 0x5D566A, 1); }
        else if (nd->kind == CK_NODE_BOSS) txt(F_MED, ST_DARK, nd->x, nd->y + 4, ALIGN_CENTER, "!");
        else txt(F_MED, ST_DARK, nd->x + 0.5f, nd->y + 4, ALIGN_CENTER, "%d", nd->num);
        if (beaten && nd->kind != CK_NODE_BONUS) {
            rect(x + sz - 1, y - 9, 1.5f, 10, 0xEFE7DA, 1);
            rect(x + sz + 0.5f, y - 9, 6 + sinf(t * 6), 4, 0x49D17A, 1);
        }
        if (nd->gate && !reach) txt(F_SMALL, ST_YELLOW, nd->x, y + sz + 9, ALIGN_CENTER, "%d gears", nd->gate);
        // three stars under every beaten level: finished, got everything, star time
        if (nd->kind == CK_NODE_LEVEL && beaten) {
            int got = G->prog.stars[id] ? G->prog.stars[id] : 1;
            for (int k = 0; k < 3; k++) {
                float sx = nd->x - 9 + k * 9, sy = nd->y + 14;
                star_icon(sx, sy, 4, 0x2B2230, 1);
                star_icon(sx, sy, 3.1f, (got >> k) & 1 ? 0xFFD23F : 0xFFFFFF, (got >> k) & 1 ? 1 : 0.35f);
            }
        }
    }
    // heroes, with the rescued babies trailing behind
    float hx = M->hx, hy = M->hy;
    int dir = M->hdir;
    int nb = M->babies < 4 ? M->babies : 4;
    for (int i = 0; i < nb; i++) {
        float hop = fabsf(sinf(t * 6 + i)) * (M->walking ? 2 : 0.6f);
        baby(Wd->babies[i % 4], (int)(t * 3 + i) % 2, hx - dir * (16 + i * 6), hy - 2 - hop, 0.032f / 0.06f);
    }
    int np = G->prog.joined[0] + G->prog.joined[1], k = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        int hero = G->prog.hero[s], col = G->prog.col[s];
        float sc = hero == HERO_ROBOT ? 0.062f / 0.074f : 0.95f * 0.062f / 0.071f, tt = t + k * 0.2f;
        if (M->walking) {
            float ox = np == 1 ? 0 : (k == 0 ? -5 : 4) * dir;
            float f = tt * (hero == HERO_ROBOT ? 11 : 10);
            int fr = 4 + ((int)(f / (2 * PI) * 8)) % 8;
            blit_o(hero_sprite(hero, col), fr, hx + ox, hy - (hero == HERO_OSTRICH), (bopt){ .sx = sc, .sy = sc, .flip = dir < 0 });
        } else {
            float off = np == 1 ? 0 : (k == 0 ? -6 : 6);
            int fr = (int)(tt / (hero == HERO_ROBOT ? 0.52f : 0.3f)) % 4;
            blit_o(hero_sprite(hero, col), fr, hx + off, hy - (hero == HERO_OSTRICH), (bopt){ .sx = sc, .sy = sc });
        }
        k++;
    }
    // HUD
    dark_panel(6, 6, 150, 15);
    char up[40];
    for (int i = 0; i < (int)sizeof up; i++) { char c = Wd->name[i]; up[i] = c >= 'a' && c <= 'z' ? c - 32 : c; if (!c) break; }
    up[sizeof up - 1] = 0;
    txt(F_MED, ST_PLAIN, 12, 17, ALIGN_LEFT, "WORLD %d \xC2\xB7 %s", Wd->num, up);
    dark_panel(232, 6, 82, 15);
    blit_o(SPR_BOLT, 0, 242, 13.5f, (bopt){ .sx = 4 / 4.2f, .sy = 4 / 4.2f });
    txt(F_MED, ST_PLAIN, 248, 17, ALIGN_LEFT, "x%d", M->bolts);
    blit_o(SPR_GEAR, 0, 280, 13.5f, (bopt){ .sx = 4.5f / 7, .sy = 4.5f / 7, .rot = -t * 0.7f });
    txt(F_MED, ST_PLAIN, 286, 17, ALIGN_LEFT, "%d/%d", M->gears, Wd->gears);
    int here = ck_map_here(G);
    float bw = 172, bx = (W - bw) / 2;
    dark_panel(bx, 214, bw, 20);
    if (here >= 0) {
        const ck_node *nd = &Wd->nodes[here];
        if (nd->kind == CK_NODE_LEVEL) txt(F_MED, ST_PLAIN, W / 2, 228, ALIGN_CENTER, "%d-%d  %s", Wd->num, nd->num, nd->label);
        else txt(F_MED, ST_PLAIN, W / 2, 228, ALIGN_CENTER, "%s", nd->label);
        if (nd->kind != CK_NODE_HOME) {
            shape(SPR_DOT, bx + bw - 12, 224, 6, 0x3A8DE0, 1);
            txt(F_SMALL, ST_WHITE, bx + bw - 12, 227, ALIGN_CENTER, "A");
            if (ck_node_beaten(G, here) && nd->kind == CK_NODE_LEVEL) star_icon(bx + 12, 224, 4, 0x49D17A, 1);
        }
        // race clock, best time and star time for the level you are standing on
        if ((nd->kind == CK_NODE_LEVEL || nd->kind == CK_NODE_BOSS || nd->kind == CK_NODE_BONUS) && G->hasDef[here]) {
            const ck_leveldef *d = &G->defs[here];
            char s[64] = "", c[12];
            if (d->time) { strcat(s, "RACE "); strcat(s, clock_str(d->time * 60, c)); strcat(s, "   "); }
            if (G->prog.best[here]) { strcat(s, "BEST "); strcat(s, clock_str((int)G->prog.best[here], c)); strcat(s, "   "); }
            if (d->par && nd->kind == CK_NODE_LEVEL) { strcat(s, "STAR TIME "); strcat(s, clock_str(d->par * 60, c)); }
            if (s[0]) { float w = strlen(s) * 5 + 16; dark_panel(6, 24, w, 14); txt(F_SMALL, ST_PLAIN, 6 + w / 2, 34, ALIGN_CENTER, "%s", s); }
        }
    } else txt(F_MED, ST_PLAIN, W / 2, 228, ALIGN_CENTER, "...");
    if (M->hasMsg) {
        float k2 = fminf(1, (t - M->msgT0) * 6), wd = 150;
        panel((W - wd) / 2, 30 - (1 - k2) * 8, wd, 18, 0xFFD23F, 1);
        txt(F_MED, ST_DARK, W / 2, 42 - (1 - k2) * 8, ALIGN_CENTER, "%s", M->msg);
    }
    lives_hud(G, 4, H - 19);
}

static void draw_shop(const ck_game *G) {
    draw_map(G);
    rect(0, 0, W, H, 0x0F0A19, 0.6f);
    panel(40, 50, 240, 140, 0x3B2A5C, 1);
    txt(F_HUGE, ST_YELLOW, W / 2, 80, ALIGN_CENTER, "PARTS SHOP");
    for (int i = 0; i < 3; i++) rect(64 + i * 68, 92, 56, 50, 0xFFFFFF, 0.08f);
    txt(F_MED, ST_PLAIN, W / 2, 164, ALIGN_CENTER, "Hats and antennas open soon!");
    txt(F_SMALL, ST_GREY, W / 2, 180, ALIGN_CENTER, "Press A to go back");
}

// ---------------------------------------------------------------------------- a level

static bool solid(char c) { return c == '#' || c == '=' || c == 'X' || c == 'D'; }
static char tile_at(const ck_level *lv, int tx, int ty) {
    if (tx < 0 || tx >= lv->w) return '#';
    if (ty < 0 || ty >= lv->h) return '.';
    return lv->tiles[ty][tx];
}

static void draw_tiles(const ck_level *lv, float cam) {
    int x0 = (int)floorf(cam / TS), x1 = x0 + 21;
    if (x0 < 0) x0 = 0;   // the camera can sit left of the level while it shakes
    if (x1 > lv->w - 1) x1 = lv->w - 1;
    bool anyNear = false;
    for (int ty = 0; ty < lv->h; ty++) for (int tx = x0; tx <= x1; tx++) {
        char t = lv->tiles[ty][tx];
        if (t == '.' || t == 'D') continue;
        float x = roundf(tx * TS - cam), y = ty * TS + shakeY;
        unsigned hash = ((unsigned)(tx * 73856093) ^ (unsigned)(ty * 19349663));
        if (t == '#') {
            bool top = !solid(tile_at(lv, tx, ty - 1));
            blit(SPR_TILES, (top ? 0 : 4) + hash % 4, x, y);
            if (!solid(tile_at(lv, tx - 1, ty)) && tx > 0) rect(x, y, 2, TS, 0, 0.15f);
            if (!solid(tile_at(lv, tx + 1, ty)) && tx < lv->w - 1) rect(x + TS - 2, y, 2, TS, 0, 0.2f);
        } else if (t == '=') blit(SPR_TILES, 8, x, y);
        else if (t == '-') blit(SPR_TILES, 9, x, y);
        else if (t == 'X') {
            char c = lv->contents[ty][tx];
            bool prize = c && c != 'o';
            blit(SPR_TILES, prize ? 11 : 10, x, y);
            if (prize) {
                if (sinf(lv->t * 4 + tx) > 0.6f) star_icon(x + 13, y + 3, 2.2f, 0xFFF6A0, 1);
                for (int k = 0; k < lv->nplayers; k++) {
                    const ck_player *p = &lv->players[k];
                    if (fabsf(p->x + p->w / 2 - (tx * TS + 8)) < 48 && fabsf(p->y + p->h - (ty + 1) * TS) < 40) anyNear = true;
                }
                if (anyNear) {
                    float by = y - 12 + sinf(lv->t * 5) * 1.5f;
                    shape(SPR_DOT, x + 8, by, 6, 0xE0564F, 1);
                    txt(F_SMALL, ST_WHITE, x + 8, by + 3.5f, ALIGN_CENTER, "B");
                    anyNear = false;
                }
            }
        }
    }
    // doors slide down into the ground when the lever opens them
    rdpq_set_scissor(0, 0, W, 12 * TS + (int)shakeY);
    for (int ty = 0; ty < lv->h; ty++) for (int tx = x0; tx <= x1; tx++) {
        if (lv->tiles[ty][tx] != 'D') continue;
        float drop = lv->doorT >= 0 ? fminf(1, lv->doorT / 40.0f) * TS * lv->h : 0;
        blit(SPR_TILES, ty % 3 == 0 ? 13 : 12, roundf(tx * TS - cam), ty * TS + drop + shakeY);
    }
    rdpq_set_scissor(0, 0, W, H);
}

static void bubble_at(float cx, float cy, float r) {
    shape(SPR_DOT, cx, cy, r, 0xBFF0FF, 0.28f);
    shape(SPR_RING, cx, cy, r, 0xDFF6FF, 0.9f);
    shape(SPR_DOT, cx - r * 0.38f, cy - r * 0.43f, r * 0.14f, 0xFFFFFF, 0.8f);
}

static void draw_ents(const ck_game *G, float cam) {
    const ck_level *lv = &G->lv;
    for (int i = 0; i < lv->nents; i++) {
        const ck_ent *e = &lv->ents[i];
        if (e->dead && e->type != E_BUTTON) continue;
        if (e->type == E_NONE) continue;
        float x = e->x - cam, y = e->y + shakeY;
        if (x < -40 || x > W + 40) continue;
        switch (e->type) {
        case E_BOLT: {
            float s = fabsf(cosf(e->t * 5)) * 0.8f + 0.2f;
            blit_o(SPR_BOLT, 0, x + 5, y + 5 + (e->fly ? 0 : sinf(e->t * 3)), (bopt){ .sx = s, .sy = 1 });
            break;
        }
        case E_GEAR: {
            float gl = 0.5f + 0.5f * sinf(e->t * 4);
            shape(SPR_DOT, x + 5, y + 5, 11, 0xFFF3A0, 0.35f * gl);
            blit_o(SPR_GEAR, 0, x + 5, y + 5 + sinf(e->t * 2) * 1.5f, (bopt){ .rot = -e->t * 1.5f });
            break;
        }
        case E_ENEMY: {
            float fx = x + e->w / 2, fy = y + e->h;
            if (e->state == ES_SHELL || e->state == ES_KICKED || e->state == ES_CARRIED) {
                float wig = e->state == ES_SHELL && e->shellT > 480 ? sinf(e->shellT * 1.3f) * 1.2f : 0;
                int fr = e->state == ES_KICKED ? (e->ground ? 28 : 29) : 24 + (int)(lv->t * 8) % 4;
                blit_o(SPR_GLITCH, fr, fx + wig, fy, (bopt){ .flip = e->state == ES_KICKED && e->dir < 0 });
                break;
            }
            bool gl = e->state == ES_GLITCHY || e->state == ES_BUBBLED;
            int row = e->kind * 8;
            if (gl) blit(SPR_GLITCH, row + (int)(e->t * 8) % 6, fx, fy);
            else {
                float k = e->happyT / 24.0f;
                blit_o(SPR_GLITCH, row + 6 + (int)(e->t * 4) % 2, fx, fy, (bopt){ .sx = 1 + k * 0.5f, .sy = fmaxf(0.1f, 1 - k * 0.9f) });
            }
            if (e->state == ES_BUBBLED) bubble_at(fx, y + e->h / 2, 13 + sinf(e->bubT * 0.3f) * 0.8f);
            break;
        }
        case E_POWER: {
            float glow = 0.5f + 0.5f * sinf(e->t * 6);
            shape(SPR_DOT, x + 6, y + 6, 10, 0xFFF3A0, 0.3f + glow * 0.3f);
            if (e->kind == PW_RAINBOW) blit_o(SPR_POWER, 1, x, y, (bopt){ .tinted = true, .tint = hexc(ck_cols[(int)(e->t * 10) % 8] | 0x404040, 255) });
            else blit(SPR_POWER, e->kind - 1, x, y);
            break;
        }
        case E_SHOT:
            shape(SPR_DOT, x + 5, y + 5, 5.5f, 0xBFF0FF, 0.3f);
            shape(SPR_RING, x + 5, y + 5, 5.5f, 0xDFF6FF, 1);
            shape(SPR_DOT, x + 3, y + 3, 1.3f, 0xFFFFFF, 1);
            break;
        case E_FLAG: blit(SPR_FLAG, e->on ? (sinf(lv->t * 6) > 0 ? 1 : 2) : 0, x, y); break;
        case E_HATCH:
        case E_HATCHOUT:
            blit(SPR_HATCH, e->type == E_HATCH ? 0 : 1, x, y);
            if (e->type == E_HATCH) {
                bool near = false;
                for (int k = 0; k < lv->nplayers; k++) if (fabsf(lv->players[k].x + lv->players[k].w / 2 - (e->x + 12)) < 40) near = true;
                if (near || sinf(lv->t * 4) > 0) blit(SPR_ARROW, 0, x + 6, y - 16 + sinf(lv->t * 6) * 2);
            }
            break;
        case E_MOVER:
            if (e->kind == MV_CRUSHER) {
                float sh = e->state == CR_WARN ? sinf(lv->frame * 1.8f) * 1.5f : 0, cx = x + sh;
                rect(cx + 13, shakeY, 6, e->y, 0x4A4652, 1);
                rect(cx, y, 32, 32, 0x7D8397, 1);
                rect(cx + 2, y + 2, 28, 4, 0xA9B0C0, 1);
                for (int k = 0; k < 4; k++) rect(cx + k * 8, y + 24, 4, 8, e->state == CR_WARN && lv->frame % 8 < 4 ? 0xFFF6A0 : 0xFFCF3A, 1);
                rect(cx + 8, y + 12, 4, e->state == CR_DOWN ? 2 : 5, 0x2B2230, 1);
                rect(cx + 20, y + 12, 4, e->state == CR_DOWN ? 2 : 5, 0x2B2230, 1);
                break;
            }
            if (e->kind == MV_HOOK) {
                float px = e->px - cam;
                line(px, e->py + shakeY, x + e->w / 2, y - 10, 1.5f, 0x3A3346, 1);
                shape(SPR_DOT, px, e->py + shakeY, 3, 0xD4892F, 1);
                line(x + e->w / 2, y - 10, x + 3, y, 1, 0x5D566A, 1);
                line(x + e->w / 2, y - 10, x + e->w - 3, y, 1, 0x5D566A, 1);
            } else {
                // the raft's track, dashed
                float ax = e->ax - cam + 16, ay = e->ay + 3 + shakeY, bx = e->bx - cam + 16, by = e->by2 + 3 + shakeY;
                float len = sqrtf((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
                for (float d = 0; d < len; d += 6) {
                    float k = d / len, k2 = fminf(1, (d + 3) / len);
                    line(ax + (bx - ax) * k, ay + (by - ay) * k, ax + (bx - ax) * k2, ay + (by - ay) * k2, 2, 0x3C2832, 0.35f);
                }
            }
            rect(x, y, e->w, e->h, 0x7A8A96, 1);
            rect(x + 1, y, e->w - 2, 2, 0xA9B8C2, 1);
            for (int k = 0; k < 3; k++) rect(x + 9 + k * 6, y + 2.5f, 3, 2, 0xFFCF3A, 1);
            break;
        case E_SIGN: blit(SPR_SIGN, 0, x, y); break;
        case E_TYRE: blit(SPR_TYRE, e->sq > 0.66f ? 2 : e->sq > 0.2f ? 1 : 0, x, y); break;
        case E_LEVER:
            blit(SPR_LEVER, e->on ? 1 : 0, x, y);
            if (!e->on && sinf(lv->t * 5) > 0.3f) star_icon(x + 12, y - 2, 2, 0xFFFFFF, 1);
            break;
        case E_BUTTON:
            if (e->dead) { blit(SPR_BUTTON, 1, x, y); break; }
            {
                float pulse = 0.5f + 0.5f * sinf(lv->t * 8);
                blit(SPR_BUTTON, 0, x, y);
                shape(SPR_DOT, x + 7, y + 1, 10, 0xFF8A8A, 0.4f * pulse);
                txt(F_SMALL, ST_YELLOW, x + 7, y - 6 - pulse * 2, ALIGN_CENTER, "!");
            }
            break;
        case E_PAD: {
            blit(SPR_PAD, 0, x, y);
            if (sinf(lv->t * 6) > 0.5f) star_icon(x + 4 + ((int)(lv->t * 3) % 3) * 8, y - 18, 2, 0xBFF6FF, 1);
            float hop = lv->clearT >= 0 ? fabsf(sinf(lv->t * 8)) * 10 : fabsf(sinf(lv->t * 3)) * 2;
            int fr = lv->clearT >= 0 ? 2 + (int)(lv->t * 4) % 2 : (sinf(lv->t * 6) > 0 ? 0 : 1);
            baby(lv->def->baby, fr, x + 12, y - hop, 1);
            break;
        }
        }
    }
}

static void draw_crane(const ck_level *lv) {
    if (!lv->hasCrane) return;
    float sh = lv->crane.shake > 0 ? sinf(lv->crane.shake * 1.7f) * 2 : 0, tx = 286 + sh;
    blit(SPR_CRANE, 0, sh, shakeY);
    blit(SPR_CAB, lv->crane.ouch > 0 ? 1 : lv->crane.fixed ? 2 : 0, tx, shakeY);
    if (!lv->crane.fixed && (int)(lv->t * 8) % 3 == 0) {
        line(tx + 18, 4 + shakeY, tx + 22, 9 + shakeY, 1, 0xBFF6FF, 1);
        line(tx + 22, 9 + shakeY, tx + 19, 10 + shakeY, 1, 0xBFF6FF, 1);
        line(tx + 19, 10 + shakeY, tx + 24, 15 + shakeY, 1, 0xBFF6FF, 1);
    }
    float mx = lv->crane.mx ? lv->crane.mx : 160, my = lv->crane.my ? lv->crane.my : 164;
    line(160, 24 + shakeY, mx, my - 8 + shakeY, 1.5f, 0x3A3346, 1);
    blit_o(SPR_MAGNET, 0, mx, my + shakeY, (bopt){ .rot = lv->crane.ang });
    if (!lv->crane.fixed) { float k = fmodf(lv->t * 1.5f, 1); shape(SPR_RING, mx, my + shakeY, 12 + k * 8, 0xFF6B6B, 0.5f * (1 - k)); }
    for (int i = 0; i < 3; i++) shape(SPR_DOT, 140 + i * 20, 220, 5, i < lv->crane.hits ? 0x49D17A : 0xFFFFFF, i < lv->crane.hits ? 1 : 0.25f);
}

static void draw_player(const ck_game *G, const ck_player *p, float cam, int nplayers) {
    const ck_level *lv = &G->lv;
    if (p->inv > 0 && p->wob <= 0 && !p->dead && !p->bubble && (p->inv / 4) % 2 == 0 && lv->clearT < 0) return;
    float fx = p->x + p->w / 2 - cam, fy = p->y + p->h + shakeY, t = lv->t + p->i * 0.7f;
    int sheet = hero_sprite(p->hero, p->col);
    bool R = p->hero == HERO_ROBOT;
    if (p->dead) { blit(sheet, 27, fx, fy); return; }
    if (p->bubble) {
        blit(sheet, 28, fx, fy);
        bubble_at(fx, p->y + p->h / 2 + shakeY, 21);
        return;
    }
    float mega = ck_has(p, PW_MEGA) ? 1.5f : 1;
    bool rainbow = p->rainbowT > 0;
    // power-up auras behind the hero
    if (ck_has(p, PW_MEGA)) shape(SPR_DOT, fx, fy - p->h / 2, p->h * 0.75f, 0x78FFA0, 0.3f + 0.12f * sinf(t * 6));
    if (rainbow) shape(SPR_DOT, fx, fy - p->h / 2, p->h * 0.7f, ck_cols[(int)(t * 12) % 8], 0.35f);
    if (p->magnetT > 0) { float k = fmodf(t * 1.2f, 1); shape(SPR_RING, fx, fy - p->h / 2, 8 + k * 22, 0xFF6B6B, 0.5f * (1 - k)); }
    int fr;
    bool side = true;
    if (lv->clearT >= 0) { fr = 25 + (int)(t * 5) % 2; side = false; }
    else if (p->low && p->slideT > 0) fr = 20;
    else if (p->low) fr = 19;
    else if (p->wob > 0) fr = 17 + (int)(t * 8) % 2;
    else if (p->actT > 0 && p->shoot) fr = 24;
    else if (p->actT > 0) fr = p->actT > 16 ? 21 : p->actT > 10 ? 22 : 23;
    else if (!p->ground) {
        if (R) fr = p->jetT > 0 ? 13 + (int)(t * 20) % 2 : 12;
        else fr = p->flapping ? 13 + ((int)(t * 13 / (2 * PI) * 4)) % 4 : 12;
    } else if (fabsf(p->vx) > 0.25f) {
        float f = R ? p->anim * 1.6f * 11 : p->anim * 1.6f / 1.1f * 10;
        fr = 4 + ((int)(f / (2 * PI) * 8)) % 8;
    } else { fr = (int)(t / (R ? 0.52f : 0.3f)) % 4; side = false; }
    bopt o = { .sx = mega, .sy = mega, .flip = side && p->face < 0 };
    if (rainbow) { o.tinted = true; o.tint = hexc(ck_cols[(int)(t * 12 + p->i * 3) % 8] | 0x606060, 255); }
    blit_o(sheet, fr, fx, fy, o);
    if (rainbow && lv->frame % 3 == 0) { /* sparkles come from the particle list in the browser; a cheap twinkle here */
        star_icon(fx + sinf(t * 17) * 6, fy - p->h * (0.5f + 0.4f * cosf(t * 13)), 2, ck_cols[(int)(t * 7) % 8], 1);
    }
    if (p->magnetT > 0 && (p->magnetT > 180 || (p->magnetT / 8) % 2))
        blit_o(SPR_PICON, 3, fx, fy - p->h - 4, (bopt){ .sx = 1.3f * mega, .sy = 1.3f * mega });
    if (ck_has(p, PW_BUBBLE) && lv->clearT < 0) {
        float cy = fy - p->h * 0.5f, rx = 11 * mega, ry = p->h * 0.28f, br = 3.4f * mega;
        static const uint32_t ring[3] = { 0x8FD3FF, 0xFF8FC7, 0xC28BFF };
        for (int k = 0; k < 3; k++) {
            float a = t * 2.6f + k * PI * 2 / 3, bx = fx + cosf(a) * rx, by = cy + sinf(a) * ry, al = sinf(a) > 0 ? 1 : 0.55f;
            shape(SPR_DOT, bx, by, br, 0xC8F2FF, 0.35f * al);
            shape(SPR_RING, bx, by, br, ring[k], al);
            shape(SPR_DOT, bx - br * 0.35f, by - br * 0.35f, br * 0.28f, 0xFFFFFF, al);
        }
    }
    if (nplayers > 1) {
        float ly = fy - p->h - (p->magnetT > 0 ? 18 : 12);
        txt(F_SMALL, p->i ? ST_BLUE : ST_YELLOW, fx, ly + 2, ALIGN_CENTER, "%d", p->slot + 1);
    }
}

static void level_hud(const ck_game *G) {
    const ck_level *lv = &G->lv;
    const ck_leveldef *def = lv->def;
    char c[12], c2[12];
    dark_panel(4, 4, 62, 15);
    blit_o(SPR_BOLT, 0, 13, 11.5f, (bopt){ .sx = 4 / 4.2f, .sy = 4 / 4.2f });
    txt(F_MED, lv->bolts ? ST_PLAIN : ST_RED, 20, 16, ALIGN_LEFT, "x%d", lv->bolts);
    lives_hud(G, W - 52, 4);
    for (int k = 0; k < lv->nplayers; k++) {
        const ck_player *p = &lv->players[k];
        int kinds[4], n = 0;
        for (int s = 0; s < p->nstack; s++) kinds[n++] = p->stack[s];
        if (p->rainbowT > 0) kinds[n++] = PW_RAINBOW;
        if (p->magnetT > 0) kinds[n++] = PW_MAGNET;
        if (!n) continue;
        float x0 = W - 56 - (k + 1) * (n * 12 + 8);
        dark_panel(x0, 4, n * 12 + 4, 15);
        for (int j = 0; j < n; j++) blit(SPR_PICON, kinds[j] - 1, x0 + 8 + j * 12, 11.5f);
    }
    if (!lv->hasShower && lv->clearT < 0) {
        int left = def->time ? def->time * 60 - lv->timeF : 0;
        if (left < 0) left = 0;
        bool warn = def->time && left < 600;
        dark_panel(128, 4, 46, 15);
        blit_o(SPR_CLOCK, 0, 137, 11.5f, (bopt){ .tinted = warn, .tint = hexc(0xFF8A8A, 255) });
        txt(F_MED, warn && (int)(lv->t * 4) % 2 ? ST_RED : ST_PLAIN, 158, 16, ALIGN_CENTER, "%s", clock_str(def->time ? left : lv->timeF, c));
    }
    if (def->time && lv->frame < 200 && lv->clearT < 0 && lv->dyingT < 0) {
        panel(W / 2 - 70, 150, 140, 22, 0xFFD23F, 1);
        txt(F_MED, ST_DARK, W / 2, 165, ALIGN_CENTER, "RACE THE CLOCK!");
    }
    if (lv->timeUp && lv->dyingT > 10) { panel(W / 2 - 60, 60, 120, 22, 0xFF8A8A, 1); txt(F_BIG, ST_DARK, W / 2, 76, ALIGN_CENTER, "TIME'S UP!"); }
    if (lv->toastT > 0 && lv->toast[0]) txt(F_BIG, ST_YELLOW, W / 2, 50, ALIGN_CENTER, "%s", lv->toast);
    if (def->kind == CK_KIND_LEVEL) {
        dark_panel(70, 4, 50, 15);
        for (int k = 0; k < 3; k++) {
            bool have = lv->got[k] || lv->gotNow[k];
            blit_o(SPR_GEAR, 0, 82 + k * 13, 11.5f, (bopt){ .sx = 5.0f / 7, .sy = 5.0f / 7, .rot = have ? -lv->t : 0, .tinted = !have, .tint = RGBA32(255, 255, 255, 60) });
        }
    }
    if (lv->hasShower && lv->showerLeft > 0) {
        dark_panel(128, 4, 64, 15);
        txt(F_MED, lv->showerLeft < 300 ? ST_RED : ST_CREAM, 160, 16, ALIGN_CENTER, "%ds", (lv->showerLeft + 59) / 60);
    }
    if (lv->nameT > 0) {
        dark_panel(W / 2 - 90, 92, 180, lv->retry ? 44 : 30);
        txt(F_MED, ST_YELLOW, W / 2, 112, ALIGN_CENTER, "%s", lv->retry ? "TRY AGAIN!" : def->name);
        if (lv->retry) txt(F_SMALL, ST_PLAIN, W / 2, 128, ALIGN_CENTER, "%d %s left", G->prog.lives, G->prog.lives > 1 ? "tries" : "try");
    }
    if (lv->dyingT > 40) { dark_panel(W / 2 - 80, 96, 160, 34); txt(F_BIG, ST_YELLOW, W / 2, 118, ALIGN_CENTER, G->prog.lives > 0 ? "OOPS!" : "OUT OF TRIES"); }
    if (lv->hasCrane && !lv->crane.fixed && lv->frame < 260) txt(F_MED, ST_CREAM, W / 2, 140, ALIGN_CENTER, "Bounce on the red buttons!");
    if (lv->hasShower && lv->frame < 180) txt(F_MED, ST_CREAM, W / 2, 140, ALIGN_CENTER, "Catch the bolts!");
    if (lv->clearT >= 0) {
        float k = fminf(1, lv->clearT / 20.0f);
        panel(W / 2 - 80, 60 - (1 - k) * 30, 160, 34, 0xFFD23F, 1);
        txt(F_HUGE, ST_DARK, W / 2, 84 - (1 - k) * 30, ALIGN_CENTER, def->kind == CK_KIND_BOSS ? "FIXED!" : def->kind == CK_KIND_BONUS ? "BONUS!" : "RESCUED!");
        if (lv->clearT > 40) {
            int nG = 0;
            for (int g = 0; g < 3; g++) if (lv->gotNow[g] && !((G->prog.got[lv->node] >> g) & 1)) nG++;
            if (nG) txt(F_MED, ST_CREAM, W / 2, 112, ALIGN_CENTER, "+%d bolts   +%d golden gear%s", lv->bolts, nG, nG > 1 ? "s" : "");
            else txt(F_MED, ST_CREAM, W / 2, 112, ALIGN_CENTER, "+%d bolts", lv->bolts);
        }
        const ck_result *R = &lv->result;
        if (lv->hasResult && lv->clearT > 55) {
            if (def->par) txt(F_SMALL, ST_PLAIN, W / 2, 126, ALIGN_CENTER, "TIME %s   (star time %s)", clock_str(R->time, c), clock_str(def->par * 60, c2));
            else txt(F_SMALL, ST_PLAIN, W / 2, 126, ALIGN_CENTER, "TIME %s", clock_str(R->time, c));
            if (R->hasStars) {
                static const char *names[3] = { "DONE", "GOT IT ALL", "SPEEDY" };
                for (int s = 0; s < 3; s++) {
                    int at = 70 + s * 18;
                    float pop = lv->clearT < at ? 0 : fminf(1, (lv->clearT - at) / 8.0f);
                    bool got = R->stars[s];
                    float sx = W / 2 - 56 + s * 56, sy = 150;   // wide enough for "GOT IT ALL" in the small font
                    float r = 9 * (got ? 0.6f + pop * 0.4f + (pop < 1 ? 0.3f * sinf(pop * PI) : 0) : 0.8f);
                    if (lv->clearT >= at || !got) star_icon(sx, sy, r + 1.5f, got ? 0x7A4A10 : 0x1B1426, got ? 1 : 0.6f);
                    if (got && lv->clearT >= at) star_icon(sx, sy, r, 0xFFD23F, 1);
                    else star_icon(sx, sy, r - 1, 0xFFFFFF, 0.25f);
                    txt(F_SMALL, got && lv->clearT >= at ? ST_PLAIN : ST_GREY, sx, sy + 18, ALIGN_CENTER, "%s", names[s]);
                }
            }
            if (R->record && lv->clearT > 140) { panel(W / 2 - 58, 183, 116, 18, 0xFF8FC7, 1); txt(F_MED, ST_DARK, W / 2, 197, ALIGN_CENTER, "NEW RECORD!"); }
        }
    }
}

static void draw_level(const ck_game *G) {
    const ck_level *lv = &G->lv;
    float cam = floorf(lv->camX);
    int far = lv->def->kind == CK_KIND_BOSS ? SPR_BG_FAR_BOSS : SPR_BG_FAR;
    float off = fmodf(lv->camX * 0.2f, 640);
    blit(far, 0, -off, 0);
    if (off > 320) blit(far, 0, 640 - off, 0);
    float mo = fmodf(lv->camX * 0.5f, 640);
    blit(SPR_BG_MID, 0, -mo, 0);
    if (mo > 320) blit(SPR_BG_MID, 0, 640 - mo, 0);
    shakeY = lv->shake > 0 ? sinf(lv->shake * 2.3f) * 2 : 0;
    draw_crane(lv);
    draw_tiles(lv, cam);
    draw_ents(G, cam);
    for (int k = 0; k < lv->nplayers; k++) draw_player(G, &lv->players[k], cam, lv->nplayers);
    particles(G, cam);
    shakeY = 0;
    level_hud(G);
}

// ---------------------------------------------------------------------------- the jetpack zone

static void draw_jet(const ck_game *G) {
    const ck_jet *J = &G->jet;
    const ck_level *lv = &G->lv;
    float sc = J->scroll;
    blit(SPR_JET_SKY, 0, 0, 0);
    for (int k = 0; k < 5; k++) {
        float cx = fmodf(fmodf(k * 110 - sc * 0.2f, 550) + 550, 550) - 60;
        blit(SPR_CLOUD, 0, cx, 30 + (k % 3) * 26);
    }
    float ho = fmodf(fmodf(sc * 0.5f, 640) + 640, 640);
    blit(SPR_JET_HILLS, 0, -ho, 180);
    if (ho > 320) blit(SPR_JET_HILLS, 0, 640 - ho, 180);
    rect(0, 224, W, 16, 0x8A5233, 1);
    rect(0, 224, W, 3, 0xC98B55, 1);
    for (int i = 0; i < J->nobs; i++) {
        const ck_obstacle *o = &J->obs[i];
        float x = o->x - sc;
        if (x < -40 || x > W + 10) continue;
        if (o->kind == 2) {
            for (float y = o->y; y < 224; y += 28) blit(SPR_PILLAR, 0, x, y);
        } else {
            bool top = o->kind == 0;
            float y0 = o->y < 0 ? 0 : o->y, y1 = fminf(o->y + o->h, 224);
            for (float y = y0; y < y1; y += 16) blit(SPR_PIPE, 0, x, fminf(y, y1 - 16));
            blit(SPR_PIPELIP, 0, x, top ? o->y + o->h - 10 : o->y);
        }
    }
    for (int i = 0; i < J->nitems; i++) {
        const ck_item *it = &J->items[i];
        if (it->dead) continue;
        float x = it->x - sc;
        if (x < -10 || x > W + 10) continue;
        blit_o(SPR_BOLT, 0, x + 5, it->y + 5, (bopt){ .sx = fabsf(cosf(it->t * 5)) * 0.8f + 0.2f, .sy = 1 });
    }
    float endX = J->len + 260 - sc;
    if (endX < W + 30) blit(SPR_JETEXIT, 0, endX, 60);
    for (int k = 0; k < lv->nplayers; k++) {
        const __typeof__(J->ps[0]) *p = &J->ps[k];
        if (p->inv > 0 && (p->inv / 4) % 2 == 0) continue;
        const ck_player *src = &lv->players[k];
        blit(hero_sprite(src->hero, src->col), 29 + (int)(J->t * 14) % 2, p->x + p->w / 2, p->y + p->h);
    }
    particles(G, sc);
    dark_panel(4, 4, 62, 15);
    blit_o(SPR_BOLT, 0, 13, 11.5f, (bopt){ .sx = 4 / 4.2f, .sy = 4 / 4.2f });
    txt(F_MED, ST_PLAIN, 20, 16, ALIGN_LEFT, "x%d", lv->bolts + J->bolts);
    if (J->nameT > 0) {
        dark_panel(W / 2 - 90, 92, 180, 44);
        txt(F_MED, ST_YELLOW, W / 2, 112, ALIGN_CENTER, "%s", J->name);
        txt(F_SMALL, ST_PLAIN, W / 2, 128, ALIGN_CENTER, "Up or A to fly up   Down to go down");
    }
    if (J->endT >= 0) rect(0, 0, W, H, 0, fminf(1, J->endT / 40.0f));
}

static void draw_pause(const ck_game *G, bool scan) {
    draw_level(G);
    const ck_level *lv = &G->lv;
    rect(0, 0, W, H, 0x0F0A19, 0.6f);
    dark_panel(80, 74, 160, 92);
    txt(F_BIG, ST_YELLOW, W / 2, 100, ALIGN_CENTER, "PAUSED");
    txt(F_SMALL, ST_PLAIN, W / 2, 122, ALIGN_CENTER, "A: keep playing   B: back to map");
    if (lv->def->kind == CK_KIND_LEVEL)
        txt(F_SMALL, ST_PALE, W / 2, 138, ALIGN_CENTER, "Bolts %d/%d    Glitchies left %d", lv->boltGot, lv->boltTotal, ck_foes_left(lv));
    txt(F_SMALL, ST_GREY, W / 2, 156, ALIGN_CENTER, "Z: TV lines %s", scan ? "on" : "off");
}

// ---------------------------------------------------------------------------- a frame

void draw_frame(const ck_game *G, bool scanlines) {
    free_for(G);
    mode = M_NONE;
    shakeY = 0;
    rdpq_set_scissor(0, 0, W, H);
    switch (G->scene) {
    case SC_INTRO: draw_intro(G); break;
    case SC_TITLE: draw_title(G); break;
    case SC_SELECT: draw_select(G); break;
    case SC_MAP: draw_map(G); break;
    case SC_SHOP: draw_shop(G); break;
    case SC_LEVEL: draw_level(G); break;
    case SC_JET: draw_jet(G); break;
    case SC_PAUSE: draw_pause(G, scanlines); break;
    }
    if (scanlines) {
        // a hint of a CRT: darken every other line
        mode_flat();
        rdpq_set_prim_color(RGBA32(0, 0, 0, 70));
        for (int y = 1; y < H; y += 2) rdpq_fill_rectangle(0, y, W, y + 1);
    }
}

void draw_overlay(const char *text) {
    mode = M_NONE;
    rect(0, 0, W, 12, 0x000000, 0.7f);
    txt(F_SMALL, ST_YELLOW, 4, 9, ALIGN_LEFT, "%s", text);
}
