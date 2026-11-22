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
    static const int big[] = { SPR_TITLE, SPR_SELECT_BG, SPR_JET_SKY, SPR_JET_HILLS, SPR_CRANE, SPR_INTRO_NIGHT,
                               SPR_INTRO_STORM, SPR_INTRO_DAWN, SPR_CUT_FLOAT, SPR_CUT_ROCKET,
                               SPR_AQUA_BG, SPR_AQUA_WATER, SPR_HOUSE_FURN };
    for (unsigned i = 0; i < sizeof big / sizeof *big; i++) if (big[i] == id) return true;
    // one of each per world or film
    for (int k = 0; k < CK_WORLDS; k++)
        if (id == SPR_MAP_W1 + k || id == SPR_BG_FAR_0 + k || id == SPR_BG_FAR_BOSS_0 + k || id == SPR_BG_MID_0 + k || id == SPR_CUT_BG_1 + k) return true;
    return false;
}
static void acc_init(void);
// col is a hero colour 0..7, or SKIN_RAINBOW: the Snow sheet tinted through the rainbow (skin_o)
#define SKIN_RAINBOW CK_NCOLS
static int hero_sprite(int hero, int col) { return (hero == HERO_ROBOT ? SPR_HERO_ROBOT_0 : SPR_HERO_OSTRICH_0) + (col >= CK_NCOLS ? CK_RAINBOW_COL : col & 7); }
static int skin_col(const ck_game *G, int s) { return ck_rainbow_skin(G, s) ? SKIN_RAINBOW : G->prog.col[s]; }

static color_t hexc(uint32_t hex, int a) { return RGBA32((hex >> 16) & 255, (hex >> 8) & 255, hex & 255, a); }

void draw_init(void) {
    static const struct { uint32_t col, out; int outA; } styles[ST_COUNT] = {
        // every style is light with a dark outline, so words read on any background (ST_DARK is kept
        // for old callers but drawn light too: dark words blur away on a TV)
        { 0xFFF6D8, 0x1B1426, 255 }, { 0xFFD23F, 0x1B1426, 255 }, { 0xFFF6D8, 0x1B1426, 255 }, { 0xFF8A8A, 0x1B1426, 255 },
        { 0x8FD3FF, 0x1B1426, 255 }, { 0xD9D2E6, 0x1B1426, 255 }, { 0xFFF6D8, 0x1B1426, 255 }, { 0x49D17A, 0x1B1426, 255 },
        { 0xFFFFFF, 0x1B1426, 255 }, { 0xA49DB4, 0x1B1426, 255 },
    };
    static const char *files[4] = { "rom:/titan8.font64", "rom:/titan10.font64", "rom:/titan14.font64", "rom:/titan18.font64" };
    for (int f = 0; f < 4; f++) {
        rdpq_font_t *font = rdpq_font_load(files[f]);
        for (int s = 0; s < ST_COUNT; s++)
            rdpq_font_style(font, s, &(rdpq_fontstyle_t){ .color = hexc(styles[s].col, 255), .outline_color = hexc(styles[s].out, styles[s].outA) });
        rdpq_text_register_font(F_SMALL + f, font);
    }
    acc_init();
}

// Free the big pictures and hero sheets the next scene does not need.
static void free_for(const ck_game *G) {
    int sc = G->scene;
    int group = sc == SC_INTRO ? 4 : (sc == SC_TITLE || sc == SC_SLOTS) ? 0 : sc == SC_HOUSE ? 6 : sc == SC_AQUA ? 7 : sc == SC_CARS ? 8 : (sc == SC_PAINT || sc == SC_PIANO || sc == SC_PINBALL || sc == SC_BLAST || sc == SC_FISH) ? 9 : G->scene == SC_SELECT ? 1 : (G->scene == SC_MAP || G->scene == SC_SHOP || G->scene == SC_MENU) ? 2 : G->scene == SC_CUT ? 5 : 3;
    group = group * 16 + G->worldNum * 4 + (G->scene == SC_CUT ? G->cut % 4 : 0);   // a new world or film needs its own pictures
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
            for (int s = 0; s < 2; s++) {
                if (!G->prog.joined[s]) continue;
                int hh = G->prog.hero[s], cc = G->prog.col[s] % CK_NCOLS;
                if (G->scene == SC_SHOP) {   // the hero trying on another hero or paint keeps that sheet too
                    int a = G->shopCur[s];
                    if (a == SHOP_ROBOT || a == SHOP_OSTRICH) hh = a == SHOP_ROBOT ? HERO_ROBOT : HERO_OSTRICH;
                    else if (a >= SHOP_PAINT0 && a < SHOP_PAINT0 + CK_NCOLS) cc = a - SHOP_PAINT0;
                    else if (a == SHOP_RAINBOW) cc = CK_RAINBOW_COL;
                    if (hh == h && cc == c) keep = true;
                }
                if (G->prog.hero[s] == h && G->prog.col[s] % CK_NCOLS == c) keep = true;
            }
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

// ---------------------------------------------------------------------------- accessories
// Parts-shop accessories are baked once per hero view into a strip per slot (see the acc_* strips
// and ACC_AT in sprites.h); they go on top of the hero sprite where that frame's head, body and
// feet are. Items that cycle colour are baked white and tinted here.
static const int ACC_SPR[SLOT_COUNT] = { SPR_ACC_HEAD, SPR_ACC_FACE, SPR_ACC_NECK, SPR_ACC_BODY, SPR_ACC_FEET };
static int acc_first[SLOT_COUNT], acc_n[SLOT_COUNT];
static void acc_init(void) {
    for (int a = 0; a < ACC_COUNT; a++) if (!acc_n[ck_acc[a].slot]++) acc_first[ck_acc[a].slot] = a;
}
static color_t rainbow_col(float t, int alpha) {
    float h = t * 0.6f * 2 * PI;
    return RGBA32((int)(150 + 105 * sinf(h)), (int)(150 + 105 * sinf(h + 2.094f)), (int)(150 + 105 * sinf(h + 4.189f)), alpha);
}
// a hero blit's options with the rainbow paint's tint (keeps any fade already in o)
static bopt skin_o(bopt o, int col, float t) {
    if (col < CK_NCOLS) return o;
    int a = o.tinted ? o.tint.a : 255;
    o.tinted = true;
    o.tint = rainbow_col(t * 0.8f, a);
    return o;
}
// one item at a point of a hero frame; view: 0 robot front, 1 robot side, 2 ostrich front, 3 ostrich side
static void acc_piece(int a, int view, const acc_pt *p, float x, float y, float sc, bool flip, float t, int shade, int alpha) {
    int slot = ck_acc[a].slot;
    bopt o = { .flip = flip, .sx = p->sx * sc, .sy = p->sy * sc, .rot = flip ? p->rot : -p->rot, .tinted = true,
               .tint = ck_acc[a].rainbow ? rainbow_col(t, alpha) : RGBA32(shade, shade, shade, alpha) };
    if (ck_acc[a].rainbow && shade < 255) { o.tint.r = o.tint.r * shade / 255; o.tint.g = o.tint.g * shade / 255; o.tint.b = o.tint.b * shade / 255; }
    blit_o(ACC_SPR[slot], view * acc_n[slot] + a - acc_first[slot], x + (flip ? -p->x : p->x) * sc, y + p->y * sc, o);
}
// Suits have sleeves: a stroke down each arm in the suit's colour (white cuffs on the sweater and
// the fancy suit), as K.wearSleeve draws them in the browser.
static void line(float x0, float y0, float x1, float y1, float w, uint32_t hex, float alpha);
static uint32_t sleeve_col(int a) {
    switch (a) {
    case ACC_SWEATER: return 0xE0564F;
    case ACC_JERSEY: return 0x49D17A;
    case ACC_TUX: return 0x2B2833;
    case ACC_SUPER: return 0x3A6FC0;
    case ACC_DISCO: return 1;   // cycles colour
    default: return 0;
    }
}
static void sleeve(int a, const acc_arm *m, float x, float y, float sc, bool flip, float t, int alpha) {
    uint32_t c = sleeve_col(a);
    if (!c) return;
    if (c == 1) { color_t r = rainbow_col(t, 255); c = (uint32_t)r.r << 16 | (uint32_t)r.g << 8 | r.b; }
    else if (m->far) c = ((c >> 16 & 255) * 3 / 4) << 16 | ((c >> 8 & 255) * 3 / 4) << 8 | (c & 255) * 3 / 4;
    float f = flip ? -1 : 1, x0 = x + m->x0 * sc * f, y0 = y + m->y0 * sc, x1 = x + m->x1 * sc * f, y1 = y + m->y1 * sc, w = fmaxf(1.5f, m->w * sc);
    line(x0, y0, x1, y1, w, c, alpha / 255.0f);
    if (a == ACC_SWEATER || a == ACC_TUX) {
        float dx = x1 - x0, dy = y1 - y0;
        line(x0 + dx * 0.86f, y0 + dy * 0.86f, x0 + dx * 1.04f, y0 + dy * 1.04f, w, a == ACC_TUX ? 0xF5F1E8 : 0xFFFFFF, alpha / 255.0f);
    }
}
// everything a player wears, over hero frame fr drawn at (x, y) with scale sc
static void hero_acc_a(const uint8_t *wear, int hero, int fr, float x, float y, float sc, bool flip, float t, int alpha) {
    if (!wear) return;
    const acc_at *A = &ACC_AT[hero == HERO_ROBOT ? 0 : 1][fr & 31];
    int view = (hero == HERO_ROBOT ? 0 : 2) + A->side;
    int w[SLOT_COUNT];
    bool any = false;
    for (int k = 0; k < SLOT_COUNT; k++) { w[k] = (int)wear[k] - 1; if (w[k] >= 0 && w[k] < ACC_COUNT && ck_acc[w[k]].slot == k) any = true; else w[k] = -1; }
    if (!any) return;
    if (w[SLOT_BODY] >= 0) for (int k = 0; k < A->narms; k++) if (A->arm[k].far) sleeve(w[SLOT_BODY], &A->arm[k], x, y, sc, flip, t, alpha);
    if (w[SLOT_FEET] >= 0) for (int f = 0; f < A->nfeet; f++) acc_piece(w[SLOT_FEET], view, &A->foot[f], x, y, sc, flip, t, A->far[f] ? 190 : 255, alpha);
    if (w[SLOT_BODY] >= 0 && A->has_body) acc_piece(w[SLOT_BODY], view, &A->body, x, y, sc, flip, t, 255, alpha);
    if (w[SLOT_BODY] >= 0) for (int k = 0; k < A->narms; k++) if (!A->arm[k].far) sleeve(w[SLOT_BODY], &A->arm[k], x, y, sc, flip, t, alpha);
    if (w[SLOT_NECK] >= 0) acc_piece(w[SLOT_NECK], view, &A->head, x, y, sc, flip, t, 255, alpha);
    if (w[SLOT_FACE] >= 0) acc_piece(w[SLOT_FACE], view, &A->head, x, y, sc, flip, t, 255, alpha);
    if (w[SLOT_HEAD] >= 0) acc_piece(w[SLOT_HEAD], view, &A->head, x, y, sc, flip, t, 255, alpha);
}
static void hero_acc(const uint8_t *wear, int hero, int fr, float x, float y, float sc, bool flip, float t) {
    hero_acc_a(wear, hero, fr, x, y, sc, flip, t, 255);
}
static const uint8_t *wear_of(const ck_game *G, int s) { return s >= 0 && s < 2 ? G->saves.closet.wear[s] : NULL; }

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
    // rdpq_text reads $xx and ^xx as font and style changes; double them to print them as they are.
    // A lone space is doubled too: the font's space is narrow and words crowd together on a TV.
    size_t n = 0;
    for (const char *c = raw; *c && n < sizeof buf - 2; c++) {
        if (*c == '$' || *c == '^') buf[n++] = *c;
        else if (*c == ' ' && c > raw && c[-1] != ' ' && c[1] != ' ') buf[n++] = ' ';
        buf[n++] = *c;
    }
    buf[n] = 0;
    float x0 = align == ALIGN_CENTER ? x - 160 : align == ALIGN_RIGHT ? x - 320 : x;
    rdpq_text_print(&(rdpq_textparms_t){ .style_id = style, .width = align == ALIGN_LEFT ? 0 : 320, .align = align, .disable_aa_fix = true },
                    font, x0, y, buf);
    mode = M_NONE;
}

// How wide a line of text is in a font, so a box can grow to hold it.
static float txt_w(int font, const char *s0) {
    char s[192];   // the same wider spaces as txt()
    size_t m = 0;
    for (const char *c = s0; *c && m < sizeof s - 2; c++) {
        if (*c == ' ' && c > s0 && c[-1] != ' ' && c[1] != ' ') s[m++] = ' ';
        s[m++] = *c;
    }
    s[m] = 0;
    int n = (int)m;
    rdpq_paragraph_t *pg = rdpq_paragraph_build(&(rdpq_textparms_t){ .disable_aa_fix = true }, font, s, &n);
    float w = pg ? pg->bbox.x1 - pg->bbox.x0 : (float)m * 6;
    if (pg) rdpq_paragraph_free(pg);
    return w;
}
// A banner: a dark box with a bright rim, sized to its words and centred, with the words in a light colour.
// (Dark text on bright yellow blurs into a blob on a TV, so bright boxes are only ever a rim.)
static void banner(int font, int style, uint32_t rim, float y, float minW, float h, float textY, const char *s) {
    float w = fminf(W - 6, fmaxf(minW, txt_w(font, s) + 20)), x = (W - w) / 2;
    rect(x, y, w, h, rim, 1);
    rect(x + 2, y + 2, w - 4, h - 4, 0x1B1426, 1);
    txt(font, style, W / 2, textY, ALIGN_CENTER, "%s", s);
}

// A line of help at the bottom or top of a game: light words on a dark strip, readable on anything.
static void hint(float y, const char *s) {
    float w = txt_w(F_SMALL, s) + 14;
    dark_panel((W - w) / 2, y - 9, w, 13);
    txt(F_SMALL, ST_CREAM, W / 2, y, ALIGN_CENTER, "%s", s);
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
    { float w = fminf(W - 6, fmaxf(260, txt_w(F_MED, cap) + 20)); panel((W - w) / 2, 210, w, 22, 0x1B1426, 0.86f * ca); }
    if (ca > 0.5f) txt(F_MED, ST_CREAM, W / 2, 225, ALIGN_CENTER, "%s", cap);
    if (t > 14.4f) rect(0, 0, W, H, 0xFFF8E6, clampf((t - 14.4f) / 0.8f, 0, 1));
    if ((int)(t * 2) % 2 && t > 1) txt(F_SMALL, ST_PALE, W - 8, 14, ALIGN_RIGHT, "START to skip");
}

// ---------------------------------------------------------------------------- the title and select screens

static void draw_title(const ck_game *G) {
    blit(SPR_TITLE, 0, 0, 0);
    if ((int)(G->t * 1.6f) % 2 == 0) txt(F_MED, ST_CREAM, W / 2, 230, ALIGN_CENTER, "PRESS START");
}

static void hero_front(int hero, int col, int frame, float x, float y, float scale, bool flip, const uint8_t *wear, float t) {
    blit_o(hero_sprite(hero, col), frame, x, y, skin_o((bopt){ .sx = scale, .sy = scale, .flip = flip }, col, t));
    hero_acc(wear, hero, frame, x, y, scale, flip, t);
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
        int hero = G->prog.hero[i], col = skin_col(G, i);
        int frame = ready ? 25 + ((int)(G->t * 4) % 2) : (int)(G->t / (hero == HERO_ROBOT ? 0.52f : 0.3f)) % 4;
        hero_front(hero, col, frame, x + w / 2, y + 120, 2.6f, false, wear_of(G, i), G->t + i);
        txt(F_MED, ST_CREAM, x + w / 2, y + 138, ALIGN_CENTER, hero == HERO_ROBOT ? "ROBOT" : "ROBOT OSTRICH");
        txt(F_SMALL, ST_PALE, x + w / 2, y + 151, ALIGN_CENTER, "< > hero   ^ v %s", ck_col_names[col]);
        if (ready) { rect(x + 30, y + 22, w - 60, 16, 0x49D17A, 1); txt(F_MED, ST_WHITE, x + w / 2, y + 34, ALIGN_CENTER, "READY!"); }
    }
    if ((int)(G->t * 2) % 2) txt(F_MED, ST_CREAM, W / 2, 220, ALIGN_CENTER, "A when ready  -  START to go");
}

// ---------------------------------------------------------------------------- the world map

// Each world's road colours (road, roadEdge, roadDot, roadLocked, roadLockedEdge in its web theme).
typedef struct { uint32_t fill, edge, dot, lockFill, lockEdge; float lockFillA, lockEdgeA; } road_cols;
static const road_cols ROADS[CK_THEME_COUNT] = {
    { 0xF2D7A6, 0x7A4A2C, 0xD9B77F, 0xFFF0D7, 0x3C1E14, 0.35f, 0.25f },   // junkyard
    { 0xCFD4DE, 0x2A2F3C, 0x9AA1B0, 0xDCE4F0, 0x0A0C14, 0.35f, 0.35f },   // factory
    { 0xE6D6BD, 0x4A3B45, 0xC4AE8E, 0xFFF0D7, 0x3C1E14, 0.35f, 0.25f },   // mountain
    { 0xCDEEFF, 0x3D4A66, 0x93C3E6, 0xCDEEFF, 0x141828, 0.30f, 0.35f },   // moon
};
static int theme_of(const ck_game *G) { return G->world.themeId >= 0 && G->world.themeId < CK_THEME_COUNT ? G->world.themeId : 0; }

static void road(const ck_road *r, float upto, bool open, const road_cols *rc) {
    float cut = (upto < 0 ? 0 : upto > 1 ? 1 : upto) * r->total;
    for (int pass = 0; pass < (open ? 2 : 2); pass++) {
        float w = open ? (pass ? 7 : 10) : (pass ? 4 : 8);
        uint32_t col = open ? (pass ? rc->fill : rc->edge) : (pass ? rc->lockFill : rc->lockEdge);
        float alpha = open ? 1 : (pass ? rc->lockFillA : rc->lockEdgeA);
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
        if (fabsf(x2 - x) > fabsf(y2 - y)) rect(x - 0.5f, y - 3, 1, 6, rc->dot, 1);
        else rect(x - 3, y - 0.5f, 6, 1, rc->dot, 1);
    }
}

static void star_icon(float x, float y, float r, uint32_t col, float a) { shape(SPR_STAR, x, y, r, col, a); }

static void draw_map(const ck_game *G) {
    const ck_world *Wd = &G->world;
    const ck_map *M = &G->map;
    float t = M->now;
    int wi = G->worldNum >= 1 && G->worldNum <= CK_WORLDS ? G->worldNum - 1 : 0;
    const road_cols *rc = &ROADS[theme_of(G)];
    blit(SPR_MAP_W1 + wi, 0, 0, 0);
    for (int r = 0; r < Wd->nroads; r++) if (!ck_road_open(G, r)) road(&Wd->roads[r], 1, false, rc);
    for (int r = 0; r < Wd->nroads; r++) {
        if (!ck_road_open(G, r)) continue;
        float upto = 1;
        for (int k = 0; k < M->nreveal; k++) if (M->reveal[k].road == r) upto = (t - M->reveal[k].t0) / M->reveal[k].dur;
        road(&Wd->roads[r], upto, true, rc);
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
        if (nd->kind == CK_NODE_EXIT) { blit(SPR_MAP_EXIT_1 + wi, reach ? 0 : 1, nd->x, nd->y); continue; }
        if (nd->kind == CK_NODE_BOSS) blit(SPR_MAP_BOSS_1 + wi, beaten ? 1 : 0, nd->x, nd->y);
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
        // light text with a dark outline: dark text on the bright pads blurs on a TV
        else if (nd->kind == CK_NODE_BOSS) txt(F_MED, ST_CREAM, nd->x, nd->y + 4, ALIGN_CENTER, "!");
        else txt(F_MED, ST_CREAM, nd->x + 0.5f, nd->y + 4, ALIGN_CENTER, "%d", nd->num);
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
        float bx, by;
        ck_map_baby_pos(G, i, &bx, &by);   // a little train along the path walked
        baby(Wd->babies[i % 4], (int)(t * 3 + i) % 2, bx, by - 2 - hop, 0.032f / 0.06f);
    }
    int np = G->prog.joined[0] + G->prog.joined[1], k = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        int hero = G->prog.hero[s], col = skin_col(G, s);
        float sc = hero == HERO_ROBOT ? 0.062f / 0.074f : 0.95f * 0.062f / 0.071f, tt = t + k * 0.2f;
        if (M->walking) {
            float ox = np == 1 ? 0 : (k == 0 ? -5 : 4) * dir;
            float f = tt * (hero == HERO_ROBOT ? 11 : 10);
            int fr = 4 + ((int)(f / (2 * PI) * 8)) % 8;
            blit_o(hero_sprite(hero, col), fr, hx + ox, hy - (hero == HERO_OSTRICH), skin_o((bopt){ .sx = sc, .sy = sc, .flip = dir < 0 }, col, tt));
            hero_acc(wear_of(G, s), hero, fr, hx + ox, hy - (hero == HERO_OSTRICH), sc, dir < 0, tt);
        } else {
            float off = np == 1 ? 0 : (k == 0 ? -6 : 6);
            int fr = (int)(tt / (hero == HERO_ROBOT ? 0.52f : 0.3f)) % 4;
            blit_o(hero_sprite(hero, col), fr, hx + off, hy - (hero == HERO_OSTRICH), skin_o((bopt){ .sx = sc, .sy = sc }, col, tt));
            hero_acc(wear_of(G, s), hero, fr, hx + off, hy - (hero == HERO_OSTRICH), sc, false, tt);
        }
        k++;
    }
    // HUD
    char up[40], head[56];
    for (int i = 0; i < (int)sizeof up; i++) { char c = Wd->name[i]; up[i] = c >= 'a' && c <= 'z' ? c - 32 : c; if (!c) break; }
    up[sizeof up - 1] = 0;
    snprintf(head, sizeof head, "WORLD %d \xC2\xB7 %s", Wd->num, up);
    dark_panel(6, 6, fminf(222, fmaxf(150, txt_w(F_MED, head) + 14)), 15);
    txt(F_MED, ST_PLAIN, 12, 17, ALIGN_LEFT, "%s", head);
    dark_panel(232, 6, 82, 15);
    blit_o(SPR_BOLT, 0, 242, 13.5f, (bopt){ .sx = 4 / 4.2f, .sy = 4 / 4.2f });
    txt(F_MED, ST_PLAIN, 248, 17, ALIGN_LEFT, "x%d", M->bolts);
    blit_o(SPR_GEAR, 0, 280, 13.5f, (bopt){ .sx = 4.5f / 7, .sy = 4.5f / 7, .rot = -t * 0.7f });
    txt(F_MED, ST_PLAIN, 286, 17, ALIGN_LEFT, "%d/%d", M->gears, Wd->gears);
    int here = ck_map_here(G);
    float bw = 172;
    if (here >= 0) bw = fminf(W - 8, fmaxf(172, txt_w(F_MED, Wd->nodes[here].label) + 70));
    float bx = (W - bw) / 2;
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
            if (s[0]) { float w = txt_w(F_SMALL, s) + 16; dark_panel(6, 24, w, 14); txt(F_SMALL, ST_PLAIN, 6 + w / 2, 34, ALIGN_CENTER, "%s", s); }
        }
    } else txt(F_MED, ST_PLAIN, W / 2, 228, ALIGN_CENTER, "...");
    if (M->hasMsg) {
        float k2 = fminf(1, (t - M->msgT0) * 6);
        banner(F_MED, ST_YELLOW, 0xFFD23F, 30 - (1 - k2) * 8, 150, 18, 42 - (1 - k2) * 8, M->msg);
    }
    lives_hud(G, 4, H - 19);
}

// The parts shop, laid out like the web one: 48 items in 4 rows of 12 (the 36 accessories, the two
// heroes, 8 paint pots, the bubble gun and the super jetpack), a cursor per player, and each hero
// trying on what their cursor is on below. Text is light on dark (dark on yellow blurs on a TV).
#define SHOP_COLS 12
#define SHOP_TW 22
#define SHOP_TH 28
static float shop_x(int a) { return 16 + (a % SHOP_COLS) * 24; }
static float shop_y(int a) { return 36 + (a / SHOP_COLS) * 30; }
static void outline(float x, float y, float w, float h, float th, uint32_t hex) {
    rect(x, y, w, th, hex, 1); rect(x, y + h - th, w, th, hex, 1);
    rect(x, y + th, th, h - th * 2, hex, 1); rect(x + w - th, y + th, th, h - th * 2, hex, 1);
}
// an item drawn small on a grey stand-in head, body or pair of feet (from the robot's front view)
static void acc_icon(int a, float x, float y, float t) {
    static const acc_pt AT = { 0, 0, 0, 1, 1 };
    const float k = 0.75f;
    int slot = ck_acc[a].slot;
    float hx = x + SHOP_TW / 2 - 1;
    y -= 3;   // leave the bottom-right corner for the price
    switch (slot) {
    case SLOT_HEAD: rect(hx - 9 * k, y + 17 * k + 4, 18 * k, 12 * k, 0x9AA1B0, 1); acc_piece(a, 0, &AT, hx, y + 17 * k + 4, 1.25f * k, false, t, 255, 255); break;
    case SLOT_FACE: rect(hx - 12 * k, y + 6, 24 * k, 20 * k, 0x9AA1B0, 1); acc_piece(a, 0, &AT, hx, y + 6 - 1.2f * k, 1.6f * k, false, t, 255, 255); break;
    case SLOT_NECK:
        rect(hx - 11 * k, y + 4, 22 * k, 13 * k, 0x9AA1B0, 1); rect(hx - 5, y + 8, 2, 2, 0x2B2230, 1); rect(hx + 3, y + 8, 2, 2, 0x2B2230, 1);
        acc_piece(a, 0, &AT, hx, y + 4 + 13 * k - 13.3f * 1.3f * k, 1.3f * k, false, t, 255, 255); break;
    case SLOT_BODY: rect(hx - 9 * k, y + 5, 18 * k, 19 * k, 0x9AA1B0, 1); acc_piece(a, 0, &AT, hx, y + 5 + 9.5f * k, 1.65f * k, false, t, 255, 255); break;
    default: acc_piece(a, 0, &AT, hx - 5, y + 18, 2.2f * k, false, t, 255, 255); acc_piece(a, 0, &AT, hx + 5, y + 18, 2.2f * k, false, t, 255, 255); break;
    }
}
// the heroes, paint pots, bubble gun and super jetpack
static void item_icon(int a, float x, float y, float t, int col) {
    if (a < ACC_COUNT) { acc_icon(a, x, y, t); return; }
    float hx = x + SHOP_TW / 2 - 1;
    y -= 3;
    if (a == SHOP_ROBOT || a == SHOP_OSTRICH)   // small white heroes, tinted (a whole hero sheet is too big to keep for an icon)
        blit_o(SPR_SHOP_HERO, a == SHOP_OSTRICH, hx, y + 28, skin_o((bopt){ .tinted = true, .tint = hexc(col < CK_NCOLS ? ck_cols[col] : 0xFFFFFF, 255) }, col, t));
    else if (a == SHOP_GUN) {
        rect(hx - 9, y + 11, 14, 7, 0x9AA1B0, 1); rect(hx - 7, y + 16, 5, 8, 0x5D566A, 1); rect(hx + 4, y + 12.5f, 4, 4, 0x5D566A, 1);
        shape(SPR_DOT, hx + 7, y + 7, 3.5f, 0xBFF0FF, 0.5f); shape(SPR_DOT, hx + 2, y + 4, 2.2f, 0xBFF0FF, 0.5f); shape(SPR_DOT, hx + 6, y + 6, 1, 0xFFFFFF, 1);
    } else if (a == SHOP_JET) {
        rect(hx - 7, y + 4, 14, 16, 0xE0564F, 1); rect(hx - 5, y + 6, 4, 12, 0xFFD23F, 1); rect(hx + 1, y + 6, 4, 12, 0xFFD23F, 1);
        float f = 3 + 2 * fabsf(sinf(t * 18));
        rect(hx - 5, y + 20, 4, f, 0xFF8A3D, 1); rect(hx + 1, y + 20, 4, f + 1 - 2 * fabsf(sinf(t * 15)), 0xFF8A3D, 1);
    } else {   // a pot of paint (the rainbow one striped)
        rect(hx - 7, y + 9, 14, 15, 0x9AA1B0, 1);
        if (a == SHOP_RAINBOW) for (int k = 0; k < 6; k++) rect(hx - 7 + k * 14 / 6.0f, y + 9, 14 / 6.0f + 0.2f, 6, ck_cols[(k + (int)(t * 6)) % 6], 1);
        else rect(hx - 7, y + 9, 14, 6, ck_cols[a - SHOP_PAINT0], 1);
        rect(hx - 8, y + 7, 16, 3, 0x5D566A, 1); rect(hx - 7, y + 2, 1.5f, 6, 0x5D566A, 1); rect(hx + 5.5f, y + 2, 1.5f, 6, 0x5D566A, 1); rect(hx - 6, y + 1.5f, 12, 1.5f, 0x5D566A, 1);
    }
}
static bool shop_worn(const ck_game *G, int s, int a) {
    const ck_closet *c = &G->saves.closet;
    if (a < ACC_COUNT) return c->wear[s][ck_acc[a].slot] == a + 1;
    if (a == SHOP_ROBOT) return G->prog.hero[s] == HERO_ROBOT;
    if (a == SHOP_OSTRICH) return G->prog.hero[s] == HERO_OSTRICH;
    if (a == SHOP_GUN || a == SHOP_JET) return ck_shop_owns(G, a);
    if (a == SHOP_RAINBOW) return ck_rainbow_skin(G, s);
    return !ck_rainbow_skin(G, s) && G->prog.col[s] == a - SHOP_PAINT0;
}
static void draw_shop(const ck_game *G) {
    static const uint32_t tint[SLOT_COUNT + 1] = { 0xFFD23F, 0x5AD1FF, 0xFF8FC7, 0xC28BFF, 0x49D17A, 0xFFB347 };
    const ck_closet *c = &G->saves.closet;
    float t = G->t;
    int first = G->prog.joined[0] ? 0 : 1;
    draw_map(G);
    rect(0, 0, W, H, 0x0F0A19, 0.65f);
    panel(8, 6, W - 16, H - 12, 0x3B2A5C, 1);
    txt(F_BIG, ST_YELLOW, 20, 26, ALIGN_LEFT, "PARTS SHOP");
    dark_panel(W - 86, 12, 70, 18);
    blit_o(SPR_BOLT, 0, W - 74, 21, (bopt){ .sx = 5 / 4.2f, .sy = 5 / 4.2f, .rot = -t });
    txt(F_MED, ST_PLAIN, W - 64, 26, ALIGN_LEFT, "x%d", G->prog.bolts);
    for (int a = 0; a < SHOP_COUNT; a++) {
        float x = shop_x(a), y = shop_y(a);
        int cost = ck_shop_cost(a);
        bool own = ck_shop_owns(G, a), worn = false;
        for (int s = 0; s < 2; s++) if (G->prog.joined[s] && shop_worn(G, s, a)) worn = true;
        rect(x, y, SHOP_TW, SHOP_TH, worn ? 0x49D17A : tint[a < ACC_COUNT ? ck_acc[a].slot : SLOT_COUNT], worn ? 0.4f : 0.14f);
        item_icon(a, x, y, t, skin_col(G, first));
        // owned: a green dot; for sale: the price in the corner, green if you have the bolts, red if not
        if (own && cost) shape(SPR_DOT, x + SHOP_TW - 3.5f, y + 3.5f, 3, 0x49D17A, 1);
        else if (!own) txt(F_SMALL, G->prog.bolts >= cost ? ST_GREEN : ST_RED, x + SHOP_TW - 1, y + SHOP_TH - 1, ALIGN_RIGHT, "%d", cost);
    }
    int np = G->prog.joined[0] + G->prog.joined[1], j = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        int a = G->shopCur[s];
        if (a < 0 || a >= SHOP_COUNT) a = 0;
        float x = shop_x(a), y = shop_y(a), o = j * 2;
        uint32_t pc = s ? 0x8FD3FF : 0xFFD23F;
        outline(x - 2 - o, y - 2 - o, SHOP_TW + 4 + o * 2, SHOP_TH + 4 + o * 2, 2, pc);
        if (np > 1) txt(F_SMALL, s ? ST_BLUE : ST_YELLOW, x + (j ? SHOP_TW - 3 : 3), y - 3 - o, ALIGN_CENTER, "P%d", s + 1);
        // the hero trying it on, over what they already wear (or the other hero, or a new paint)
        uint8_t tryOn[SLOT_COUNT];
        memcpy(tryOn, c->wear[s], sizeof tryOn);
        if (a < ACC_COUNT) tryOn[ck_acc[a].slot] = (uint8_t)(a + 1);
        int hero = a == SHOP_ROBOT ? HERO_ROBOT : a == SHOP_OSTRICH ? HERO_OSTRICH : G->prog.hero[s];
        int col = a >= SHOP_PAINT0 && a < SHOP_PAINT0 + CK_NCOLS ? a - SHOP_PAINT0 : a == SHOP_RAINBOW ? SKIN_RAINBOW : skin_col(G, s);
        float hx = np > 1 ? 40 + j * 160 : 120, sc = hero == HERO_ROBOT ? 0.11f / 0.074f : 0.1f / 0.071f, tt = t + j;
        rect(hx - 22, 228, 44, 4, 0xFFFFFF, 0.12f);
        int fr = (int)(tt / (hero == HERO_ROBOT ? 0.52f : 0.3f)) % 4;
        hero_front(hero, col, fr, hx, 230, sc, false, tryOn, tt);
        // the words go beside the hero, so they never cover what it is trying on
        float tx = hx + 24;
        char name[32];
        txt(F_MED, s ? ST_BLUE : ST_YELLOW, tx, 200, ALIGN_LEFT, "%s", ck_shop_name(a, name, sizeof name));
        int cost = ck_shop_cost(a);
        if (!ck_shop_owns(G, a)) {
            char buy[24];
            snprintf(buy, sizeof buy, "A buy  %d", cost);
            txt(F_SMALL, G->prog.bolts >= cost ? ST_PLAIN : ST_RED, tx, 213, ALIGN_LEFT, "%s", buy);
            blit_o(SPR_BOLT, 0, tx + txt_w(F_SMALL, buy) + 5, 210, (bopt){ .sx = 3 / 4.2f, .sy = 3 / 4.2f });
        } else {
            const char *how = a < ACC_COUNT ? (shop_worn(G, s, a) ? "A take off" : "A wear")
                            : a == SHOP_GUN ? "Z shoots in a level" : a == SHOP_JET ? "Hold A to glide" : "A use";
            txt(F_SMALL, ST_PLAIN, tx, 213, ALIGN_LEFT, "%s", how);
        }
        j++;
    }
    if (G->shopMsgT > 0) banner(F_MED, ST_YELLOW, 0xFFD23F, 10, 120, 20, 24, G->shopMsg);
    txt(F_SMALL, ST_GREY, W - 20, H - 12, ALIGN_RIGHT, "B leave");
}

// START on the map: keep playing, change heroes, let player 2 go, or start a new game (asked twice).
static void draw_menu(const ck_game *G) {
    static const char *names[4] = { [MENU_KEEP] = "Keep playing", [MENU_HEROES] = "Change heroes", [MENU_DROP] = "Player 2 leaves", [MENU_NEW] = "New game" };
    static const char *sure[2] = { "No, keep my game", "Yes, start over" };
    int items[4], ni = ck_menu_items(G, items);
    draw_map(G);
    rect(0, 0, W, H, 0x0F0A19, 0.6f);
    bool ask = G->menuAsk;
    int n = ask ? 2 : ni;
    float h = ask ? 120 : 38 + ni * 22, y0 = (H - h) / 2;
    panel(70, y0, 180, h, 0x3B2A5C, 1);
    txt(F_BIG, ST_YELLOW, W / 2, y0 + 22, ALIGN_CENTER, ask ? "START OVER?" : "MENU");
    if (ask) {
        txt(F_SMALL, ST_PALE, W / 2, y0 + 38, ALIGN_CENTER, "Every world, bolt, star and hat");
        txt(F_SMALL, ST_PALE, W / 2, y0 + 48, ALIGN_CENTER, "goes back to the start.");
    }
    for (int k = 0; k < n; k++) {
        float y = y0 + (ask ? 58 : 34) + k * 22;
        bool on = k == G->menuPick;
        if (on) { rect(84, y, 152, 18, 0xFFD23F, 1); rect(86, y + 2, 148, 14, 0x1B1426, 1); }
        else rect(84, y, 152, 18, 0xFFFFFF, 0.08f);
        txt(F_MED, on ? ST_YELLOW : ST_CREAM, W / 2, y + 13, ALIGN_CENTER, "%s", ask ? sure[k] : names[items[k]]);
    }
    txt(F_SMALL, ST_GREY, W / 2, y0 + h - 6, ALIGN_CENTER, "^ v choose   A ok   B back");
}

// ---------------------------------------------------------------------------- a level

static bool solid(char c) { return c == '#' || c == '=' || c == 'X' || c == 'D' || c == '<' || c == '>'; }
static char tile_at(const ck_level *lv, int tx, int ty) {
    if (tx < 0 || tx >= lv->w) return '#';
    if (ty < 0 || ty >= lv->h) return '.';
    return lv->tiles[ty][tx];
}

static void draw_tiles(const ck_level *lv, float cam, int theme) {
    const int TILES = SPR_TILES_0 + theme;
    const int beltK = (lv->frame / 4) % 4;
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
            blit(TILES, (top ? 0 : 4) + hash % 4, x, y);
            if (!solid(tile_at(lv, tx - 1, ty)) && tx > 0) rect(x, y, 2, TS, 0, 0.15f);
            if (!solid(tile_at(lv, tx + 1, ty)) && tx < lv->w - 1) rect(x + TS - 2, y, 2, TS, 0, 0.2f);
        } else if (t == '=') blit(TILES, 8, x, y);
        else if (t == '-') blit(TILES, 9, x, y);
        else if (t == '>' || t == '<') blit(TILES, 14 + (t == '>' ? beltK : 3 - beltK), x, y);
        else if (t == 'X') {
            char c = lv->contents[ty][tx];
            bool prize = c && c != 'o';
            blit(TILES, prize ? 11 : 10, x, y);
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
        blit(TILES, ty % 3 == 0 ? 13 : 12, roundf(tx * TS - cam), ty * TS + drop + shakeY);
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
    float sh = lv->boss.shake > 0 ? sinf(lv->boss.shake * 1.7f) * 2 : 0, tx = 286 + sh;
    blit(SPR_CRANE, 0, sh, shakeY);
    blit(SPR_CAB, lv->boss.ouch > 0 ? 1 : lv->boss.fixed ? 2 : 0, tx, shakeY);
    if (!lv->boss.fixed && (int)(lv->t * 8) % 3 == 0) {
        line(tx + 18, 4 + shakeY, tx + 22, 9 + shakeY, 1, 0xBFF6FF, 1);
        line(tx + 22, 9 + shakeY, tx + 19, 10 + shakeY, 1, 0xBFF6FF, 1);
        line(tx + 19, 10 + shakeY, tx + 24, 15 + shakeY, 1, 0xBFF6FF, 1);
    }
    float mx = lv->boss.mx ? lv->boss.mx : 160, my = lv->boss.my ? lv->boss.my : 164;
    line(160, 24 + shakeY, mx, my - 8 + shakeY, 1.5f, 0x3A3346, 1);
    blit_o(SPR_MAGNET, 0, mx, my + shakeY, (bopt){ .rot = lv->boss.ang });
    if (!lv->boss.fixed) { float k = fmodf(lv->t * 1.5f, 1); shape(SPR_RING, mx, my + shakeY, 12 + k * 8, 0xFF6B6B, 0.5f * (1 - k)); }
}

// A little crackle of glitch beside a boss that is not fixed yet.
static void zap(const ck_level *lv, float x, float y) {
    if (lv->boss.fixed || (int)(lv->t * 8) % 3) return;
    line(x, y, x + 4, y + 5, 1, 0xBFF6FF, 1);
    line(x + 4, y + 5, x + 1, y + 6, 1, 0xBFF6FF, 1);
    line(x + 1, y + 6, x + 6, y + 11, 1, 0xBFF6FF, 1);
}

// drawBoss in web/game.js.
static void draw_boss(const ck_level *lv) {
    if (!lv->hasBoss) return;
    int face = lv->boss.ouch > 0 ? 1 : lv->boss.fixed ? 2 : (lv->boss.type == BOSS_BOULDER && lv->boss.state == BS_DIZZY) ? 3 : 0;
    float bx = lv->boss.x, by = lv->boss.y + shakeY;
    switch (lv->boss.type) {
    case BOSS_STAMPER: {
        // the rail it hangs from, and its piston
        rect(24, 14 + shakeY, 272, 6, 0x3A3346, 1); rect(24, 14 + shakeY, 272, 2, 0x5D6378, 1);
        rect(bx - 12, 10 + shakeY, 24, 12, 0x7D8397, 1);
        float len = fmaxf(0, lv->boss.y - 20);
        rect(bx - 3, 20 + shakeY, 6, len, 0x9AA1B0, 1); rect(bx + 1, 20 + shakeY, 2, len, 0x5D6378, 1);
        float sh = lv->boss.state == BS_WARN ? sinf(lv->frame * 1.8f) * 1.5f : 0;
        blit(SPR_BOSS_STAMPER, face, bx + sh, by);
        if (lv->boss.state == BS_WARN && lv->frame % 8 < 4) rect(bx - 24, lv->boss.low - 2 + shakeY, 48, 2, 0xFF6B6B, 0.5f);
        zap(lv, bx + 26, by + 2);
        break;
    }
    case BOSS_BOULDER:
        blit_o(SPR_DOT, 0, bx, 12 * TS + shakeY, (bopt){ .sx = 18 / 7.5f, .sy = 4 / 7.5f, .tinted = true, .tint = hexc(0x1E141E, 77) });
        blit_o(SPR_BOSS_ROCK, 0, bx, by, (bopt){ .rot = lv->boss.rot });
        blit(SPR_BOSS_ROCKFACE, face, bx, by);
        if (face == 3)
            for (int k = 0; k < 3; k++) {
                float a = lv->t * 5 + k * PI * 2 / 3;
                star_icon(bx + cosf(a) * 14, by - 24 + sinf(a) * 3, 2.2f, 0xFFF6A0, 1);
            }
        zap(lv, bx + 16, by - 22);
        break;
    case BOSS_HAULER: {
        float bottom = 12 * TS + shakeY, top = by + 14;
        if (lv->boss.state == BS_WARN && lv->frame % 6 < 3) rect(bx - 1, top, 2, bottom - top, 0xBFF6FF, 0.7f);
        if (lv->boss.state == BS_BEAM) {
            float w = 24 + sinf(lv->frame * 0.6f) * 2;
            rect(bx - w / 2, top, w, bottom - top, 0xBFF6FF, 0.35f);
            rect(bx - 4, top, 8, bottom - top, 0xBFF6FF, 0.7f);
        }
        float tilt = lv->boss.fixed ? 0 : sinf(lv->t * 2.2f) * 0.08f;
        blit_o(SPR_BOSS_HAULER, face, bx, by, (bopt){ .rot = tilt });
        zap(lv, bx + 30, by - 14);
        break;
    }
    default: draw_crane(lv); break;
    }
    // hits so far
    for (int i = 0; i < 3; i++) shape(SPR_DOT, 140 + i * 20, 220, 5, i < lv->boss.hits ? 0x49D17A : 0xFFFFFF, i < lv->boss.hits ? 1 : 0.25f);
}

static void draw_player(const ck_game *G, const ck_player *p, float cam, int nplayers) {
    const ck_level *lv = &G->lv;
    if (p->inv > 0 && p->wob <= 0 && !p->dead && !p->bubble && (p->inv / 4) % 2 == 0 && lv->clearT < 0) return;
    float fx = p->x + p->w / 2 - cam, fy = p->y + p->h + shakeY, t = lv->t + p->i * 0.7f;
    int skin = ck_rainbow_skin(G, p->slot) ? SKIN_RAINBOW : p->col, sheet = hero_sprite(p->hero, skin);
    bool R = p->hero == HERO_ROBOT;
    const uint8_t *wear = wear_of(G, p->slot);
    if (p->dead) { blit(sheet, 27, fx, fy); hero_acc(wear, p->hero, 27, fx, fy, 1, false, t); return; }
    if (p->bubble) {
        blit(sheet, 28, fx, fy);
        hero_acc(wear, p->hero, 28, fx, fy, 1, false, t);
        bubble_at(fx, p->y + p->h / 2 + shakeY, 21);
        if (G->prog.lives <= 0) txt(F_SMALL, ST_RED, fx, p->y + p->h / 2 + shakeY - 25, ALIGN_CENTER, "OUT");   // out of lives: riding along
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
        if (R) fr = p->jetT > 0 || p->flapping ? 13 + (int)(t * 20) % 2 : 12;   // jetpack for the double jump and the glide
        else fr = p->flapping || p->jetT > 0 ? 13 + ((int)(t * 13 / (2 * PI) * 4)) % 4 : 12;
    } else if (fabsf(p->vx) > 0.25f) {
        float f = R ? p->anim * 1.6f * 11 : p->anim * 1.6f / 1.1f * 10;
        fr = 4 + ((int)(f / (2 * PI) * 8)) % 8;
    } else { fr = (int)(t / (R ? 0.52f : 0.3f)) % 4; side = false; }
    bopt o = { .sx = mega, .sy = mega, .flip = side && p->face < 0 };
    if (rainbow) { o.tinted = true; o.tint = hexc(ck_cols[(int)(t * 12 + p->i * 3) % 8] | 0x606060, 255); }
    else o = skin_o(o, skin, t + p->i);
    blit_o(sheet, fr, fx, fy, o);
    hero_acc(wear, p->hero, fr, fx, fy, mega, o.flip, t);
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
        int kinds[6], n = 0;
        for (int s = 0; s < p->nstack; s++) kinds[n++] = p->stack[s];
        if (p->rainbowT > 0) kinds[n++] = PW_RAINBOW;
        if (p->magnetT > 0) kinds[n++] = PW_MAGNET;
        if (ck_gun_on(G, p->slot)) kinds[n++] = PW_MAGNET + 1;   // the bubble gun's icon follows the magnet's
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
        banner(F_MED, ST_YELLOW, 0xFFD23F, 150, 140, 22, 165, "RACE THE CLOCK!");
    }
    if (lv->timeUp && lv->dyingT > 10) banner(F_BIG, ST_WHITE, 0xFF8A8A, 60, 120, 22, 76, "TIME'S UP!");
    if (lv->toastT > 0 && lv->toast[0]) banner(F_BIG, ST_YELLOW, 0x1B1426, 35, 0, 21, 51, lv->toast);
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
        { float w = fminf(W - 6, fmaxf(180, txt_w(F_MED, lv->retry ? "TRY AGAIN!" : def->name) + 20)); dark_panel((W - w) / 2, 92, w, lv->retry ? 44 : 30); }
        txt(F_MED, ST_YELLOW, W / 2, 112, ALIGN_CENTER, "%s", lv->retry ? "TRY AGAIN!" : def->name);
        if (lv->retry) txt(F_SMALL, ST_PLAIN, W / 2, 128, ALIGN_CENTER, "%d %s left", G->prog.lives, G->prog.lives > 1 ? "tries" : "try");
    }
    if (lv->dyingT > 40) { dark_panel(W / 2 - 80, 96, 160, 34); txt(F_BIG, ST_YELLOW, W / 2, 118, ALIGN_CENTER, G->prog.lives > 0 ? "OOPS!" : "OUT OF TRIES"); }
    if (lv->hasBoss && !lv->boss.fixed && lv->frame < 260) banner(F_MED, ST_CREAM, 0x1B1426, 127, 0, 18, 140, "Bounce on the red buttons!");
    if (lv->hasShower && lv->frame < 180) banner(F_MED, ST_CREAM, 0x1B1426, 127, 0, 18, 140, "Catch the bolts!");
    if (lv->clearT >= 0) {
        float k = fminf(1, lv->clearT / 20.0f);
        banner(F_HUGE, ST_YELLOW, 0xFFD23F, 60 - (1 - k) * 30, 160, 34, 84 - (1 - k) * 30, def->kind == CK_KIND_BOSS ? "FIXED!" : def->kind == CK_KIND_BONUS ? "BONUS!" : "RESCUED!");
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
            if (R->record && lv->clearT > 140) banner(F_MED, ST_WHITE, 0xFF8FC7, 183, 116, 18, 197, "NEW RECORD!");
        }
    }
}

static void draw_level(const ck_game *G) {
    const ck_level *lv = &G->lv;
    float cam = floorf(lv->camX);
    int theme = theme_of(G);
    int far = (lv->def->kind == CK_KIND_BOSS ? SPR_BG_FAR_BOSS_0 : SPR_BG_FAR_0) + theme;
    float off = fmodf(lv->camX * 0.2f, 640);
    blit(far, 0, -off, 0);
    if (off > 320) blit(far, 0, 640 - off, 0);
    float mo = fmodf(lv->camX * 0.5f, 640);
    blit(SPR_BG_MID_0 + theme, 0, -mo, 0);
    if (mo > 320) blit(SPR_BG_MID_0 + theme, 0, 640 - mo, 0);
    shakeY = lv->shake > 0 ? sinf(lv->shake * 2.3f) * 2 : 0;
    draw_boss(lv);
    draw_tiles(lv, cam, theme);
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
        int jf = 29 + (int)(J->t * 14) % 2;
        int skin = ck_rainbow_skin(G, src->slot) ? SKIN_RAINBOW : src->col;
        blit_o(hero_sprite(src->hero, skin), jf, p->x + p->w / 2, p->y + p->h, skin_o((bopt){ 0 }, skin, J->t + k));
        hero_acc(wear_of(G, src->slot), src->hero, jf, p->x + p->w / 2, p->y + p->h, 1, false, J->t);
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

// ---------------------------------------------------------------------------- the films between worlds

// cutScene().draw in web/game.js; where everyone is comes from ck_cut_cast().
static void draw_cut(const ck_game *G) {
    ck_cut_view v;
    ck_cut_cast(G, &v);
    int k = G->cut < 1 ? 1 : G->cut > CK_WORLDS ? CK_WORLDS : G->cut;
    float t = G->t;
    blit(SPR_CUT_BG_1 + k - 1, 0, 0, 0);
    if (k == 1) {   // the roller door rises into the wall
        rdpq_set_scissor(CK_CUT_GATE_X, CK_CUT_GATE_Y, CK_CUT_GATE_X + CK_CUT_GATE_W, CK_CUT_GATE_Y + CK_CUT_GATE_H);
        blit(SPR_CUT_GATE, 0, CK_CUT_GATE_X, CK_CUT_GATE_Y - v.gate * CK_CUT_GATE_H);
        rdpq_set_scissor(0, 0, W, H);
    }
    if (k == 2) blit(SPR_CUT_CAGE, 0, CK_CUT_SHAFT_X, v.cageY);
    if (k == 3) {
        float x = CK_CUT_PAD_X + v.shake;
        if (v.flame) { float sc = v.bigFlame ? 1 : 0.55f; blit_o(SPR_CUT_FLAME, (int)(t * 14) % 2, x, v.rocketY, (bopt){ .sx = sc, .sy = sc }); }
        blit(SPR_CUT_ROCKET, 0, x, v.rocketY);
    }
    if (k == 4) blit(SPR_CUT_FLOAT, 0, v.floatX, 210);
    // babies behind, heroes in front (in the lift the babies stand at the front of the cage)
    for (int pass = 0; pass < 2; pass++) {
    bool babyPass = (pass == 0) != (k == 2);
    if (babyPass) {
    for (int j = 0; j < v.nbabies; j++) {
        const ck_cut_actor *c = &v.babies[j];
        if (c->a <= 0) continue;
        float hop = c->walk || k == 4 ? fabsf(sinf(t * (k == 4 ? 8 : 9) + j)) * (k == 4 ? 6 : 4) : 0;
        int fr = (k == 4 ? 2 : 0) + (int)(t * 3 + j) % 2;
        char name[32];
        snprintf(name, sizeof name, "rom:/baby_%06lX.sprite", (unsigned long)G->cutBabies[j]);
        int id = SPR_BABY_FF8FC7;
        for (int i = 0; i < SPR_COUNT; i++) if (!strcmp(SPR_INFO[i].file, name)) { id = i; break; }
        blit_o(id, fr, c->x, c->y - hop, (bopt){ .sx = 0.05f / 0.06f, .sy = 0.05f / 0.06f, .tinted = true, .tint = RGBA32(255, 255, 255, (int)(c->a * 255)) });
    }
    } else {
    int i = 0;
    for (int s = 0; s < 2 && i < v.nheroes; s++) {
        if (!G->prog.joined[s]) continue;
        const ck_cut_actor *c = &v.heroes[i];
        int hero = G->prog.hero[s], col = skin_col(G, s);
        float tt = t + i * 0.7f;
        i++;
        if (c->a <= 0) continue;
        int fr;
        if (c->walk) fr = 4 + (int)(tt * (hero == HERO_ROBOT ? 2.2f * 11 : 2 * 10) / (2 * PI) * 8) % 8;
        else if (k == 4 || (k == 2 && t > 9.5f)) fr = 25 + (int)(tt * 6) % 2;
        else fr = (int)(tt / (hero == HERO_ROBOT ? 0.52f : 0.3f)) % 4;
        blit_o(hero_sprite(hero, col), fr, c->x, c->y, skin_o((bopt){ .tinted = true, .tint = RGBA32(255, 255, 255, (int)(c->a * 255)) }, col, tt));
        hero_acc_a(wear_of(G, s), hero, fr, c->x, c->y, 1, false, tt, (int)(c->a * 255));
    }
    }
    }
    if (k == 2) blit(SPR_CUT_CAGE, 1, CK_CUT_SHAFT_X, v.cageY);
    particles(G, 0);
    // the caption strip
    if (v.caption && v.captionA > 0.2f) {
        float w = fminf(W - 6, fmaxf(280, txt_w(F_MED, v.caption) + 20));
        panel((W - w) / 2, 210, w, 22, 0x1B1426, 0.86f * v.captionA);
        txt(F_MED, v.gold ? ST_YELLOW : ST_CREAM, W / 2, 225, ALIGN_CENTER, "%s", v.caption);
    }
    if (v.black > 0) rect(0, 0, W, H, 0x000000, v.black);
    if (v.white > 0) rect(0, 0, W, H, 0xFFF8E6, v.white);
    if (t > 1 && (int)(t * 2) % 2) txt(F_SMALL, ST_PALE, W - 8, 14, ALIGN_RIGHT, "START to skip");
}

// ---------------------------------------------------------------------------- the house
// Mirrors houseScene and its games in web/game.js; the rooms are boxes, the furniture and the
// creatures are baked from web/lib/houseart.js.

// A hero in one of the house's poses: idle and cheer face the front, the rest are side frames.
enum { HP_IDLE, HP_RUN, HP_JUMP, HP_JET, HP_CHEER };
// A food from ck_foods at (x, y), sc times its size.
static void food_art(int kind, float x, float y, float sc) {
    if (kind < 0 || kind >= FOOD_COUNT) return;
    const ck_foodart *f = &ck_foods[kind];
    for (int i = 0; i < f->n; i++) {
        const ck_fshape *s = &f->s[i];
        if (s->kind == 0) shape(SPR_DOT, x + s->x * sc, y + s->y * sc, s->w * sc, s->col, 1);
        else rect(x + s->x * sc, y + s->y * sc, s->w * sc, s->h * sc, s->col, 1);
    }
}
static void house_hero(const ck_game *G, int slot, float x, float y, int pose, float t, int face) {
    int hero = G->prog.hero[slot], col = skin_col(G, slot);
    bool R = hero == HERO_ROBOT, side = pose == HP_RUN || pose == HP_JUMP || pose == HP_JET;
    int fr;
    switch (pose) {
    case HP_RUN: fr = 4 + (int)(t * (R ? 14 : 16)) % 8; break;
    case HP_JUMP: fr = 12; break;
    case HP_JET: fr = R ? 13 + (int)(t * 20) % 2 : 13 + (int)(t * 13 / (2 * PI) * 4) % 4; break;
    case HP_CHEER: fr = 25 + (int)(t * 6) % 2; break;
    default: fr = (int)(t / (R ? 0.52f : 0.3f)) % 4; break;
    }
    bool flip = side && face < 0;
    blit_o(hero_sprite(hero, col), fr, x, y, skin_o((bopt){ .flip = flip }, col, t));
    hero_acc(wear_of(G, slot), hero, fr, x, y, 1, flip, t);
}
static void cloud(float x, float y) { shape(SPR_DOT, x, y, 12, 0xFFFFFF, 1); shape(SPR_DOT, x + 12, y - 4, 14, 0xFFFFFF, 1); shape(SPR_DOT, x + 26, y, 11, 0xFFFFFF, 1); }
static void draw_rooms(const ck_game *G, float cam, float t) {
    for (int i = 0; i < CK_HOUSE_ROOMS; i++) {
        const ck_room *r = &ck_rooms[i];
        float a = r->x0 - cam, b = r->x1 - cam;
        if (b < 0 || a > W) continue;
        if (i == 0) {   // the yard: sky, clouds, grass and a fence
            rect(a, 0, b - a, CK_HOUSE_FLOOR, r->wall, 1);
            rect(a, 120, b - a, 80, r->stripe, 1);
            cloud(60 - cam, 40); cloud(200 - cam, 26);
            for (int x = 10; x < r->x1 - 20; x += 16) rect(x - cam, 170, 8, 30, 0xFFF6D8, 1);
            rect(a, 178, b - a - 20, 5, 0xFFF6D8, 1);
            rect(a, CK_HOUSE_FLOOR, b - a, H - CK_HOUSE_FLOOR, r->floor, 1);
            for (int x = 0; x < r->x1; x += 10) rect(x - cam, CK_HOUSE_FLOOR, 5, 3, 0x4FA856, 1);
            continue;
        }
        rect(a, CK_HOUSE_CEIL, b - a, CK_HOUSE_FLOOR - CK_HOUSE_CEIL, r->wall, 1);
        for (int x = r->x0 + 12; x < r->x1; x += 24) rect(x - cam, CK_HOUSE_CEIL, 10, CK_HOUSE_FLOOR - CK_HOUSE_CEIL, r->stripe, 1);
        rect(a, CK_HOUSE_FLOOR - 8, b - a, 8, 0x6F4630, 1);
        rect(a, CK_HOUSE_FLOOR, b - a, H - CK_HOUSE_FLOOR, r->floor, 1);
        for (int x = r->x0; x < r->x1; x += 20) rect(x - cam, CK_HOUSE_FLOOR, 1, H - CK_HOUSE_FLOOR, 0x000000, 0.12f);
        rect(a, 0, b - a, CK_HOUSE_CEIL, 0x5A3F2E, 1);
        float mid = (a + b) / 2, w = txt_w(F_SMALL, r->name) + 16;
        rect(mid - w / 2, 28, w, 15, 0x6F4630, 1);
        rect(mid - w / 2 + 2, 30, w - 4, 11, 0xC98A4B, 1);
        txt(F_SMALL, ST_CREAM, mid, 39, ALIGN_CENTER, "%s", r->name);
    }
    for (int i = 1; i < CK_HOUSE_ROOMS; i++) {
        float x = ck_rooms[i].x0 - cam - CK_HOUSE_PILLAR / 2;
        if (x < -10 || x > W) continue;
        rect(x, CK_HOUSE_CEIL, CK_HOUSE_PILLAR, CK_HOUSE_DOOR_TOP - CK_HOUSE_CEIL, 0x6F4630, 1);
        rect(x - 2, CK_HOUSE_DOOR_TOP - 4, CK_HOUSE_PILLAR + 4, 6, 0x5A3F2E, 1);
    }
    rect(CK_HOUSE_W - 8 - cam, CK_HOUSE_CEIL, 8, CK_HOUSE_FLOOR - CK_HOUSE_CEIL, 0x6F4630, 1);
    for (int i = 0; i < CK_HOUSE_DECOR; i++) { float x = ck_decors[i].x - cam; if (x > -60 && x < W + 60) blit(SPR_HOUSE_FURN, ck_decors[i].furn, x, CK_HOUSE_FLOOR); }
    for (int i = 0; i < CK_HOUSE_SPOTS; i++) {
        float x = ck_spots[i].x - cam;
        if (x < -60 || x > W + 60 || !ck_spot_here(G, i)) continue;
        blit(SPR_HOUSE_FURN, ck_spots[i].furn, x, CK_HOUSE_FLOOR);
        if (ck_spots[i].kind == SPOT_AQUA)   // fish swimming in the tank
            for (int k = 0; k < 3; k++) {
                float u = t * (0.3f + k * 0.1f) + k * 2;
                blit_o(SPR_HOUSE_FISH, k * 2 + (int)(t * 5) % 2, x + sinf(u) * 30, CK_HOUSE_FLOOR - 70 + k * 12, (bopt){ .sx = 0.7f, .sy = 0.7f, .flip = cosf(u) < 0 });
            }
    }
}
static void slot_picker(const ck_game *G, const char *title, const char *sub, int pick) {
    rect(0, 0, W, H, 0x0F0A19, 0.6f);
    panel(40, 46, 240, 150, 0x3B2A5C, 1);
    txt(F_BIG, ST_YELLOW, W / 2, 70, ALIGN_CENTER, "%s", title);
    txt(F_SMALL, ST_PALE, W / 2, 86, ALIGN_CENTER, "%s", sub);
    for (int k = 0; k < CK_SLOTS; k++) {
        float y = 96 + k * 28;
        bool on = k == pick;
        ck_slot_info s = ck_slot_summary(G, k);
        if (on) { rect(52, y, 216, 24, 0xFFD23F, 1); rect(54, y + 2, 212, 20, 0x1B1426, 1); }
        else rect(52, y, 216, 24, 0xFFFFFF, 0.08f);
        txt(F_MED, on ? ST_YELLOW : ST_CREAM, 62, y + 16, ALIGN_LEFT, "Slot %d", k + 1);
        if (s.used) txt(F_SMALL, ST_PLAIN, 258, y + 16, ALIGN_RIGHT, "World %d \xC2\xB7 %d bolts", s.world, s.bolts);
        else txt(F_SMALL, ST_GREY, 258, y + 16, ALIGN_RIGHT, "empty");
        if (k == G->slot) shape(SPR_DOT, 120, y + 12, 3, 0x49D17A, 1);
    }
    txt(F_SMALL, ST_GREY, W / 2, 188, ALIGN_CENTER, "^ v choose   A ok   B back");
}
static void draw_slots(const ck_game *G) {
    blit(SPR_TITLE, 0, 0, 0);
    slot_picker(G, "PICK A GAME", "Each slot is its own game", G->slotPick);
}
static void draw_house(const ck_game *G) {
    const ck_house *Hs = &G->house;
    float cam = Hs->cam, t = G->t;
    draw_rooms(G, cam, t);
    for (int i = 0; i < Hs->nbabies; i++) {
        const ck_hbaby *b = &Hs->babies[i];
        float x = b->x - cam;
        if (x < -20 || x > W + 20) continue;
        baby(b->hex, (int)(t * 3 + i) % 2, x, CK_HOUSE_FLOOR + b->y, 0.05f / 0.06f);
    }
    for (int i = 0; i < Hs->np; i++) {
        const ck_hhero *p = &Hs->ps[i];
        float x = p->x - cam, tt = t + p->slot * 0.7f;
        int pose = p->eat > 0 || Hs->mode == HM_SLEEP ? HP_CHEER : !p->ground ? HP_JUMP : fabsf(p->vx) > 0.3f ? HP_RUN : HP_IDLE;
        house_hero(G, p->slot, x, p->y, pose, tt, p->face);
        if (p->eat > 0) food_art(p->col, x + 6, p->y - 24, 0.4f + 0.6f * p->eat / 50.0f);   // shrinking as it is eaten
        else if (p->hold) food_art(p->hold - 1, x + p->face * 9, p->y - 20, 1);
    }
    for (int i = 0; i < Hs->nthrown; i++) food_art(Hs->thrown[i].kind, Hs->thrown[i].x - cam, Hs->thrown[i].y, 1);
    // what Down does where you stand
    int shown = -1;
    if (Hs->mode == HM_WALK)
        for (int i = 0; i < Hs->np; i++) {
            int k = ck_house_near(G, &Hs->ps[i]);
            if (k < 0 || k == shown) continue;
            shown = k;
            char s[32];
            if (ck_spots[k].kind == SPOT_BACK) snprintf(s, sizeof s, "A  Back to World %d", G->worldNum - 1);
            else snprintf(s, sizeof s, "A  %s", ck_spots[k].label);
            float w = txt_w(F_SMALL, s) + 14, x = ck_spots[k].x - cam;
            dark_panel(x - w / 2, 120, w, 15);
            txt(F_SMALL, ST_PLAIN, x, 131, ALIGN_CENTER, "%s", s);
        }
    particles(G, 0);
    if (cam < 40) txt(F_SMALL, ST_CREAM, 18 - cam, 186, ALIGN_LEFT, "< Map");
    lives_hud(G, 4, 4);
    if (Hs->msgT > 0) banner(F_MED, ST_YELLOW, 0xFFD23F, 26, 120, 20, 40, Hs->msg);
    if (Hs->mode == HM_SLEEP) {
        rect(0, 0, W, H, 0x0C081E, fminf(0.75f, Hs->sleepT * 0.6f));
        for (int k = 0; k < 3; k++) {
            float u = fmodf(Hs->sleepT * 0.8f + k / 3.0f, 1), sz = (8 + u * 8) / 12;
            blit_o(SPR_ZEE, 0, ck_spots[SPOT_BED].x - cam + 10 + u * 20, 150 - u * 50, (bopt){ .sx = sz, .sy = sz, .tinted = true, .tint = RGBA32(255, 246, 216, (int)((1 - u) * 255)) });
        }
    }
    if (Hs->mode == HM_SLOTS) slot_picker(G, "SLEEP AND SAVE", "Which save slot?", Hs->pick);
    if (Hs->mode == HM_FRIDGE) {   // the fridge door open: three shelves of food, a cursor each
        rect(0, 0, W, H, 0x0F0A19, 0.55f);
        rect(76, 14, 168, 212, 0xD9DEE6, 1);
        rect(82, 20, 156, 200, 0xF4F8FC, 1);
        for (int r = 0; r < 3; r++) rect(84, 80 + r * 62, 152, 4, 0xBFC8D4, 1);
        for (int k = 0; k < CK_FRIDGE_FOODS; k++) {
            float fx = 110 + (k % 3) * 50, fy = 56 + (k / 3) * 62;
            for (int j = 0; j < Hs->np; j++)
                if (Hs->fpick[j] == k) { shape(SPR_RING, fx, fy, 20 + j * 3, Hs->ps[j].slot ? 0x8FD3FF : 0xFFD23F, 1); }
            food_art(k, fx, fy, 3);
        }
        for (int j = 0; j < Hs->np; j++) {
            txt(F_SMALL, Hs->ps[j].slot ? ST_BLUE : ST_YELLOW, j ? 300 : 20, 228, j ? ALIGN_RIGHT : ALIGN_LEFT, "%s", ck_foods[Hs->fpick[j]].name);
        }
        hint(10, "Stick choose   A take it   START leave");
    }
    if (Hs->mode == HM_COOK) {   // the stove: a shelf of ingredients, the pan, the heat bar
        rect(0, 0, W, H, 0x0F0A19, 0.55f);
        rect(30, 24, 260, 50, 0x8A5A3C, 1);
        rect(34, 28, 252, 42, 0xC98A4B, 1);
        for (int k = 0; k < CK_COOK_INGR; k++) {
            float fx = 56 + k * 42, fy = 49;
            for (int j = 0; j < Hs->np; j++) if (Hs->cpick[j] == k && !Hs->dish) shape(SPR_RING, fx, fy, 17 + j * 3, Hs->ps[j].slot ? 0x8FD3FF : 0xFFD23F, 1);
            food_art(ck_cook_ingr[k], fx, fy, 2.4f);
        }
        rect(50, 150, 220, 80, 0x3A3F4A, 1);   // the stove top
        rect(56, 156, 208, 6, 0x5A5E6A, 1);
        shape(SPR_DOT, 160, 160, 34, Hs->heat > 0 && !Hs->dish ? 0xFF8A3D : 0x2B2833, 1);   // the burner glows while it cooks
        shape(SPR_DOT, 160, 132, 50, 0x5A5E6A, 1);   // the pan
        shape(SPR_DOT, 160, 132, 44, 0x2B2833, 1);
        rect(206, 128, 60, 8, 0x3A2618, 1);   // its handle
        if (Hs->dish) food_art(Hs->dish - 1, 160, 132, 6);
        else for (int i = 0; i < Hs->npan; i++) {
            float a = i * 1.57f + G->t * (Hs->heat > 0 ? 1.5f : 0);
            food_art(Hs->pan[i], 160 + cosf(a) * 18, 132 + sinf(a) * 14, 2.2f);
        }
        rect(60, 196, 200, 10, 0x1B1426, 1);   // heat
        rect(62, 198, 196 * Hs->heat / 100, 6, Hs->dish ? 0x49D17A : 0xFF8A3D, 1);
        if (Hs->dish) { banner(F_MED, ST_YELLOW, 0xFFD23F, 80, 120, 22, 95, Hs->dishName); hint(226, "A take it   START leave"); }
        else {
            txt(F_SMALL, ST_CREAM, W / 2, 222, ALIGN_CENTER, "%s", ck_foods[ck_cook_ingr[Hs->cpick[0]]].name);
            hint(14, Hs->npan ? "< > choose   A add   B stir   START leave" : "< > choose   A add to the pan   START leave");
        }
    }
    if (Hs->mode == HM_LETTER) {   // a big letter to read
        const ck_letter *L = &ck_letters[(Hs->letter + CK_LETTERS - 1) % CK_LETTERS];
        rect(0, 0, W, H, 0x0F0A19, 0.55f);
        rect(40, 34, 240, 172, 0xC9B79A, 1);
        rect(42, 36, 236, 168, 0xFFF6E6, 1);
        for (int r = 0; r < 6; r++) rect(56, 96 + r * 18, 208, 1, 0xD9C9AE, 1);
        shape(SPR_DOT, 252, 58, 12, 0xE04F4F, 1);   // a wax seal
        shape(SPR_STAR, 252, 58, 6, 0xFFD23F, 1);
        txt(F_MED, ST_RED, 58, 64, ALIGN_LEFT, "Dear heroes,");
        for (int r = 0; r < 3; r++) txt(F_MED, ST_CREAM, 60, 96 + r * 22, ALIGN_LEFT, "%s", L->lines[r]);
        txt(F_MED, ST_YELLOW, 262, 188, ALIGN_RIGHT, "Love, %s", L->from);
        hint(226, "START leave");
    }
}

// The paint studio: the wall in runs of one colour per row (few boxes for the RDP), the palette,
// and the heroes on their jetpacks with a dab of their colour on the brush hand.
static void draw_paint(const ck_game *G) {
    const ck_house *Hs = &G->house;
    float t = G->t;
    rect(0, 0, W, H, 0xC98A4B, 1);
    rect(0, CK_MURAL_Y, CK_MURAL_W * CK_MURAL_CELL, CK_MURAL_H * CK_MURAL_CELL, 0xFFF6E6, 1);
    for (int y = 0; y < CK_MURAL_H; y++) {
        const uint8_t *row = Hs->mural + y * CK_MURAL_W;
        int x = 0;
        while (x < CK_MURAL_W) {
            int v = row[x], e = x + 1;
            while (e < CK_MURAL_W && row[e] == v) e++;
            if (v) rect(x * CK_MURAL_CELL, CK_MURAL_Y + y * CK_MURAL_CELL, (e - x) * CK_MURAL_CELL, CK_MURAL_CELL, ck_mural_pal[v - 1], 1);
            x = e;
        }
    }
    rect(0, CK_MURAL_Y + CK_MURAL_H * CK_MURAL_CELL, W, H, 0x9C7552, 1);
    for (int k = 0; k < CK_MURAL_CHOICES; k++) {
        float x = 26 + k * 24, y = 228;
        if (k < CK_MURAL_PAL) shape(SPR_DOT, x, y, 7, ck_mural_pal[k], 1);
        else if (k == CK_MURAL_ERASER) { shape(SPR_DOT, x, y, 7, 0xFFF6E6, 1); txt(F_SMALL, ST_RED, x, y + 3, ALIGN_CENTER, "x"); }
        else { shape(SPR_DOT, x, y, 9, 0xE04F4F, 1); txt(F_SMALL, ST_WHITE, x, y + 3, ALIGN_CENTER, "ALL"); }
        for (int j = 0; j < Hs->np; j++) if (Hs->gs[j].col == k) shape(SPR_RING, x, y, 9 + j * 2, Hs->gs[j].slot ? 0x8FD3FF : 0xFFD23F, 1);
    }
    for (int i = 0; i < Hs->np; i++) {
        const ck_hhero *p = &Hs->gs[i];
        float tt = t + p->slot * 0.7f;
        if (p->jet) blit_o(SPR_CUT_FLAME, (int)(t * 14) % 2, p->x - p->face * 5, p->y - 8, (bopt){ .sx = 0.3f, .sy = 0.3f });
        house_hero(G, p->slot, p->x, p->y, p->y < CK_MURAL_FLOOR ? HP_JET : HP_IDLE, tt, p->face);
        float bx = p->x + p->face * 9, by = p->y - 16;
        float br = 2 + p->size * 2;   // the brush tip grows with the brush size (C)
        shape(SPR_DOT, bx, by, br + 1, 0x1B1426, 1);
        shape(SPR_DOT, bx, by, br, p->col < CK_MURAL_PAL ? ck_mural_pal[p->col] : p->col == CK_MURAL_CLEAR ? 0xE04F4F : 0xFFF6E6, 1);
    }
    particles(G, 0);
    hint(11, "A up   B paint   L R colour   C size   START leave");
}

// The aquarium, three screens wide: two water tiles then the baked beach screen, swaying weed,
// fish food, then shrimp, fish, crabs and the heroes (a diving bubble round the head in the water).
static void draw_aqua(const ck_game *G) {
    const ck_house *Hs = &G->house;
    float t = G->t, cam = Hs->gcam;
    for (int k = 0; k < 2; k++) if (k * W - cam > -W && k * W - cam < W) blit(SPR_AQUA_WATER, 0, k * W - cam, 0);
    blit(SPR_AQUA_BG, 0, 2 * W - cam, 0);
    for (int k = 0; k < 8; k++) {
        float x0 = 30 + k * 110 + (k % 2) * 22 - cam, y0 = CK_AQUA_SEABED, hgt = 28 + (k * 37 % 3) * 9;
        if (x0 < -10 || x0 > W + 10 || x0 + cam > CK_AQUA_BEACH_X - 40) continue;
        for (float y = 0; y > -hgt; y -= 6) {
            float xa = x0 + sinf(t * 2 + y * 0.15f) * 3, xb = x0 + sinf(t * 2 + (y - 6) * 0.15f) * 3;
            line(xa, y0 + y, xb, y0 + y - 6, 3, 0x2F9A5A, 1);
        }
    }
    for (int k = 0; k < CK_AQUA_CHESTS; k++) {   // treasure chests: B opens them
        float x = ck_aqua_chest_x[k] - cam, y = CK_AQUA_SEABED;
        if (x < -30 || x > W + 30) continue;
        rect(x - 11, y - 14, 22, 14, 0xA8672F, 1);
        if (Hs->chestT[k] > 0) {   // open: the lid up, gold inside, glinting
            rect(x - 9, y - 13, 18, 4, 0x3A2618, 1);
            shape(SPR_DOT, x - 4, y - 13, 3, 0xFFD23F, 1); shape(SPR_DOT, x + 3, y - 14, 3, 0xFFD23F, 1);
            rect(x - 11, y - 24, 22, 6, 0xC98A4B, 1);
            if ((int)(t * 6) % 2) shape(SPR_STAR, x + 2, y - 18, 3, 0xFFF6A0, 1);
        } else {
            rect(x - 11, y - 18, 22, 6, 0xC98A4B, 1);
            rect(x - 2, y - 14, 4, 4, 0xFFD23F, 1);
        }
    }
    for (int i = 0; i < Hs->nfood; i++) shape(SPR_DOT, Hs->food[i].x - cam, Hs->food[i].y, 1.6f, 0xE8B46A, 1);
    for (int i = 0; i < CK_AQUA_SHRIMP; i++) { const ck_shrimp *s = &Hs->shrimp[i]; blit_o(SPR_HOUSE_SHRIMP, (int)(t * 6 + i) % 2, s->x - cam, s->y - 3, (bopt){ .flip = s->face < 0 }); }
    for (int i = 0; i < CK_AQUA_FISH; i++) {
        const ck_fish *f = &Hs->fish[i];
        float x = f->x - cam, wig = f->happy > 0 ? sinf(f->happy * 0.8f) * 0.25f : 0;
        if (x < -20 || x > W + 20) continue;
        blit_o(SPR_HOUSE_FISH, f->kind * 2 + (int)(t * (f->happy > 0 ? 14 : 6) + f->ph) % 2, x, f->y, (bopt){ .flip = f->vx < 0, .rot = wig });
        if (f->happy > 20) shape(SPR_STAR, x, f->y - 12, 3, 0xFF8FC7, 1);
    }
    for (int i = 0; i < CK_AQUA_CRABS; i++) { const ck_crab *c = &Hs->crabs[i]; blit(SPR_HOUSE_CRAB, c->snap > 0 && (c->snap / 5) % 2 == 0, c->x - cam, CK_AQUA_BEACH_Y - 5 + c->y); }
    for (int i = 0; i < Hs->np; i++) {
        const ck_hhero *p = &Hs->gs[i];
        float tt = t + p->slot * 0.7f, x = p->x - cam;
        int pose = p->land ? (!p->ground ? HP_JUMP : fabsf(p->vx) > 0.3f ? HP_RUN : HP_IDLE) : HP_JUMP;
        house_hero(G, p->slot, x, p->y, pose, tt, p->face);
        if (!p->land) {
            float hy = p->y - 24;
            shape(SPR_DOT, x, hy, 12, 0xBFF0FF, 0.3f);
            shape(SPR_RING, x, hy, 12, 0xE6FCFF, 0.95f);
            shape(SPR_DOT, x - 5, hy - 5, 2, 0xFFFFFF, 1);
        }
    }
    particles(G, cam);
    hint(14, "Stick swim   A up   B feed, grab, throw   START leave");
}

// The garage race, top down: a half of the screen per player, boxes and circles only (raceView
// and carTop in web/lib/houseart.js draw the same picture).
static void car_top(float x, float y, uint32_t hex) {
    for (int k = 0; k < 4; k++) rect(x + (k % 2 ? 7 : -10), y + (k < 2 ? -11 : 5), 3, 7, 0x1B1426, 1);
    rect(x - 8, y - 14, 16, 28, hex, 1);
    rect(x - 6, y - 7, 12, 5, 0xBFF0FF, 1);
    rect(x - 6, y + 6, 12, 3, 0x000000, 0.25f);
    rect(x - 7, y - 15, 3, 2, 0xFFE066, 1); rect(x + 4, y - 15, 3, 2, 0xFFE066, 1);
}
// one obstacle in a track's look: the kinds behave the same on every track
static void race_thing(int track, int kind, float x, float y, float t) {
    switch (track * 4 + kind) {
    case TRACK_HILLS * 4 + OBST_CONE: shape(SPR_DOT, x, y, 6, 0xFF8A3D, 1); shape(SPR_DOT, x, y, 2.5f, 0xFFFFFF, 1); break;
    case TRACK_HILLS * 4 + OBST_CRATE: rect(x - 9, y - 9, 18, 18, 0x8A5A3C, 1); rect(x - 7, y - 7, 14, 14, 0xC98A4B, 1); rect(x - 1, y - 7, 2, 14, 0x8A5A3C, 1); break;
    case TRACK_HILLS * 4 + OBST_OIL: shape(SPR_DOT, x, y, 10, 0x1B1426, 0.85f); shape(SPR_DOT, x + 3, y - 2, 3, 0x5A4F78, 1); break;
    case TRACK_HILLS * 4 + OBST_CAR: car_top(x, y, 0x9AA1B0); break;
    case TRACK_DESERT * 4 + OBST_CONE:   // a cactus
        shape(SPR_DOT, x, y, 6, 0x4F9A43, 1); shape(SPR_DOT, x - 5, y - 3, 3, 0x6FAF4F, 1); shape(SPR_DOT, x + 5, y + 2, 3, 0x6FAF4F, 1); shape(SPR_DOT, x, y, 2, 0xFF8FC7, 1); break;
    case TRACK_DESERT * 4 + OBST_CRATE:   // a boulder
        shape(SPR_DOT, x, y, 10, 0x8C7A66, 1); shape(SPR_DOT, x - 3, y - 3, 5, 0xB09C84, 1); break;
    case TRACK_DESERT * 4 + OBST_OIL:   // a sand pit
        shape(SPR_DOT, x, y, 11, 0xC9A35A, 1); shape(SPR_DOT, x, y, 6, 0xB08A45, 1); break;
    case TRACK_DESERT * 4 + OBST_CAR:   // a tumbleweed rolling across
        shape(SPR_RING, x, y, 7, 0x9C7A4A, 1); shape(SPR_DOT, x + cosf(t * 6) * 3, y + sinf(t * 6) * 3, 2.5f, 0x8A6A3C, 1); shape(SPR_DOT, x - cosf(t * 6) * 3, y - sinf(t * 6) * 3, 2.5f, 0xB08A55, 1); break;
    case TRACK_SNOW * 4 + OBST_CONE:   // a snowman
        shape(SPR_DOT, x, y + 2, 7, 0xFFFFFF, 1); shape(SPR_DOT, x, y - 6, 4.5f, 0xFFFFFF, 1); shape(SPR_DOT, x + 3, y - 6, 1.5f, 0xFF8A3D, 1); break;
    case TRACK_SNOW * 4 + OBST_CRATE:   // a block of ice
        rect(x - 9, y - 9, 18, 18, 0x7FC4E6, 1); rect(x - 7, y - 7, 14, 14, 0xBFE8FF, 1); rect(x - 5, y - 5, 4, 4, 0xFFFFFF, 1); break;
    case TRACK_SNOW * 4 + OBST_OIL:   // an ice patch
        shape(SPR_DOT, x, y, 11, 0xA8DCF5, 0.95f); shape(SPR_DOT, x - 3, y - 3, 3, 0xFFFFFF, 1); break;
    case TRACK_SNOW * 4 + OBST_CAR:   // a penguin waddling across
        shape(SPR_DOT, x, y, 7, 0x1B1426, 1); shape(SPR_DOT, x, y + 1, 4, 0xFFFFFF, 1); shape(SPR_DOT, x, y - 5, 1.8f, 0xFF8A3D, 1); break;
    case TRACK_MOON * 4 + OBST_CONE:   // a moon rock
        shape(SPR_DOT, x, y, 6, 0xA9A4BC, 1); shape(SPR_DOT, x + 2, y + 1, 2, 0x6E6A80, 1); break;
    case TRACK_MOON * 4 + OBST_CRATE:   // a supply pod
        rect(x - 9, y - 9, 18, 18, 0xC0C4D0, 1); rect(x - 9, y - 2, 18, 4, 0xFFD23F, 1); rect(x - 6, y - 7, 4, 3, 0x5AD1FF, 1); break;
    case TRACK_MOON * 4 + OBST_OIL:   // a crater
        shape(SPR_DOT, x, y, 11, 0x8F8AA8, 1); shape(SPR_DOT, x + 1, y + 1, 8, 0x3A3650, 1); break;
    default: car_top(x, y, 0xE6E6F0); shape(SPR_DOT, x, y, 3, 0xFF5A5A, 1); break;   // a moon rover
    }
}
static void race_view(int track, float x0, float vw, float dist, float clock) {
    const ck_racetrack *T = &ck_tracks[track];
    float cx = x0 + vw / 2;
    rect(x0, 0, vw, H, T->ground, 1);
    for (int k = 0; k < 8; k++) {   // bushes, cacti, pines or craters on the verges, scrolling down
        float y = fmodf(k * 60 + dist, 480) - 120;
        float r = track == TRACK_MOON ? 4 : 7;
        shape(SPR_DOT, x0 + 8 + (k % 2) * 4, y, r, T->deco, 1);
        shape(SPR_DOT, x0 + vw - 10 - (k % 3) * 3, y + 30, r, T->deco, 1);
    }
    rect(cx - 64, 0, 128, H, T->road, 1);
    for (float y = fmodf(dist, 24) - 24; y < H; y += 24) {
        rect(cx - 66, y, 4, 12, T->kerbA, 1); rect(cx - 66, y + 12, 4, 12, T->kerbB, 1);
        rect(cx + 62, y, 4, 12, T->kerbA, 1); rect(cx + 62, y + 12, 4, 12, T->kerbB, 1);
    }
    for (float y = fmodf(dist, 40) - 40; y < H; y += 40) { rect(cx - 21, y, 2, 20, T->line, 1); rect(cx + 19, y, 2, 20, T->line, 1); }
    float fy = CK_RACE_CAR_Y - (CK_RACE_LEN - dist);
    if (fy > -10 && fy < H + 10)
        for (int r = 0; r < 2; r++) for (int k = 0; k < 16; k++) rect(cx - 64 + k * 8, fy - 8 + r * 4, 8, 4, (k + r) % 2 ? 0xFFFFFF : 0x1B1426, 1);
    for (int i = 0; i < CK_RACE_OBST; i++) {
        float od, ox, w, h; int kind;
        ck_race_obst(track, i, clock, &od, &ox, &kind, &w, &h);
        float y = CK_RACE_CAR_Y - (od - dist);
        if (y < -30 || y > H + 30) continue;
        race_thing(track, kind, cx + ox, y, clock + i);
    }
}
static void draw_cars(const ck_game *G) {
    const ck_house *Hs = &G->house;
    char c[12], s[32];
    int n = Hs->np;
    float vw = n > 1 ? W / 2 : W;
    for (int i = 0; i < n; i++) {
        const ck_hhero *k = &Hs->gs[i];
        float x0 = i * vw;
        rdpq_set_scissor(x0, 0, x0 + vw, H);
        race_view(Hs->track, x0, vw, k->a, Hs->clock);
        int col = skin_col(G, k->slot);
        uint32_t hex;
        if (col < CK_NCOLS) hex = ck_cols[col];
        else { color_t rc = rainbow_col(G->t * 0.8f, 255); hex = (uint32_t)rc.r << 16 | (uint32_t)rc.g << 8 | rc.b; }
        float wob = k->rep > 0 ? sinf(k->rep * 0.6f) * 4 : 0;
        car_top(x0 + vw / 2 + k->x + wob, CK_RACE_CAR_Y, hex);
        rect(x0 + vw - 12, 20, 5, 150, 0x1B1426, 0.7f);
        rect(x0 + vw - 13, 166 - 146 * fminf(1, k->a / CK_RACE_LEN), 7, 4, k->slot ? 0x8FD3FF : 0xFFD23F, 1);
        if (n > 1) txt(F_SMALL, k->slot ? ST_BLUE : ST_YELLOW, x0 + 8, 14, ALIGN_LEFT, "P%d", k->slot + 1);
    }
    rdpq_set_scissor(0, 0, W, H);
    if (n > 1) rect(W / 2 - 1, 0, 2, H, 0x1B1426, 1);
    particles(G, 0);
    if (n == 1) { dark_panel(W / 2 - 34, 4, 68, 16); txt(F_SMALL, ST_PLAIN, W / 2, 15, ALIGN_CENTER, "%s", clock_str((int)(Hs->clock * 60), c)); }
    if (Hs->pickTrack) {   // choosing the track
        banner(F_BIG, ST_YELLOW, 0xFFD23F, 78, 200, 34, 101, ck_tracks[Hs->track].name);
        txt(F_SMALL, ST_PLAIN, W / 2 - 92, 100, ALIGN_CENTER, "<");
        txt(F_SMALL, ST_PLAIN, W / 2 + 92, 100, ALIGN_CENTER, ">");
        snprintf(s, sizeof s, "Track %d of %d", Hs->track + 1, CK_RACE_TRACKS);
        hint(124, s);
        hint(H - 12, "< > pick a track   A race!   START leave");
        return;
    }
    if (Hs->count > 0) { snprintf(s, sizeof s, "%d", (int)ceilf(Hs->count)); banner(F_HUGE, ST_YELLOW, 0xFFD23F, 100, 60, 34, 126, s); }
    else if (Hs->win < 0 && Hs->clock < 0.8f) banner(F_HUGE, ST_GREEN, 0x49D17A, 100, 80, 34, 126, "GO!");
    if (Hs->win >= 0) {
        if (n > 1) snprintf(s, sizeof s, "P%d WINS!", Hs->gs[Hs->win].slot + 1);
        else snprintf(s, sizeof s, "FINISHED  %s", clock_str((int)(Hs->gs[0].best * 60), c));
        banner(F_BIG, ST_YELLOW, 0xFFD23F, 98, 120, 30, 119, s);
    }
    hint(H - 12, "Stick steer   A faster   B brake   START leave");
}

// ---------------------------------------------------------------------------- the arcade and the pond
// Mirrors pinballScene, blastScene and fishScene in web/arcade.js: boxes, lines and dots only.
static void score_box(float x, float y, const char *label, int v, int style) {
    dark_panel(x, y, 58, 30);
    txt(F_SMALL, ST_PALE, x + 29, y + 11, ALIGN_CENTER, "%s", label);
    txt(F_MED, style, x + 29, y + 26, ALIGN_CENTER, "%d", v);
}
static void game_over(int score, int best) {
    char s[32];
    banner(F_BIG, ST_YELLOW, 0xFFD23F, 70, 160, 34, 93, "GAME OVER");
    snprintf(s, sizeof s, "Score %d   Best %d", score, best);
    hint(124, s);
    hint(142, "A play again");
}
static void draw_pinball(const ck_game *G) {
    const ck_pinball *P = &G->house.pin;
    rect(0, 0, W, H, 0x2B2240, 1);
    for (int k = 0; k < 24; k++) shape(SPR_DOT, (k * 53) % 320, (k * 37) % 240, 1, 0x5A4F78, 1);
    rect(70, 16, 180, H - 16, 0x1B2A4A, 1);
    for (int y = 30; y < H; y += 20) rect(72, y, 158, 1, 0x22355C, 1);
    for (int k = 0; k < CK_PIN_WALLS; k++) { const ck_seg *w = &ck_pin_walls[k]; line(w->x0, w->y0, w->x1, w->y1, 3, 0xC9C4E0, 1); }
    line(ck_pin_gate.x0, ck_pin_gate.y0, ck_pin_gate.x1, ck_pin_gate.y1, 2, 0xFFD23F, 1);
    for (int k = 0; k < CK_PIN_LIGHTS; k++) {
        bool on = (P->lit & (1 << k)) || (P->litT > 0 && (P->litT / 6) % 2);
        shape(SPR_DOT, ck_pin_lights[k][0], ck_pin_lights[k][1], 5, on ? 0xFFD23F : 0x3A3650, 1);
        shape(SPR_RING, ck_pin_lights[k][0], ck_pin_lights[k][1], 6, 0x8F8AA8, 1);
    }
    static const uint32_t bcol[CK_PIN_BUMPS] = { 0xFF5A5A, 0x5AD1FF, 0xC28BFF, 0x49D17A, 0x49D17A };
    for (int k = 0; k < CK_PIN_BUMPS; k++) {
        const ck_bump *b = &ck_pin_bumps[k];
        float r = b->r + (P->flash[k] > 0 ? 2 : 0);
        shape(SPR_DOT, b->x, b->y, r, P->flash[k] > 0 ? 0xFFFFFF : bcol[k], 1);
        shape(SPR_DOT, b->x, b->y, r * 0.55f, 0x1B1426, 1);
        shape(SPR_DOT, b->x, b->y, r * 0.3f, bcol[k], 1);
    }
    for (int s = 0; s < 2; s++) {
        float x0, y0, x1, y1;
        ck_pin_flipper(s, P->flip[s], &x0, &y0, &x1, &y1);
        line(x0, y0, x1, y1, 6, 0xFFD23F, 1);
        shape(SPR_DOT, x0, y0, 4, 0xFF8A3D, 1);
    }
    if (!P->over || P->balls > 0) {
        shape(SPR_DOT, P->x, P->y, CK_PIN_R, 0xE6E6F0, 1);
        shape(SPR_DOT, P->x - 1.2f, P->y - 1.2f, 1.4f, 0xFFFFFF, 1);
    }
    particles(G, 0);
    score_box(6, 20, "SCORE", P->score, ST_YELLOW);
    score_box(6, 56, "BALLS", P->balls, ST_CREAM);
    score_box(256, 20, "BEST", P->best, ST_GREEN);
    if (P->litT > 0) banner(F_MED, ST_YELLOW, 0xFFD23F, 120, 100, 22, 135, "BONUS!");
    if (P->over) { game_over(P->score, P->best); return; }
    if (P->x > 232 && P->y > 218) hint(H - 12, "A launch!   START leave");
    else hint(H - 12, "B left flipper   A right flipper   START leave");
}

static void bug(int row, float x, float y, int frame) {
    uint32_t col = row == 0 ? 0xFF5A5A : row == 1 ? 0xC28BFF : 0x49D17A;
    float lg = frame ? 2 : -2;
    rect(x - 8, y + 1 + lg * 0.5f, 3, 4, col, 1); rect(x + 5, y + 1 - lg * 0.5f, 3, 4, col, 1);   // legs
    rect(x - 5 + lg, y + 4, 2, 4, col, 1); rect(x + 3 - lg, y + 4, 2, 4, col, 1);
    shape(SPR_DOT, x, y, 6, col, 1);
    rect(x - 3, y - 9, 1, 4, col, 1); rect(x + 2, y - 9, 1, 4, col, 1);   // feelers
    shape(SPR_DOT, x - 2.5f, y - 1, 2, 0xFFFFFF, 1); shape(SPR_DOT, x + 2.5f, y - 1, 2, 0xFFFFFF, 1);
    shape(SPR_DOT, x - 2.5f + (frame ? 0.6f : -0.6f), y - 1, 1, 0x1B1426, 1); shape(SPR_DOT, x + 2.5f + (frame ? 0.6f : -0.6f), y - 1, 1, 0x1B1426, 1);
}
static void draw_blast(const ck_game *G) {
    const ck_house *Hs = &G->house;
    const ck_blaster *B = &Hs->blast;
    char s[24];
    rect(0, 0, W, H, 0x14102A, 1);
    for (int k = 0; k < 40; k++) shape(SPR_DOT, (k * 97) % 320, fmodf((k * 53) % 240 + G->t * (5 + k % 3 * 4), 240), (k % 3 + 1) * 0.5f, 0x8F8AA8, 1);
    for (int k = 0; k < CK_BUG_COLS * CK_BUG_ROWS; k++) {
        if (!(B->bugs & (1u << k))) continue;
        float x, y;
        ck_bug_at(B, k, &x, &y);
        bug(k / CK_BUG_COLS, x, y, B->frame);
    }
    for (int sh = 0; sh < CK_SHIELDS; sh++)
        for (int c = 0; c < CK_SHIELD_W * CK_SHIELD_H; c++)
            if (B->shield[sh][c]) rect(ck_shield_x[sh] + (c % CK_SHIELD_W) * CK_SHIELD_CELL, CK_SHIELD_Y + (c / CK_SHIELD_W) * CK_SHIELD_CELL, CK_SHIELD_CELL, CK_SHIELD_CELL, (c / CK_SHIELD_W + c) % 2 ? 0x49D17A : 0x3FB86A, 1);
    for (int b = 0; b < CK_BLAST_BOMBS; b++) if (B->bomb[b].on) { shape(SPR_DOT, B->bomb[b].x + sinf(B->bomb[b].y * 0.4f) * 1.5f, B->bomb[b].y, 2.5f, 0xFFD23F, 1); }
    for (int i = 0; i < Hs->np; i++) {
        if (B->shot[i].on) rect(B->shot[i].x - 1, B->shot[i].y - 4, 2, 8, 0xFFFFFF, 1);
        if (B->inv[i] > 0 && (B->inv[i] / 4) % 2) continue;
        int col = skin_col(G, Hs->gs[i].slot);
        uint32_t hex = col < CK_NCOLS ? ck_cols[col] : 0xFFFFFF;
        float x = B->gun[i];
        rect(x - 9, CK_BLAST_Y - 2, 18, 7, hex, 1);
        rect(x - 6, CK_BLAST_Y - 5, 12, 4, hex, 1);
        rect(x - 1.5f, CK_BLAST_Y - 10, 3, 6, 0xE6E6F0, 1);
        shape(SPR_DOT, x, CK_BLAST_Y - 2, 2, 0xBFF0FF, 1);
    }
    particles(G, 0);
    txt(F_SMALL, ST_YELLOW, 8, 12, ALIGN_LEFT, "SCORE %d", B->score);
    txt(F_SMALL, ST_GREEN, W / 2, 12, ALIGN_CENTER, "BEST %d", B->best);
    for (int k = 0; k < B->lives; k++) { rect(W - 16 - k * 16, 5, 12, 5, 0xFFD23F, 1); rect(W - 11 - k * 16, 2, 2, 4, 0xE6E6F0, 1); }
    if (B->over) { game_over(B->score, B->best); return; }
    if (B->waveT > 0) { snprintf(s, sizeof s, "WAVE %d", B->wave + 1); banner(F_BIG, ST_YELLOW, 0xFFD23F, 100, 120, 34, 123, s); }
    hint(H - 6, "Stick move   A or B fire   START leave");
}

static void pond_fish(int kind, float x, float y, int dir, float t) {
    const ck_catch *c = &ck_catches[kind];
    if (kind == CATCH_BOOT) {
        rect(x - 5, y - 8, 7, 9, c->col, 1); rect(x - 5, y - 1, 13, 5, c->col, 1); rect(x - 6, y + 3, 15, 2, 0x3A2A20, 1);
        return;
    }
    float L = c->len, w = sinf(t * 8) * 1.2f;
    shape(SPR_DOT, x - dir * L * 0.75f, y + w, L * 0.4f, c->col, 1);   // tail
    shape(SPR_DOT, x, y, L * 0.55f, c->col, 1);
    shape(SPR_DOT, x + dir * L * 0.25f, y, L * 0.4f, c->col, 1);
    shape(SPR_DOT, x + dir * L * 0.35f, y - L * 0.12f, 1.4f, 0x1B1426, 1);
    if (kind == CATCH_GOLDEN) shape(SPR_STAR, x, y - L * 0.6f, 3, 0xFFFFFF, 0.5f + 0.5f * sinf(t * 6));
}
static void draw_fish(const ck_game *G) {
    const ck_house *Hs = &G->house;
    const ck_pond *Pd = &Hs->pond;
    float t = G->t;
    char s[40];
    rect(0, 0, W, CK_POND_TOP, 0x8FD3FF, 1);
    cloud(150, 30); cloud(250, 18);
    for (int k = 0; k < 8; k++) shape(SPR_DOT, 90 + k * 32, CK_POND_TOP - 4, 14 + (k % 3) * 4, 0x3F9D4E, 1);
    rect(0, CK_POND_TOP, W, H - CK_POND_TOP, 0x3A8FC0, 1);
    rect(0, CK_POND_TOP + 40, W, 40, 0x347FB0, 1);
    rect(0, CK_POND_TOP + 80, W, H, 0x2E70A0, 1);
    rect(0, CK_POND_BED, W, H - CK_POND_BED, 0xC9B27A, 1);
    for (int k = 0; k < 6; k++) { float x = 100 + k * 40; rect(x, CK_POND_BED - 18 - (k % 3) * 6, 2, 18 + (k % 3) * 6, 0x2F7A3C, 1); }
    for (int x = 0; x < W; x += 16) rect(x + fmodf(t * 8, 16), CK_POND_TOP, 8, 1, 0xBFE8FF, 1);
    for (int k = 0; k < CK_POND_FISH; k++) {
        const ck_pfish *f = &Pd->fish[k];
        pond_fish(f->kind, f->x, f->y, f->vx < 0 ? -1 : 1, t + k);
    }
    rect(0, 100, CK_POND_BANK, H - 100, 0x8A5A3C, 1);   // the bank
    rect(0, 96, CK_POND_BANK + 4, 8, 0x5DBB63, 1);
    for (int i = 0; i < Hs->np; i++) {
        const ck_rod *R = &Pd->rod[i];
        int slot = Hs->gs[i].slot;
        float hx = 22 + i * 18, tx, ty;
        ck_rod_tip(i, &tx, &ty);
        if (R->state == ROD_CHARGE) { tx -= R->power * 30; ty -= R->power * 6; }   // swung back
        house_hero(G, slot, hx, 98, R->state == ROD_GOT ? HP_CHEER : HP_IDLE, t + i * 0.7f, 1);
        line(hx + 6, 78, tx, ty, 2, 0x8A5A3C, 1);
        if (R->state == ROD_CHARGE) {
            dark_panel(hx - 14, 30 + i * 8, 30, 7);
            rect(hx - 13, 31 + i * 8, 28 * R->power, 5, R->power > 0.8f ? 0xFF5A5A : 0xFFD23F, 1);
        }
        if (R->state == ROD_FLY) line(tx, ty, R->x, R->y, 1, 0xFFF6D8, 0.9f);
        if (R->state == ROD_WAIT || R->state == ROD_BITE || R->state == ROD_REEL) {
            float dip = R->state == ROD_BITE ? 4 + sinf(t * 30) * 2 : (R->fish >= 0 && R->t < 6) ? 2 : sinf(t * 3) * 0.8f;
            float by = CK_POND_TOP - 2 + dip;
            line(tx, ty, R->x, by, 1, 0xFFF6D8, 0.9f);
            line(R->x, by, R->x, R->y, 1, 0xFFF6D8, 0.6f);
            shape(SPR_DOT, R->x, by, 3, 0xFFFFFF, 1); shape(SPR_DOT, R->x, by - 1.5f, 2, 0xFF5A5A, 1);
            shape(SPR_RING, R->x, R->y, 2.5f, 0xC9C4E0, 1);
            if (R->state == ROD_BITE) banner(F_BIG, ST_YELLOW, 0xFF5A5A, by - 40, 30, 26, by - 21, "!");
        }
        if (R->state == ROD_REEL) {   // how tight the line is
            float bx = 96 + i * 120;
            dark_panel(bx, 4, 104, 16);
            txt(F_SMALL, ST_PALE, bx + 4, 15, ALIGN_LEFT, "LINE");
            rect(bx + 30, 8, 70, 8, 0x3A3650, 1);
            rect(bx + 30, 8, 70 * R->tension / 100, 8, R->tension > 75 ? 0xFF5A5A : R->tension > 45 ? 0xFFD23F : 0x49D17A, 1);
        }
        if (R->state == ROD_GOT && Pd->last >= 0) {
            const ck_catch *c = &ck_catches[Pd->last];
            snprintf(s, sizeof s, Pd->last == CATCH_BOOT ? "An %s!" : "A %s!", c->name);
            banner(F_BIG, ST_YELLOW, 0xFFD23F, 40, 160, 32, 62, s);
            pond_fish(Pd->last, W / 2, 92, 1, t);
            if (c->bolts) { snprintf(s, sizeof s, "+%d bolts", c->bolts); hint(116, s); }
        }
        if (R->state == ROD_SNAP) banner(F_BIG, ST_RED, 0xFF5A5A, 50, 160, 32, 72, "Snap! It got away");
    }
    particles(G, 0);
    snprintf(s, sizeof s, "Caught %d", Pd->caught);
    dark_panel(W - 74, 4, 70, 16); txt(F_SMALL, ST_CREAM, W - 39, 15, ALIGN_CENTER, "%s", s);
    const ck_rod *R0 = &Pd->rod[0];
    const char *help = R0->state == ROD_CHARGE ? "Let go of A to cast!" : R0->state == ROD_WAIT ? "^ v hook depth   B reel in   START leave"
                     : R0->state == ROD_BITE ? "A bite! Press A!" : R0->state == ROD_REEL ? "Hold A to reel   let go when it goes red"
                     : "Hold A, let go to cast   START leave";
    hint(H - 6, help);
}

// The music room's piano: the keys (lit while they sound), notes floating up, and each hero
// standing over the key under their hand.
static void draw_drums(const ck_game *G) {
    const ck_house *Hs = &G->house;
    const ck_room *room = &ck_rooms[4];
    rect(0, 0, W, H, room->wall, 1);
    for (int x = 12; x < W; x += 24) rect(x, 0, 10, 200, room->stripe, 1);
    rect(0, 200, W, 40, room->floor, 1);
    for (int k = 0; k < CK_DRUMS; k++) {
        const ck_drum *d = &ck_drums[k];
        float lit = Hs->lit[k] / 10.0f, r = d->r * (1 + lit * 0.12f);
        rect(d->x - 1, d->y, 2, 200 - d->y, 0x5A5560, 1);
        if (d->kind == DRUM_DRUM) {
            shape(SPR_DOT, d->x, d->y, r + 3, 0x2B2833, 1);
            shape(SPR_DOT, d->x, d->y, r, d->col, 1);
            shape(SPR_DOT, d->x, d->y, r * 0.62f, 0xFFF6E6, 1);
        } else if (d->kind == DRUM_CYMBAL) {
            rect(d->x - r - 2, d->y - 4 - lit * 3, r * 2 + 4, 8, 0x2B2833, 1);
            rect(d->x - r, d->y - 2 - lit * 3, r * 2, 4, d->col, 1);
        } else {
            shape(SPR_DOT, d->x - 6 + lit * 4, d->y, r * 0.6f, d->col, 1);
            shape(SPR_DOT, d->x + 6 - lit * 4, d->y, r * 0.6f, d->col, 1);
        }
        if (lit > 0) shape(SPR_DOT, d->x, d->y, r + 4, 0xFFFFFF, lit * 0.5f);
        rect(d->x - 8, d->y + r + 4, 16, 13, 0x1B1426, 1);
        txt(F_SMALL, ST_PLAIN, d->x, d->y + r + 14, ALIGN_CENTER, "%s", d->button);
    }
    for (int i = 0; i < Hs->nnotes; i++) {
        const ck_note_fx *n = &Hs->notes[i];
        float a = fminf(1, n->life / 20.0f);
        shape(SPR_DOT, n->x, n->y, 3.5f, n->col, a);
        rect(n->x + 2.5f, n->y - 12, 1.6f, 12, n->col, a);
    }
    for (int i = 0; i < Hs->np; i++) {
        const ck_hhero *p = &Hs->gs[i];
        float bob = p->bounce > 0 ? sinf(p->bounce / 10.0f * PI) * 6 : 0;
        house_hero(G, p->slot, i ? 296 : 24, 222 - bob, p->bounce > 0 ? HP_CHEER : HP_IDLE, G->t + i, 1);
    }
    hint(14, "Every button is a drum   START leave");
}
static void draw_piano(const ck_game *G) {
    const ck_house *Hs = &G->house;
    if (Hs->drums) { draw_drums(G); return; }
    const ck_room *room = &ck_rooms[4];
    rect(0, 0, W, H, room->wall, 1);
    for (int x = 12; x < W; x += 24) rect(x, 0, 10, CK_KEYS_Y, room->stripe, 1);
    rect(CK_KEYS_X - 8, CK_KEYS_Y - 14, 14 * CK_KEYS_W + 16, CK_KEYS_H + 18, 0x2B2833, 1);
    for (int w = 0; w < 14; w++) rect(CK_KEYS_X + w * CK_KEYS_W + 1, CK_KEYS_Y, CK_KEYS_W - 2, CK_KEYS_H, 0xFFF6D8, 1);
    for (int i = 0; i < CK_KEYS_N; i++) {
        float x; bool black;
        ck_piano_key(i, &x, &black);
        if (!black && Hs->lit[i]) rect(x - CK_KEYS_W / 2.0f + 1, CK_KEYS_Y, CK_KEYS_W - 2, CK_KEYS_H, 0xFFE066, 1);
    }
    for (int i = 0; i < CK_KEYS_N; i++) {
        float x; bool black;
        ck_piano_key(i, &x, &black);
        if (black) rect(x - CK_KEYS_BW / 2.0f, CK_KEYS_Y, CK_KEYS_BW, CK_KEYS_BH, Hs->lit[i] ? 0xC28BFF : 0x1B1426, 1);
    }
    for (int i = 0; i < Hs->nnotes; i++) {
        const ck_note_fx *n = &Hs->notes[i];
        float a = fminf(1, n->life / 20.0f);
        shape(SPR_DOT, n->x, n->y, 3.5f, n->col, a);
        rect(n->x + 2.5f, n->y - 12, 1.6f, 12, n->col, a);
    }
    for (int i = 0; i < Hs->np; i++) {
        const ck_hhero *p = &Hs->gs[i];
        float x; bool black;
        ck_piano_key(p->key, &x, &black);
        float bob = p->bounce > 0 ? sinf(p->bounce / 10.0f * PI) * 6 : 0;
        house_hero(G, p->slot, x, 132 - bob, p->bounce > 0 ? HP_CHEER : HP_IDLE, G->t + i, 1);
        float y = black ? CK_KEYS_Y + CK_KEYS_BH - 10 : CK_KEYS_Y + CK_KEYS_H - 12;
        uint32_t c = p->slot ? 0x8FD3FF : 0xFFD23F;
        for (int r = 0; r < 8; r++) rect(x - r * 5 / 8.0f, y - 8 + r, r * 10 / 8.0f + 1, 1, c, 1);   // a little arrow
    }
    hint(14, "Stick move   A play   B other octave   START leave");
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
    case SC_MENU: draw_menu(G); break;
    case SC_LEVEL: draw_level(G); break;
    case SC_JET: draw_jet(G); break;
    case SC_PAUSE: draw_pause(G, scanlines); break;
    case SC_CUT: draw_cut(G); break;
    case SC_HOUSE: draw_house(G); break;
    case SC_PAINT: draw_paint(G); break;
    case SC_AQUA: draw_aqua(G); break;
    case SC_CARS: draw_cars(G); break;
    case SC_PIANO: draw_piano(G); break;
    case SC_SLOTS: draw_slots(G); break;
    case SC_PINBALL: draw_pinball(G); break;
    case SC_BLAST: draw_blast(G); break;
    case SC_FISH: draw_fish(G); break;
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
