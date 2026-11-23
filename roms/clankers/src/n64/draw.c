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

static bool is_house_art(int id);
static void sand_free(void);
static bool is_big(int id) {
    static const int big[] = { SPR_TITLE, SPR_SELECT_BG, SPR_JET_SKY, SPR_JET_HILLS, SPR_CRANE, SPR_INTRO_NIGHT,
                               SPR_INTRO_STORM, SPR_INTRO_DAWN, SPR_CUT_FLOAT, SPR_CUT_ROCKET,
                               SPR_AQUA_BG, SPR_AQUA_WATER };
    for (unsigned i = 0; i < sizeof big / sizeof *big; i++) if (big[i] == id) return true;
    if (is_house_art(id)) return true;
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
    int group = sc == SC_INTRO ? 4 : (sc == SC_TITLE || sc == SC_SLOTS) ? 0 : sc == SC_HOUSE ? 6 : sc == SC_AQUA ? 7 : sc == SC_CARS ? 8 : (sc == SC_PAINT || sc == SC_PIANO || sc == SC_PINBALL || sc == SC_BLAST || sc == SC_FISH || sc == SC_BOLT || sc == SC_JETRUN || sc == SC_SAND) ? 9 : G->scene == SC_SELECT ? 1 : (G->scene == SC_MAP || G->scene == SC_SHOP || G->scene == SC_MENU) ? 2 : G->scene == SC_CUT ? 5 : 3;
    group = group * 16 + G->worldNum * 4 + (G->scene == SC_CUT ? G->cut % 4 : 0);   // a new world or film needs its own pictures
    waited = false;
    if (sc != SC_SAND) sand_free();   // the sandbox's textures only while it is open
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
static int acc_idx[ACC_COUNT], acc_n[SLOT_COUNT];   // each item's frame in its slot's strip, and how many share the strip
static void acc_init(void) {
    for (int a = 0; a < ACC_COUNT; a++) acc_idx[a] = acc_n[ck_acc[a].slot]++;
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
// one item at a point of a hero frame; view: hero * 3 + 0 front, 1 side, 2 back (robot 0-2, ostrich 3-5)
static void acc_piece(int a, int view, const acc_pt *p, float x, float y, float sc, bool flip, float t, int shade, int alpha) {
    int slot = ck_acc[a].slot;
    bopt o = { .flip = flip, .sx = p->sx * sc, .sy = p->sy * sc, .rot = flip ? p->rot : -p->rot, .tinted = true,
               .tint = ck_acc[a].rainbow ? rainbow_col(t, alpha) : RGBA32(shade, shade, shade, alpha) };
    if (ck_acc[a].rainbow && shade < 255) { o.tint.r = o.tint.r * shade / 255; o.tint.g = o.tint.g * shade / 255; o.tint.b = o.tint.b * shade / 255; }
    blit_o(ACC_SPR[slot], view * acc_n[slot] + acc_idx[a], x + (flip ? -p->x : p->x) * sc, y + p->y * sc, o);
}
// Suits have sleeves: a stroke down each arm in the suit's colour (white cuffs on the sweater and
// the fancy suit), as K.wearSleeve draws them in the browser.
static void line(float x0, float y0, float x1, float y1, float w, uint32_t hex, float alpha);
// K.SLEEVE and K.CUFF: 0 no sleeve, 1 cycles colour
static const uint32_t SLEEVE[ACC_COUNT] = {
    [ACC_SWEATER] = 0xE0564F, [ACC_JERSEY] = 0x49D17A, [ACC_TUX] = 0x2B2833, [ACC_SUPER] = 0x3A6FC0, [ACC_DISCO] = 1,
    [ACC_CHEFCOAT] = 0xF5F5F0, [ACC_LABCOAT] = 0xF2F4F8, [ACC_POLICE] = 0x2B4A9A, [ACC_LEATHER] = 0x1F1B26, [ACC_FIRECOAT] = 0xD4A64A,
    [ACC_SPACESUIT] = 0xEEF1F6, [ACC_VEST] = 0x6A7A8C, [ACC_PIRATESHIRT] = 0xF5F1E8, [ACC_NINJASUIT] = 0x24202E, [ACC_CLOWNSUIT] = 0xFFD23F,
    [ACC_ARMOR] = 0x9AA3B3, [ACC_ROBE] = 0x5B3F86, [ACC_COWVEST] = 0xC0392B, [ACC_BLUESUIT] = 0x2B4A9A, [ACC_PINSTRIPE] = 0x4A4A58,
    [ACC_WHITESUIT] = 0xF5F1E8, [ACC_PLAIDSUIT] = 0x3A7A4A, [ACC_VELVETSUIT] = 0x7A2A8A, [ACC_GOLDSUIT] = 0xE8B83A,
};
static const uint32_t CUFF[ACC_COUNT] = {
    [ACC_SWEATER] = 0xFFFFFF, [ACC_TUX] = 0xF5F1E8, [ACC_BLUESUIT] = 0xF5F1E8, [ACC_PINSTRIPE] = 0xF5F1E8, [ACC_PLAIDSUIT] = 0xF5F1E8,
    [ACC_VELVETSUIT] = 0xF5F1E8, [ACC_GOLDSUIT] = 0x1B1426, [ACC_WHITESUIT] = 0xFF8FC7, [ACC_FIRECOAT] = 0xFFD23F,
};
static void sleeve(int a, const acc_arm *m, float x, float y, float sc, bool flip, float t, int alpha) {
    uint32_t c = SLEEVE[a];
    if (!c) return;
    if (c == 1) { color_t r = rainbow_col(t, 255); c = (uint32_t)r.r << 16 | (uint32_t)r.g << 8 | r.b; }
    else if (m->far) c = ((c >> 16 & 255) * 3 / 4) << 16 | ((c >> 8 & 255) * 3 / 4) << 8 | (c & 255) * 3 / 4;
    float f = flip ? -1 : 1, x0 = x + m->x0 * sc * f, y0 = y + m->y0 * sc, x1 = x + m->x1 * sc * f, y1 = y + m->y1 * sc, w = fmaxf(1.5f, m->w * sc);
    line(x0, y0, x1, y1, w, c, alpha / 255.0f);
    if (CUFF[a]) {
        float dx = x1 - x0, dy = y1 - y0;
        line(x0 + dx * 0.86f, y0 + dy * 0.86f, x0 + dx * 1.04f, y0 + dy * 1.04f, w, CUFF[a], alpha / 255.0f);
    }
}
// everything a player wears, over hero frame fr drawn at (x, y) with scale sc
static void hero_acc_a(const uint8_t *wear, int hero, int fr, float x, float y, float sc, bool flip, float t, int alpha) {
    if (!wear) return;
    if (fr < 0 || fr >= (int)(sizeof ACC_AT[0] / sizeof ACC_AT[0][0])) fr = 0;
    const acc_at *A = &ACC_AT[hero == HERO_ROBOT ? 0 : 1][fr];
    int view = (hero == HERO_ROBOT ? 0 : 3) + A->side;   // 0 front, 1 side, 2 back
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

// A straight line of width w, drawn as runs of plain rectangles. It used to be a stretched,
// rotated pixel, but the RDP dropped one of that quad's two triangles on long thin lines, so the
// first half of a fishing rod or line vanished on the console.
static void line(float x0, float y0, float x1, float y1, float w, uint32_t hex, float alpha) {
    float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    if (w < 1) w = 1;
    bool steep = fabsf(dy) > fabsf(dx);
    if (steep) { float s; s = x0; x0 = y0; y0 = s; s = x1; x1 = y1; y1 = s; s = dx; dx = dy; dy = s; }
    if (dx < 0) { float s; s = x0; x0 = x1; x1 = s; s = y0; y0 = y1; y1 = s; dx = -dx; dy = -dy; }
    // Walk the long axis a pixel at a time, merging pixels that land on the same row into one run.
    float slope = dy / dx, h = w * len / dx;
    int a0 = (int)floorf(x0), a1 = (int)ceilf(x1);
    if (a1 <= a0) a1 = a0 + 1;
    int run = a0;
    float runB = roundf(y0 + (a0 + 0.5f - x0) * slope - h / 2);
    for (int a = a0 + 1; a <= a1; a++) {
        float b = a < a1 ? roundf(y0 + (a + 0.5f - x0) * slope - h / 2) : runB + 1e9f;
        if (b == runB) continue;
        if (steep) rect(runB, run, h, a - run, hex, alpha);
        else rect(run, runB, a - run, h, hex, alpha);
        run = a; runB = b;
    }
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
        // $ and ^ doubled, as in txt(): rdpq_text reads them as font and style changes (a bare
        // ^ makes rdpq_paragraph_build assert)
        if (*c == '$' || *c == '^') s[m++] = *c;
        else if (*c == ' ' && c > s0 && c[-1] != ' ' && c[1] != ' ') s[m++] = ' ';
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
        txt(F_SMALL, ST_PALE, x + w / 2, y + 151, ALIGN_CENTER, "< > hero   ^ v %s", ck_col_name(col));   // col is SKIN_RAINBOW in the rainbow paint
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
        if (nd->kind == CK_NODE_HOME) { blit(SPR_MAP_HOME_1 + wi, 0, nd->x, nd->y); continue; }   // over the roads leading out of it
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
#define SHOP_TW 22
#define SHOP_TH 28
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
static void item_icon(int a, float x, float y, float t, int col, int hero) {
    if (a < ACC_COUNT) { acc_icon(a, x, y, t); return; }
    float hx = x + SHOP_TW / 2 - 1;
    y -= 3;
    if (a >= SHOP_COSTUME0) {   // the player's own hero in the whole look
        uint8_t wear[SLOT_COUNT];
        for (int k = 0; k < SLOT_COUNT; k++) wear[k] = (uint8_t)(ck_costumes[a - SHOP_COSTUME0].piece[k] + 1);
        hero_front(hero, col, 0, hx, y + 29, hero == HERO_ROBOT ? 0.062f / 0.074f : 0.056f / 0.071f, false, wear, t);
        return;
    }
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
    if (a >= SHOP_COSTUME0) return ck_costume_worn(G, s, a);
    if (a == SHOP_ROBOT) return G->prog.hero[s] == HERO_ROBOT;
    if (a == SHOP_OSTRICH) return G->prog.hero[s] == HERO_OSTRICH;
    if (a == SHOP_GUN || a == SHOP_JET) return ck_shop_owns(G, a);
    if (a == SHOP_RAINBOW) return ck_rainbow_skin(G, s);
    return !ck_rainbow_skin(G, s) && G->prog.col[s] == a - SHOP_PAINT0;
}
// One player gets the whole width, 12 tiles across; two players split it, P1 on the left and P2
// on the right, 6 across each, every half with its own cursor and scroll (as the web shop).
static void draw_shop(const ck_game *G) {
    static const uint32_t tint[SLOT_COUNT + 2] = { 0xFFD23F, 0x5AD1FF, 0xFF8FC7, 0xC28BFF, 0x49D17A, 0xFFB347, 0xFFB347 };
    const ck_closet *c = &G->saves.closet;
    float t = G->t;
    int cols = ck_shop_cols(G), nrows = ck_shop_rows(cols);
    bool two = cols < 12;
    draw_map(G);
    rect(0, 0, W, H, 0x0F0A19, 0.65f);
    panel(8, 6, W - 16, H - 12, 0x3B2A5C, 1);
    txt(F_BIG, ST_YELLOW, 20, 26, ALIGN_LEFT, "PARTS SHOP");
    dark_panel(W - 86, 12, 70, 18);
    blit_o(SPR_BOLT, 0, W - 74, 21, (bopt){ .sx = 5 / 4.2f, .sy = 5 / 4.2f, .rot = -t });
    txt(F_MED, ST_PLAIN, W - 64, 26, ALIGN_LEFT, "x%d", G->prog.bolts);
    if (two) rect(160, 34, 1, 196, 0xFFFFFF, 0.14f);
    int j = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        int cur = G->shopCur[s] >= 0 && G->shopCur[s] < SHOP_COUNT ? G->shopCur[s] : SHOP_COSTUME0, top = G->shopScroll[s];
        float x0 = two ? 14 + j * 152 : 16, right = x0 + cols * 24 - 2;
        // the section the cursor is in, and arrows when there is more above or below
        if (two) txt(F_SMALL, s ? ST_BLUE : ST_YELLOW, x0, 40, ALIGN_LEFT, "P%d  %s", s + 1, ck_shop_sec_name(cur));
        else txt(F_SMALL, ST_PALE, x0, 40, ALIGN_LEFT, "%s", ck_shop_sec_name(cur));
        if (top > 0) txt(F_SMALL, ST_CREAM, right - 4, 40, ALIGN_CENTER, "^");
        if (top + CK_SHOP_ROWS < nrows) txt(F_SMALL, ST_CREAM, right - 4, 170, ALIGN_CENTER, "v");
        for (int r = top; r < top + CK_SHOP_ROWS && r < nrows; r++) {
            int n = ck_shop_row_len(r, cols);
            for (int k = 0; k < n; k++) {
                int a = ck_shop_item_at(r, k, cols);
                float x = x0 + k * 24, y = 44 + (r - top) * 30;
                int cost = ck_shop_price(G, a);
                bool own = ck_shop_owns(G, a), worn = shop_worn(G, s, a);
                int ti = a < ACC_COUNT ? ck_acc[a].slot : a >= SHOP_COSTUME0 ? SLOT_COUNT + 1 : SLOT_COUNT;
                rect(x, y, SHOP_TW, SHOP_TH, worn ? 0x49D17A : tint[ti], worn ? 0.4f : ti == SLOT_COUNT + 1 ? 0.16f : 0.14f);
                item_icon(a, x, y, t, skin_col(G, s), G->prog.hero[s]);
                // owned: a green dot; for sale: the price in the corner, green if you have the bolts, red if not
                if (own && ck_shop_cost(a)) shape(SPR_DOT, x + SHOP_TW - 3.5f, y + 3.5f, 3, 0x49D17A, 1);
                else if (!own) txt(F_SMALL, G->prog.bolts >= cost ? ST_GREEN : ST_RED, x + SHOP_TW - 1, y + SHOP_TH - 1, ALIGN_RIGHT, "%d", cost);
                if (a == cur) outline(x - 2, y - 2, SHOP_TW + 4, SHOP_TH + 4, 2, s ? 0x8FD3FF : 0xFFD23F);
            }
        }
        // the hero trying it on, over what they already wear (or the whole costume, another hero, a new paint)
        int a = cur;
        uint8_t tryOn[SLOT_COUNT];
        memcpy(tryOn, c->wear[s], sizeof tryOn);
        if (a < ACC_COUNT) tryOn[ck_acc[a].slot] = (uint8_t)(a + 1);
        else if (a >= SHOP_COSTUME0) for (int k = 0; k < SLOT_COUNT; k++) tryOn[k] = (uint8_t)(ck_costumes[a - SHOP_COSTUME0].piece[k] + 1);
        int hero = a == SHOP_ROBOT ? HERO_ROBOT : a == SHOP_OSTRICH ? HERO_OSTRICH : G->prog.hero[s];
        int col = a >= SHOP_PAINT0 && a < SHOP_PAINT0 + CK_NCOLS ? a - SHOP_PAINT0 : a == SHOP_RAINBOW ? SKIN_RAINBOW : skin_col(G, s);
        float hx = two ? 40 + j * 152 : 120, sc = hero == HERO_ROBOT ? 0.11f / 0.074f : 0.1f / 0.071f, tt = t + j;
        rect(hx - 22, 228, 44, 4, 0xFFFFFF, 0.12f);
        int fr = (int)(tt / (hero == HERO_ROBOT ? 0.52f : 0.3f)) % 4;
        hero_front(hero, col, fr, hx, 230, sc, false, tryOn, tt);
        // the words go beside the hero, so they never cover what it is trying on
        float tx = hx + 24;
        char name[32];
        txt(F_MED, s ? ST_BLUE : ST_YELLOW, tx, 206, ALIGN_LEFT, "%s", ck_shop_name(a, name, sizeof name));
        int cost = ck_shop_price(G, a);
        if (!ck_shop_owns(G, a)) {
            char buy[24];
            snprintf(buy, sizeof buy, "A buy  %d", cost);
            txt(F_SMALL, G->prog.bolts >= cost ? ST_PLAIN : ST_RED, tx, 219, ALIGN_LEFT, "%s", buy);
            blit_o(SPR_BOLT, 0, tx + txt_w(F_SMALL, buy) + 5, 216, (bopt){ .sx = 3 / 4.2f, .sy = 3 / 4.2f });
        } else {
            const char *how = a < ACC_COUNT || a >= SHOP_COSTUME0 ? (shop_worn(G, s, a) ? "A take off" : "A wear")
                            : a == SHOP_GUN ? "Z shoots in a level" : a == SHOP_JET ? "Hold A to glide" : "A use";
            txt(F_SMALL, ST_PLAIN, tx, 219, ALIGN_LEFT, "%s", how);
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

static void draw_pause(const ck_game *G, bool scan, bool interlaced) {
    draw_level(G);
    const ck_level *lv = &G->lv;
    rect(0, 0, W, H, 0x0F0A19, 0.6f);
    dark_panel(80, 74, 160, 92);
    txt(F_BIG, ST_YELLOW, W / 2, 100, ALIGN_CENTER, "PAUSED");
    txt(F_SMALL, ST_PLAIN, W / 2, 122, ALIGN_CENTER, "A: keep playing   B: back to map");
    if (lv->def->kind == CK_KIND_LEVEL)
        txt(F_SMALL, ST_PALE, W / 2, 138, ALIGN_CENTER, "Bolts %d/%d    Glitchies left %d", lv->boltGot, lv->boltTotal, ck_foes_left(lv));
    txt(F_SMALL, ST_GREY, W / 2, 156, ALIGN_CENTER, "Z: TV lines %s   R: %s", scan ? "on" : "off", interlaced ? "480i" : "240p");
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
// Mirrors houseScene and its games in web/game.js. The house is seen from above: its garden,
// floors and walls are a baked picture in 160 x 120 tiles (hbg_*), the furniture a sprite per look
// (hart_*, from web/lib/furn34.js), sorted in with the heroes and babies by where their feet are.

// A hero in one of the house's poses: idle and cheer face the front, walking toward us or away
// (and standing with the back to us) use frames 32-41, the rest are side frames.
enum { HP_IDLE, HP_RUN, HP_JUMP, HP_JET, HP_CHEER, HP_WALK_FRONT, HP_WALK_BACK, HP_BACK };
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
// A cooked dish's extras on top: a little one of each ingredient (shelf indices into ck_cook_ingr).
// web/game.js garnishArt draws the same.
static void garnish_art(ck_garnish gar, float x, float y, float sc) {
    int n = gar.n < CK_COOK_PAN ? gar.n : CK_COOK_PAN;
    for (int j = 0; j < n; j++) {
        if (gar.i[j] >= CK_COOK_INGR) continue;
        food_art(ck_cook_ingr[gar.i[j]], x + (j - (n - 1) / 2.0f) * 4.5f * sc, y - 3.5f * sc + (j % 2) * 1.5f * sc, 0.4f * sc);
    }
}
static void house_hero_s(const ck_game *G, int slot, float x, float y, int pose, float t, int face, float sc) {
    int hero = G->prog.hero[slot], col = skin_col(G, slot);
    bool R = hero == HERO_ROBOT, side = pose == HP_RUN || pose == HP_JUMP || pose == HP_JET;
    int fr;
    switch (pose) {
    case HP_RUN: fr = 4 + (int)(t * (R ? 14 : 16)) % 8; break;
    case HP_JUMP: fr = 12; break;
    case HP_JET: fr = R ? 13 + (int)(t * 20) % 2 : 13 + (int)(t * 13 / (2 * PI) * 4) % 4; break;
    case HP_CHEER: fr = 25 + (int)(t * 6) % 2; break;
    case HP_WALK_FRONT: fr = 32 + (int)(t * (R ? 7 : 8)) % 4; break;
    case HP_WALK_BACK: fr = 36 + (int)(t * (R ? 7 : 8)) % 4; break;
    case HP_BACK: fr = 40 + (int)(t / 1.2f) % 2; break;
    default: fr = (int)(t / (R ? 0.52f : 0.3f)) % 4; break;
    }
    bool flip = side && face < 0;
    blit_o(hero_sprite(hero, col), fr, x, y, skin_o((bopt){ .flip = flip, .sx = sc, .sy = sc }, col, t));
    hero_acc(wear_of(G, slot), hero, fr, x, y, sc, flip, t);
}
static void house_hero(const ck_game *G, int slot, float x, float y, int pose, float t, int face) { house_hero_s(G, slot, x, y, pose, t, face, 1); }
static void cloud(float x, float y) { shape(SPR_DOT, x, y, 12, 0xFFFFFF, 1); shape(SPR_DOT, x + 12, y - 4, 14, 0xFFFFFF, 1); shape(SPR_DOT, x + 26, y, 11, 0xFFFFFF, 1); }
// a soft oval, rx by ry (shadows on the floor)
static void oval(float x, float y, float rx, float ry, uint32_t hex, float alpha) {
    blit_o(SPR_DOT, 0, x, y, (bopt){ .sx = rx / 7.5f, .sy = ry / 7.5f, .tinted = true, .tint = hexc(hex, (int)(alpha * 255)) });
}
// The house picture is baked at CK_HOUSE_ZOOM, and at a half and a quarter of it, in tiles of
// CK_HBG_TW x CK_HBG_TH pixels (SPR_HBG_*, SPR_HBS_*, SPR_HBQ_*); the furniture (SPR_HART_*) is
// baked at CK_HOUSE_ZOOM too. ck_house_tier picks the set for the zoom, and ck_house_tile_near and
// ck_house_thing_near what to keep loaded: the host tests check that against a memory budget.
static const int HOUSE_TILE0[CK_HOUSE_TIERS] = { SPR_HBG_0, SPR_HBS_0, SPR_HBQ_0 };
static int house_ntiles(int tier) { int c, r; float ww, wh; ck_house_tiles(tier, &c, &r, &ww, &wh); return c * r; }
static bool is_house_art(int id) {
    if (id >= SPR_HART_0 && id < SPR_HART_0 + ck_ntarts) return true;
    for (int k = 0; k < CK_HOUSE_TIERS; k++) if (id >= HOUSE_TILE0[k] && id < HOUSE_TILE0[k] + house_ntiles(k)) return true;
    return false;
}
// The house picture in view, from the tile set for zoom z. First lets go of every tile and
// furniture sprite that is out of reach (drop waits for the RDP once), then draws the tiles.
static void draw_house_bg(float cam, float camy, float z) {
    int tier = ck_house_tier(z);
    static bool art_near[256];
    for (int a = 0; a < ck_ntarts && a < 256; a++) art_near[a] = false;
    for (int i = 0; i < ck_nthings; i++) if (ck_house_thing_near(i, cam, camy, z, CK_HOUSE_KEEP)) art_near[ck_things[i].art] = true;
    for (int a = 0; a < ck_ntarts && a < 256; a++) if (!art_near[a]) drop(SPR_HART_0 + a);
    for (int k = 0; k < CK_HOUSE_TIERS; k++) {
        int cols, rows; float ww, wh;
        ck_house_tiles(k, &cols, &rows, &ww, &wh);
        for (int ty = 0; ty < rows; ty++)
            for (int tx = 0; tx < cols; tx++)
                if (k != tier || !ck_house_tile_near(k, tx, ty, cam, camy, z, CK_HOUSE_KEEP)) drop(HOUSE_TILE0[k] + ty * cols + tx);
    }
    int cols, rows; float ww, wh;
    ck_house_tiles(tier, &cols, &rows, &ww, &wh);
    float sc = z * ww / CK_HBG_TW;
    for (int ty = 0; ty < rows; ty++)
        for (int tx = 0; tx < cols; tx++) {
            if (!ck_house_tile_near(tier, tx, ty, cam, camy, z, 0)) continue;
            float x = (tx * ww - cam) * z, y = (ty * wh - camy) * z;
            int id = HOUSE_TILE0[tier] + ty * cols + tx;
            // whole pixels at the tiles' own size; scaled, a hair bigger so no seams show between them
            if (fabsf(sc - 1) < 1e-4f) blit(id, 0, roundf(x), roundf(y));
            else blit_o(id, 0, floorf(x), floorf(y), (bopt){ .sx = sc + 1.0f / CK_HBG_TW, .sy = sc + 1.0f / CK_HBG_TH });
        }
}
// one thing from ck_things: its shadow, its sprite, and fish in the tank
static void house_thing(const ck_thing *th, float cam, float camy, float z, float t) {
    const ck_tart *a = &ck_tarts[th->art];
    float x = (th->x - cam) * z, y = (th->y - camy) * z;
    if (a->shade) oval(x, y - th->fd * 0.4f * z, (th->fw + 4) * z, (th->fd * 0.5f + 3) * z, 0x140C1E, 0.14f);
    float hs = z / CK_HOUSE_ZOOM;   // the furniture is baked at CK_HOUSE_ZOOM
    if (fabsf(hs - 1) < 1e-4f) blit(SPR_HART_0 + th->art, 0, roundf(x), roundf(y));
    else blit_o(SPR_HART_0 + th->art, 0, x, y, (bopt){ .sx = hs, .sy = hs });
    if (a->tank)
        for (int k = 0; k < 3; k++) {
            float u = t * (0.3f + k * 0.1f) + k * 2;
            blit_o(SPR_HOUSE_FISH, k * 2 + (int)(t * 5) % 2, x + sinf(u) * 28 * a->scale * z, y - (72 - k * 12) * a->scale * z, (bopt){ .sx = 0.6f * z, .sy = 0.6f * z, .flip = cosf(u) < 0 });
        }
}
// a toilet flushing (ck_house flushT): the water swirls round the bowl and down, and for the first
// half a splash of drops leaps out over whoever is standing there (drops: the splash only)
static void house_flush(const ck_house *Hs, float cam, float camy, float z, bool drops) {
    if (Hs->flushT <= 0 || Hs->flushAt < 0) return;
    const ck_thing *th = &ck_things[Hs->flushAt];
    float sc = ck_tarts[th->art].scale, u = 1 - Hs->flushT / (float)CK_FLUSH_T;
    float bx = (th->x - cam) * z, by = (th->y - 18 * sc - camy) * z;
    if (drops) {
        if (u >= 0.5f) return;
        float v = u * 2;
        for (int k = 0; k < 6; k++) shape(SPR_DOT, bx + (k - 2.5f) * 9.6f * v * z, by + (-70 * v + 50 * v * v - 4) * z, 1.4f * z, k % 2 ? 0xBFE9FF : 0x8FD3FF, 1);
        return;
    }
    oval(bx, by, 5.2f * sc * z, 3.6f * sc * z, 0x8FD3FF, 1);
    for (int k = 0; k < 4; k++) {
        float a = u * 26 + k * PI / 2, r = 1 - u * 0.75f;
        shape(SPR_DOT, bx + cosf(a) * 4.4f * sc * r * z, by + sinf(a) * 3 * sc * r * z, (1.4f - u * 0.6f) * sc * z, k % 2 ? 0xFFFFFF : 0x3F8FD8, 1);
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
    float cam = Hs->cam, camy = Hs->camy, t = G->t, Z = Hs->zoom > 0 ? Hs->zoom : CK_HOUSE_ZOOM;
    float B = Z * CK_HOUSE_BIG;   // heroes, babies and food are CK_HOUSE_BIG times their usual size here
    if (cam < 0 || camy < 0 || cam + W / Z > ck_td_world[0] || camy + H / Z > ck_td_world[1]) rect(0, 0, W, H, 0x5DBB63, 1);   // zoomed out past the property (housetop.js GRASS): grass round it
    draw_house_bg(cam, camy, Z);
    // the furniture, babies, heroes and thrown food, back to front by where their feet are
    enum { D_THING, D_BABY, D_HERO, D_FOOD, D_SPLASH };
    static struct { float y; short kind, i; } list[200];
    int n = 0;
    for (int i = 0; i < ck_nthings && n < 200; i++) {
        const ck_thing *th = &ck_things[i];
        if (th->spot == SPOT_BACK && !ck_spot_here(G, SPOT_BACK)) continue;
        if (!ck_house_thing_near(i, cam, camy, Z, 0)) continue;
        list[n++] = (typeof(list[0])){ th->flat ? th->y - 1e4f : th->y, D_THING, (short)i };
    }
    for (int i = 0; i < Hs->nbabies && n < 200; i++) list[n++] = (typeof(list[0])){ Hs->babies[i].y, D_BABY, (short)i };
    for (int i = 0; i < Hs->np && n < 200; i++) list[n++] = (typeof(list[0])){ Hs->ps[i].y, D_HERO, (short)i };
    for (int i = 0; i < Hs->nthrown && n < 200; i++) list[n++] = (typeof(list[0])){ Hs->thrown[i].y, D_FOOD, (short)i };
    if (Hs->flushT > 0 && Hs->flushAt >= 0 && n < 200) list[n++] = (typeof(list[0])){ ck_things[Hs->flushAt].y + 60, D_SPLASH, 0 };
    for (int i = 1; i < n; i++) for (int j = i; j > 0 && list[j - 1].y > list[j].y; j--) { typeof(list[0]) tmp = list[j]; list[j] = list[j - 1]; list[j - 1] = tmp; }
    for (int k = 0; k < n; k++) {
        int i = list[k].i;
        if (list[k].kind == D_THING) { house_thing(&ck_things[i], cam, camy, Z, t); if (i == Hs->flushAt) house_flush(Hs, cam, camy, Z, false); }
        else if (list[k].kind == D_SPLASH) house_flush(Hs, cam, camy, Z, true);
        else if (list[k].kind == D_BABY) {
            const ck_hbaby *b = &Hs->babies[i];
            float x = (b->x - cam) * Z, y = (b->y - camy) * Z;
            if (x < -20 || x > W + 20 || y < -10 || y > H + 40) continue;
            float hop = b->wait > 0 ? 0 : fabsf(sinf(t * 8 + b->x)) * 2;
            oval(x, y, 6 * B, 2.1f * B, 0x140C1E, 0.22f);
            baby(b->hex, (int)(t * 3 + i) % 2, x, y - hop * B - b->z * Z, 0.05f / 0.06f * B);
        } else if (list[k].kind == D_HERO) {
            const ck_hhero *p = &Hs->ps[i];
            float x = (p->x - cam) * Z, y = (p->y - camy - p->jz) * Z, tt = t + p->slot * 0.7f;
            // four ways: side on when walking more across than up or down, else toward us or away
            bool vert = fabsf(p->ly) > fabsf(p->lx), away = vert && p->ly < 0;
            int pose = p->eat > 0 || Hs->mode == HM_SLEEP ? HP_CHEER : p->jz > 0 ? HP_JUMP
                     : p->moving ? (vert ? (away ? HP_WALK_BACK : HP_WALK_FRONT) : HP_RUN) : away ? HP_BACK : HP_IDLE;
            float r = G->prog.hero[p->slot] == HERO_ROBOT ? 10 : 9;
            oval(x, (p->y - camy) * Z, r * B, r * 0.35f * B, 0x140C1E, 0.22f);
            if (fabsf(B - 1) < 1e-4f) { x = roundf(x); y = roundf(y); }   // whole pixels at their usual size
            float hx = x + p->face * 9 * B, hy = y - 20 * B;
            if (away && p->hold && p->eat <= 0) { food_art(p->hold - 1, hx, hy, B); garnish_art(p->garnish, hx, hy, B); }   // from behind, what you hold is in front of you
            house_hero_s(G, p->slot, x, y, pose, tt, p->face, B);
            if (p->eat > 0) { float sc = (0.4f + 0.6f * p->eat / 50.0f) * B; food_art(p->col, x + 6 * B, y - 24 * B, sc); if (p->col >= FOOD_DISH0) garnish_art(p->garnish, x + 6 * B, y - 24 * B, sc); }   // shrinking as it is eaten
            else if (p->hold && !away) { food_art(p->hold - 1, hx, hy, B); garnish_art(p->garnish, hx, hy, B); }
        } else {
            const ck_thrown *f = &Hs->thrown[i];
            oval((f->x - cam) * Z, (f->y - camy) * Z, 4 * B, 1.4f * B, 0x140C1E, 0.22f);
            food_art(f->kind, (f->x - cam) * Z, (f->y - camy - f->z) * Z - 4 * B, B);
        }
    }
    // what A does where you stand, over your head
    int shown = -1;
    if (Hs->mode == HM_WALK)
        for (int i = 0; i < Hs->np; i++) {
            const ck_hhero *p = &Hs->ps[i];
            int k = ck_house_near(G, p);
            if (k < 0 || k == shown) continue;
            shown = k;
            char s[32];
            if (ck_spots[k].kind == SPOT_BACK) snprintf(s, sizeof s, "A  Back to World %d", G->worldNum - 1);
            else snprintf(s, sizeof s, "A  %s", ck_spots[k].label);
            float w = txt_w(F_SMALL, s) + 14, x = fminf(fmaxf((p->x - cam) * Z, w / 2 + 4), W - w / 2 - 4), y = fmaxf(22, (p->y - camy) * Z - 64 * B);
            dark_panel(x - w / 2, y, w, 15);
            txt(F_SMALL, ST_PLAIN, x, y + 11, ALIGN_CENTER, "%s", s);
        }
    particles(G, 0);
    lives_hud(G, 4, 4);
    if (Hs->msgT > 0) banner(F_MED, ST_YELLOW, 0xFFD23F, 26, 120, 20, 40, Hs->msg);
    if (Hs->mode == HM_SLEEP) {
        rect(0, 0, W, H, 0x0C081E, fminf(0.75f, Hs->sleepT * 0.6f));
        for (int k = 0; k < 3; k++) {
            float u = fmodf(Hs->sleepT * 0.8f + k / 3.0f, 1), sz = (8 + u * 8) / 12;
            const ck_thing *bed = &ck_things[0];
            for (int i = 0; i < ck_nthings; i++) if (ck_things[i].spot == SPOT_BED) bed = &ck_things[i];
            blit_o(SPR_ZEE, 0, (bed->x - cam + 10 + u * 20) * Z, (bed->y - camy - 70 - u * 50) * Z, (bopt){ .sx = sz, .sy = sz, .tinted = true, .tint = RGBA32(255, 246, 216, (int)((1 - u) * 255)) });
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
        // a shelf row for each player, six on show, scrolled to keep their pick in view
        rect(30, 22, 260, 4 + 31 * Hs->np, 0x8A5A3C, 1);
        for (int j = 0; j < Hs->np; j++) {
            // the row shows the pick's group, scrolled to keep the pick in view
            int c = Hs->cpick[j], g = ck_cook_group_of(c), a = ck_cook_group[g], len = ck_cook_group[g + 1] - a;
            int show = len < CK_COOK_ROW ? len : CK_COOK_ROW, first = c - 2 < a ? a : c - 2 > a + len - show ? a + len - show : c - 2;
            float fy = 39 + j * 31;
            uint32_t col = Hs->ps[j].slot ? 0x8FD3FF : 0xFFD23F;
            rect(34, 26 + j * 31, 252, 27, 0xC98A4B, 1);
            for (int k = 0; k < show; k++) {
                float fx = 56 + k * 42;
                if (first + k == c && !Hs->dish) shape(SPR_RING, fx, fy, 13, col, 1);
                food_art(ck_cook_ingr[first + k], fx, fy, 2.0f);
            }
            if (first > a) txt(F_SMALL, ST_CREAM, 26, fy + 3, ALIGN_CENTER, "<");
            if (first < a + len - show) txt(F_SMALL, ST_CREAM, 294, fy + 3, ALIGN_CENTER, ">");
            if (!Hs->dish) {   // the group's name, and the pick's under it
                txt(F_SMALL, ST_CREAM, j ? 310 : 10, 211, j ? ALIGN_RIGHT : ALIGN_LEFT, "%s", ck_cook_group_name[g]);
                txt(F_SMALL, Hs->ps[j].slot ? ST_BLUE : ST_YELLOW, j ? 310 : 10, 222, j ? ALIGN_RIGHT : ALIGN_LEFT, "%s", ck_foods[ck_cook_ingr[c]].name);
            }
        }
        rect(50, 150, 220, 80, 0x3A3F4A, 1);   // the stove top
        rect(56, 156, 208, 6, 0x5A5E6A, 1);
        shape(SPR_DOT, 160, 160, 34, Hs->heat > 0 && !Hs->dish ? 0xFF8A3D : 0x2B2833, 1);   // the burner glows while it cooks
        shape(SPR_DOT, 160, 140, 46, 0x5A5E6A, 1);   // the pan
        shape(SPR_DOT, 160, 140, 40, 0x2B2833, 1);
        rect(202, 136, 62, 8, 0x3A2618, 1);   // its handle
        if (Hs->dish) { food_art(Hs->dish - 1, 160, 140, 6); garnish_art(Hs->dishExtra, 160, 140, 6); }
        else for (int i = 0; i < Hs->npan; i++) {
            float a = i * 1.57f + G->t * (Hs->heat > 0 ? 1.5f : 0);
            food_art(Hs->pan[i], 160 + cosf(a) * 18, 140 + sinf(a) * 14, 2.2f);
        }
        rect(60, 196, 200, 10, 0x1B1426, 1);   // heat
        rect(62, 198, 196 * Hs->heat / 100, 6, Hs->dish ? 0x49D17A : 0xFF8A3D, 1);
        if (Hs->dish) { banner(F_MED, ST_YELLOW, 0xFFD23F, 80, 120, 22, 95, Hs->dishName); hint(226, Hs->np > 1 ? "A take one each" : "A take it"); }
        else {
            hint(14, Hs->npan ? "Up/down group   A add   B stir   START leave" : "Up/down group   Left/right pick   A add to the pan");
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
        hint(226, "A close");
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
            if (v) rect(x * CK_MURAL_CELL, CK_MURAL_Y + y * CK_MURAL_CELL, (e - x) * CK_MURAL_CELL, CK_MURAL_CELL, ck_mural_hex(v), 1);
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
        if (p->stick && p->col != CK_MURAL_CLEAR) {   // the sticker in hand, just where A would stamp it
            int g[CK_STICKER_N][CK_STICKER_N], h = CK_STICKER_N / 2;
            int cx = (int)floorf((bx + p->face * 14) / CK_MURAL_CELL), cy = (int)floorf((by - CK_MURAL_Y) / CK_MURAL_CELL);
            ck_sticker_cells(p->kind, p->col == CK_MURAL_ERASER ? 9 : p->col, g);
            float ox = (cx - h) * CK_MURAL_CELL, oy = CK_MURAL_Y + (cy - h) * CK_MURAL_CELL;
            for (int y = 0; y < CK_STICKER_N; y++)
                for (int x = 0; x < CK_STICKER_N; x++)
                    if (g[y][x] > 0) rect(ox + x * CK_MURAL_CELL, oy + y * CK_MURAL_CELL - 2, CK_MURAL_CELL, CK_MURAL_CELL, ck_mural_hex(g[y][x]), p->col == CK_MURAL_ERASER ? 0.4f : 0.85f);
            continue;
        }
        float br = 2 + p->size * 2;   // the brush tip grows with the brush size (C)
        shape(SPR_DOT, bx, by, br + 1, 0x1B1426, 1);
        shape(SPR_DOT, bx, by, br, p->col < CK_MURAL_PAL ? ck_mural_pal[p->col] : p->col == CK_MURAL_CLEAR ? 0xE04F4F : 0xFFF6E6, 1);
    }
    particles(G, 0);
    bool st = Hs->gs[Hs->last].stick;   // the hint follows whoever pressed last
    hint(11, st ? "A stick   B brush   L R colour   C next sticker   START leave" : "A paint   B sticker   L R colour   C size   START leave");
}

// The aquarium, three screens wide: two water tiles then the baked beach screen, swaying weed,
// fish food, then shrimp, fish, crabs and the heroes (a diving bubble round the head in the water).
// The aquarium's fish that are not sprites, facing d (1 right, -1 left); fancyFish in
// web/lib/houseart.js draws the same.
static void fancy_fish(int kind, float x, float y, float d, float t) {
    switch (kind) {
    case AFISH_ANGEL:
        rect(x - d * 8 - 2, y - 3, 4, 6, 0xFFB020, 1);
        rect(x - 2, y - 11, 4, 7, 0xFFE066, 1); rect(x - 2, y + 4, 4, 7, 0xFFE066, 1);
        shape(SPR_DOT, x, y, 5, 0xFFE066, 1);
        rect(x - 1, y - 6, 2, 12, 0x2B2230, 1);
        shape(SPR_DOT, x + d * 3, y - 1, 1.2f, 0x1B1426, 1); break;
    case AFISH_PUFFER:
        shape(SPR_DOT, x - d * 7, y, 3, 0xE0A92B, 1);
        for (int k = 0; k < 8; k++) shape(SPR_DOT, x + cosf(k * 0.785f) * 7, y + sinf(k * 0.785f) * 7, 1.2f, 0xC9A35A, 1);
        shape(SPR_DOT, x, y, 6, 0xFFD23F, 1); shape(SPR_DOT, x, y + 2, 3.5f, 0xFFF6A0, 1);
        shape(SPR_DOT, x + d * 3, y - 2, 1.6f, 0xFFFFFF, 1); shape(SPR_DOT, x + d * 3.4f, y - 2, 0.8f, 0x1B1426, 1); break;
    case AFISH_JELLY:
        for (int k = 0; k < 4; k++) { float tx = x - 4 + k * 2.7f; line(tx, y + 2, tx + sinf(t * 3 + k) * 2, y + 13, 1, 0xE6B8FF, 0.9f); }
        shape(SPR_DOT, x, y, 6, 0xC28BFF, 0.75f); shape(SPR_DOT, x - 2, y - 2, 1.5f, 0xFFFFFF, 0.8f); break;
    case AFISH_EEL:
        for (int k = 6; k >= 0; k--) shape(SPR_DOT, x - d * k * 4, y + sinf(t * 5 - k * 0.9f) * 2.5f, 3.2f - k * 0.25f, k % 2 ? 0x4F9A6E : 0x3F7D5E, 1);
        shape(SPR_DOT, x + d * 1.5f, y - 1, 1, 0xFFE066, 1); break;
    case AFISH_CLOWN:
        rect(x - d * 9 - 2, y - 3, 4, 6, 0xFF8A3D, 1);
        shape(SPR_DOT, x, y, 5, 0xFF8A3D, 1);
        rect(x + d * 1.5f - 1, y - 4.5f, 2, 9, 0xFFFFFF, 1); rect(x - d * 4 - 1, y - 4, 2, 8, 0xFFFFFF, 1);
        shape(SPR_DOT, x + d * 3.5f, y - 1, 1, 0x1B1426, 1); break;
    case AFISH_TANG:
        rect(x - d * 9 - 2, y - 3, 4, 6, 0xFFD23F, 1);
        shape(SPR_DOT, x, y, 5, 0x3A6FC0, 1); rect(x - 3, y - 1, 6, 2, 0x1B2A5A, 1);
        shape(SPR_DOT, x + d * 3, y - 1.5f, 1, 0xFFFFFF, 1); break;
    default: {   // a ray, gliding with its wings
        float fl = sinf(t * 3) * 2;
        line(x - d * 5, y, x - d * 18, y + 1, 1.5f, 0x3A4A6A, 1);
        shape(SPR_DOT, x - d * 1, y - 5 + fl, 4, 0x5A6E8A, 1); shape(SPR_DOT, x - d * 1, y + 5 - fl, 4, 0x5A6E8A, 1);
        shape(SPR_DOT, x, y, 5, 0x6E86A8, 1);
        shape(SPR_DOT, x + d * 3, y - 1.5f, 0.9f, 0x1B1426, 1); break;
    }
    }
}
// The plants and things of each section, behind the fish.
static void aqua_decor(float cam, float t) {
    float S = CK_AQUA_SEABED;
    if (cam < 340) {   // the kelp forest: tall swaying kelp
        static const float kx[6] = { 40, 95, 150, 205, 260, 300 }, kh[6] = { 120, 150, 100, 160, 130, 110 };
        for (int k = 0; k < 6; k++)
            for (float y = 0; y < kh[k]; y += 8) {
                float sw = sinf(t * 1.4f + k + y * 0.03f) * y * 0.06f, sw2 = sinf(t * 1.4f + k + (y + 8) * 0.03f) * (y + 8) * 0.06f;
                line(kx[k] + sw - cam, S - y, kx[k] + sw2 - cam, S - y - 8, 4, 0x2F8A4A, 1);
                if ((int)y % 16 == 8) shape(SPR_DOT, kx[k] + sw - cam + ((int)y % 32 ? 4 : -4), S - y, 3, 0x49D17A, 1);
            }
    }
    if (cam > 0 && cam < 660) {   // the coral reef
        static const float bx[3] = { 360, 470, 590 }, br[3] = { 10, 8, 11 };
        static const uint32_t bc[3] = { 0xFF8FC7, 0xFFB347, 0xC28BFF };
        for (int k = 0; k < 3; k++) { shape(SPR_DOT, bx[k] - cam, S - 4, br[k], bc[k], 1); shape(SPR_RING, bx[k] - cam, S - 4, br[k] * 0.6f, 0xFFFFFF, 0.5f); }
        static const float cx[3] = { 400, 520, 615 };
        for (int k = 0; k < 3; k++) {
            float x = cx[k] - cam; uint32_t c = k % 2 ? 0xFF8A3D : 0xE04F4F;
            line(x, S, x, S - 30, 3, c, 1); line(x, S - 17, x - 10, S - 32, 3, c, 1); line(x, S - 22, x + 9, S - 36, 3, c, 1);
            shape(SPR_DOT, x, S - 31, 2, c, 1); shape(SPR_DOT, x - 10, S - 33, 2, c, 1); shape(SPR_DOT, x + 9, S - 37, 2, c, 1);
        }
        for (int k = 0; k < 7; k++) { float a = -2.6f + k * 0.35f; line(440 - cam, S, 440 - cam + cosf(a) * 34, S + sinf(a) * 34, 2, 0x9C5AD1, 1); shape(SPR_DOT, 440 - cam + cosf(a) * 34, S + sinf(a) * 34, 2, 0xC28BFF, 1); }
        for (int k = 0; k < 6; k++) { float a = -2.4f + k * 0.34f + sinf(t * 2 + k) * 0.12f; line(560 - cam, S - 4, 560 - cam + cosf(a) * 16, S - 4 + sinf(a) * 16, 3, 0xFF8FC7, 1); }
    }
    if (cam > 320 && cam < 980) {   // the shipwreck
        float x = 700 - cam;
        line(x + 80, S - 44, x + 110, S - 130, 5, 0x6F4630, 1);   // the broken mast and its tattered sail
        line(x + 90, S - 100, x + 128, S - 90, 3, 0x6F4630, 1);
        rect(x + 96, S - 96, 22, 18, 0xE8DCC0, 0.75f);
        rect(x, S - 40, 150, 40, 0x5A3A22, 1);
        rect(x, S - 44, 150, 6, 0x6F4630, 1);
        for (int k = 1; k < 5; k++) rect(x, S - 40 + k * 8, 150, 1, 0x3A2618, 1);
        for (int k = 0; k < 4; k++) { shape(SPR_DOT, x + 20 + k * 30, S - 26, 4, 0x2B3A4A, 1); shape(SPR_RING, x + 20 + k * 30, S - 26, 5, 0xC9A35A, 1); }
        float ax = 900 - cam;   // an anchor
        line(ax, S - 32, ax, S - 6, 3, 0x6E6A80, 1); line(ax - 8, S - 26, ax + 8, S - 26, 3, 0x6E6A80, 1);
        shape(SPR_RING, ax, S - 36, 4, 0x6E6A80, 1); line(ax - 9, S - 9, ax, S - 4, 3, 0x6E6A80, 1); line(ax + 9, S - 9, ax, S - 4, 3, 0x6E6A80, 1);
    }
    if (cam > 640 && cam < 1300) {   // the deep blue: darker, with glowing tube worms
        float x = 960 - cam;
        rect(x + 16, CK_AQUA_TOP, 288, S - CK_AQUA_TOP, 0x0A1840, 0.45f);
        for (int k = 0; k < 4; k++) { rect(x + k * 4, CK_AQUA_TOP, 4, S - CK_AQUA_TOP, 0x0A1840, 0.1f + k * 0.09f); rect(x + 304 + (3 - k) * 4, CK_AQUA_TOP, 4, S - CK_AQUA_TOP, 0x0A1840, 0.1f + k * 0.09f); }
        for (int k = 0; k < 7; k++) {
            float wx = x + 30 + k * 40, h = 20 + (k * 13) % 22;
            line(wx, S, wx, S - h, 3, 0x3A3650, 1);
            shape(SPR_DOT, wx, S - h, 3, k % 2 ? 0x5AD1FF : 0xFF8FC7, 0.5f + 0.5f * sinf(t * 2 + k));
        }
    }
    if (cam > 960) {   // the rocky caves
        static const float rx[3] = { 1320, 1400, 1520 }, rr[3] = { 30, 22, 36 };
        for (int k = 0; k < 3; k++) { shape(SPR_DOT, rx[k] - cam, S + 6, rr[k], k % 2 ? 0x7D798F : 0x6E6A80, 1); shape(SPR_DOT, rx[k] - cam - rr[k] * 0.3f, S - rr[k] * 0.4f, rr[k] * 0.3f, 0x8F8AA8, 1); }
        shape(SPR_STAR, 1360 - cam, S - 30, 6, 0xFF8A3D, 1); shape(SPR_STAR, 1478 - cam, S - 22, 5, 0xFF8FC7, 1);
        static const float ux[2] = { 1440, 1580 };
        for (int k = 0; k < 2; k++) { float x = ux[k] - cam; for (int j = 0; j < 8; j++) line(x, S - 5, x + cosf(j * 0.785f) * 9, S - 5 + sinf(j * 0.785f) * 9, 1, 0x1B1426, 1); shape(SPR_DOT, x, S - 5, 5, 0x2B2240, 1); }
        for (int k = 0; k < 3; k++) rect(1556 - cam + k * 7, S - 14 - k * 5, 5, 14 + k * 5, 0xFFD23F, 1);
    }
}
static void draw_aqua(const ck_game *G) {
    const ck_house *Hs = &G->house;
    float t = G->t, cam = Hs->gcam;
    for (int k = 0; k < CK_AQUA_SECTIONS; k++) if (k * W - cam > -W && k * W - cam < W) blit(SPR_AQUA_WATER, 0, k * W - cam, 0);
    blit(SPR_AQUA_BG, 0, CK_AQUA_SECTIONS * W - cam, 0);
    aqua_decor(cam, t);
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
        if (f->kind >= AFISH_ANGEL) fancy_fish(f->kind, x, f->y, f->vx < 0 ? -1 : 1, t * (f->happy > 0 ? 2.3f : 1) + f->ph);
        else blit_o(SPR_HOUSE_FISH, f->kind * 2 + (int)(t * (f->happy > 0 ? 14 : 6) + f->ph) % 2, x, f->y, (bopt){ .flip = f->vx < 0, .rot = wig });
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
    int sec = (int)((cam + W / 2) / CK_AQUA_SEC_W);
    txt(F_SMALL, ST_CREAM, 8, 234, ALIGN_LEFT, "%s", sec < CK_AQUA_SECTIONS ? ck_aqua_sections[sec] : "The Beach");
    hint(14, "Stick swim   A up   B feed, grab, throw   START leave");
}

// The garage race from behind the car: a half of the screen per player. The road is drawn in strips
// from the bottom up to the horizon, each strip as far ahead as its height says and shifted sideways
// by the bends ahead; things on the road shrink with distance. Boxes and circles only (raceView in
// web/lib/houseart.js draws the same picture).
static uint32_t shade(uint32_t hex, float f) {
    int r = (int)((hex >> 16 & 255) * f), g = (int)((hex >> 8 & 255) * f), b = (int)((hex & 255) * f);
    return (uint32_t)(r > 255 ? 255 : r) << 16 | (uint32_t)(g > 255 ? 255 : g) << 8 | (uint32_t)(b > 255 ? 255 : b);
}
// a car from behind, standing on (x, y), s px per unit, leaning -1..1 into a turn
static void car_back(float x, float y, float s, uint32_t hex, float lean, bool boost, float t) {
    float w = 22 * s, l = lean * 2 * s;
    rect(x - w / 2, y - 1.5f * s, w, 3 * s, 0x000000, 0.35f);   // shadow
    rect(x - w / 2, y - 6 * s, 5 * s, 6 * s, 0x1B1426, 1);   // wheels
    rect(x + w / 2 - 5 * s, y - 6 * s, 5 * s, 6 * s, 0x1B1426, 1);
    rect(x - w / 2 + s + l * 0.3f, y - 12 * s, w - 2 * s, 7 * s, hex, 1);   // body
    rect(x - 7 * s + l, y - 17 * s, 14 * s, 6 * s, shade(hex, 0.8f), 1);   // cabin and its back window
    rect(x - 5.5f * s + l, y - 16 * s, 11 * s, 3.5f * s, 0x8FD3FF, 1);
    rect(x - w / 2 + 2.5f * s + l * 0.3f, y - 10 * s, 3.5f * s, 2 * s, 0xFF5A5A, 1);   // tail lights
    rect(x + w / 2 - 6 * s + l * 0.3f, y - 10 * s, 3.5f * s, 2 * s, 0xFF5A5A, 1);
    rect(x - 4 * s + l * 0.3f, y - 7 * s, 8 * s, 2 * s, 0x2B2833, 1);   // bumper
    if (boost) for (int k = 0; k < 2; k++) shape(SPR_DOT, x + (k ? 4 : -4) * s, y - 4 * s + sinf(t * 40 + k) * s, (2 + fabsf(sinf(t * 30 + k * 2))) * s, k ? 0xFFD23F : 0xFF8A3D, 1);
}
// one obstacle, standing on (x, y), s px per unit
static void race_thing(int track, int kind, float x, float y, float s, float t) {
    switch (track * 4 + kind) {
    case TRACK_HILLS * 4 + OBST_CONE:
        rect(x - 6 * s, y - 2 * s, 12 * s, 2 * s, 0xFF8A3D, 1); rect(x - 4 * s, y - 8 * s, 8 * s, 6 * s, 0xFF8A3D, 1);
        rect(x - 3 * s, y - 6 * s, 6 * s, 2 * s, 0xFFFFFF, 1); rect(x - 2 * s, y - 13 * s, 4 * s, 5 * s, 0xFF8A3D, 1); break;
    case TRACK_HILLS * 4 + OBST_CRATE:
        rect(x - 9 * s, y - 18 * s, 18 * s, 18 * s, 0x8A5A3C, 1); rect(x - 7 * s, y - 16 * s, 14 * s, 14 * s, 0xC98A4B, 1); rect(x - s, y - 16 * s, 2 * s, 14 * s, 0x8A5A3C, 1); break;
    case TRACK_HILLS * 4 + OBST_OIL: rect(x - 10 * s, y - 3 * s, 20 * s, 4 * s, 0x1B1426, 0.85f); rect(x - 4 * s, y - 2.5f * s, 6 * s, 1.5f * s, 0x5A4F78, 1); break;
    case TRACK_HILLS * 4 + OBST_CAR: car_back(x, y, s * 0.85f, 0x9AA1B0, 0, false, t); break;
    case TRACK_DESERT * 4 + OBST_CONE:   // a cactus
        rect(x - 2.5f * s, y - 18 * s, 5 * s, 18 * s, 0x4F9A43, 1); rect(x - 8 * s, y - 13 * s, 3 * s, 7 * s, 0x6FAF4F, 1); rect(x - 8 * s, y - 7 * s, 6 * s, 2.5f * s, 0x6FAF4F, 1);
        rect(x + 5 * s, y - 15 * s, 3 * s, 6 * s, 0x6FAF4F, 1); rect(x + 2 * s, y - 10 * s, 6 * s, 2.5f * s, 0x6FAF4F, 1); shape(SPR_DOT, x, y - 18 * s, 1.5f * s, 0xFF8FC7, 1); break;
    case TRACK_DESERT * 4 + OBST_CRATE:   // a boulder
        shape(SPR_DOT, x, y - 8 * s, 10 * s, 0x8C7A66, 1); shape(SPR_DOT, x - 3 * s, y - 11 * s, 5 * s, 0xB09C84, 1); break;
    case TRACK_DESERT * 4 + OBST_OIL: rect(x - 11 * s, y - 3 * s, 22 * s, 4 * s, 0xB08A45, 1); rect(x - 7 * s, y - 2 * s, 14 * s, 2 * s, 0x9C7A3A, 1); break;   // a sand pit
    case TRACK_DESERT * 4 + OBST_CAR:   // a tumbleweed rolling across
        shape(SPR_RING, x, y - 7 * s, 7 * s, 0x9C7A4A, 1); shape(SPR_DOT, x + cosf(t * 6) * 3 * s, y - 7 * s + sinf(t * 6) * 3 * s, 2.5f * s, 0x8A6A3C, 1);
        shape(SPR_DOT, x - cosf(t * 6) * 3 * s, y - 7 * s - sinf(t * 6) * 3 * s, 2.5f * s, 0xB08A55, 1); break;
    case TRACK_SNOW * 4 + OBST_CONE:   // a snowman
        shape(SPR_DOT, x, y - 6 * s, 7 * s, 0xFFFFFF, 1); shape(SPR_DOT, x, y - 16 * s, 4.5f * s, 0xFFFFFF, 1); rect(x - s, y - 16 * s, 3 * s, 1.5f * s, 0xFF8A3D, 1);
        shape(SPR_DOT, x - 1.5f * s, y - 17.5f * s, 0.8f * s, 0x1B1426, 1); shape(SPR_DOT, x + 1.5f * s, y - 17.5f * s, 0.8f * s, 0x1B1426, 1); break;
    case TRACK_SNOW * 4 + OBST_CRATE:   // a block of ice
        rect(x - 9 * s, y - 18 * s, 18 * s, 18 * s, 0x7FC4E6, 1); rect(x - 7 * s, y - 16 * s, 14 * s, 14 * s, 0xBFE8FF, 1); rect(x - 5 * s, y - 14 * s, 4 * s, 4 * s, 0xFFFFFF, 1); break;
    case TRACK_SNOW * 4 + OBST_OIL: rect(x - 11 * s, y - 3 * s, 22 * s, 4 * s, 0xA8DCF5, 0.95f); rect(x - 5 * s, y - 2.5f * s, 4 * s, 1.5f * s, 0xFFFFFF, 1); break;   // an ice patch
    case TRACK_SNOW * 4 + OBST_CAR:   // a penguin waddling across
        shape(SPR_DOT, x, y - 8 * s, 7 * s, 0x1B1426, 1); shape(SPR_DOT, x, y - 7 * s, 4.5f * s, 0xFFFFFF, 1); rect(x - 1.5f * s, y - 13 * s, 3 * s, 2 * s, 0xFF8A3D, 1);
        rect(x - 4 * s, y - 1.5f * s, 3 * s, 1.5f * s, 0xFF8A3D, 1); rect(x + s, y - 1.5f * s, 3 * s, 1.5f * s, 0xFF8A3D, 1); break;
    case TRACK_MOON * 4 + OBST_CONE: shape(SPR_DOT, x, y - 5 * s, 6 * s, 0xA9A4BC, 1); shape(SPR_DOT, x + 2 * s, y - 4 * s, 2 * s, 0x6E6A80, 1); break;   // a moon rock
    case TRACK_MOON * 4 + OBST_CRATE:   // a supply pod
        rect(x - 9 * s, y - 18 * s, 18 * s, 18 * s, 0xC0C4D0, 1); rect(x - 9 * s, y - 11 * s, 18 * s, 4 * s, 0xFFD23F, 1); rect(x - 6 * s, y - 16 * s, 4 * s, 3 * s, 0x5AD1FF, 1); break;
    case TRACK_MOON * 4 + OBST_OIL: rect(x - 11 * s, y - 3 * s, 22 * s, 4 * s, 0x8F8AA8, 1); rect(x - 8 * s, y - 2 * s, 16 * s, 2.5f * s, 0x3A3650, 1); break;   // a crater
    default: car_back(x, y, s * 0.85f, 0xE6E6F0, 0, false, t); rect(x - 0.5f * s, y - 24 * s, s, 8 * s, 0x8F8AA8, 1); shape(SPR_DOT, x, y - 24 * s, 1.5f * s, 0xFF5A5A, 1); break;   // a moon rover
    }
}
// where something at depth zr ahead of the camera and x across sits on the screen, and its scale
static bool race_project(int track, float a, float camx, float cx, float vw, float zr, float x, float *sx, float *sy, float *s) {
    if (zr < 4) return false;
    float row = ck_race_row(zr), ppu = row / (H - CK_RACE_HZ) * CK_RACE_PX0 * vw / 320;
    float dz = fmaxf(0, CK_RACE_ZC / row - CK_RACE_CAR_Z);   // bends curve the road as a true camera would show that row
    *s = ppu;
    *sy = CK_RACE_HZ + row;
    *sx = cx + (x + 0.5f * CK_RACE_BEND * ck_race_curve(track, a + dz / 2) * dz * dz - camx) * ppu;
    return true;
}
typedef struct { float zr, x; int what, kind; uint32_t col; float lean; bool boost; } race_spr;   // what: 0 obstacle, 1 pad, 2 car
static void race_view(const ck_game *G, int track, float x0, float vw, const ck_hhero *me) {
    const ck_house *Hs = &G->house;
    const ck_racetrack *T = &ck_tracks[track];
    float cx = x0 + vw / 2, a = me->a, camx = me->x * 0.7f, cam = a - CK_RACE_CAR_Z;
    // the sky and a skyline that turns with the bends
    rect(x0, 0, vw, CK_RACE_HZ, T->sky, 1);
    if (track == TRACK_MOON) for (int k = 0; k < 24; k++) shape(SPR_DOT, x0 + fmodf(k * 47 - me->sky * 0.3f + 4000, vw), 6 + (k * 29) % 70, (k % 3) * 0.4f + 0.4f, 0xFFFFFF, 1);
    else for (int k = 0; k < 3; k++) shape(SPR_DOT, x0 + fmodf(k * 70 + 20 - me->sky * 0.15f + 4000, vw + 40) - 20, 22 + k * 9, 9, 0xFFFFFF, 0.8f);   // clouds
    for (int k = 0; k < 8; k++) {
        float hx = x0 + fmodf(k * 61 - me->sky + 8000, vw + 80) - 40, r = 18 + (k * 7) % 14;
        shape(SPR_DOT, hx, CK_RACE_HZ + r * 0.35f, r, T->hills, 1);
    }
    // the ground and road, strip by strip from the bottom up
    for (int y = H - 2; y >= CK_RACE_HZ; y -= 2) {
        float zr = ck_race_depth(y + 1 - CK_RACE_HZ), z = cam + zr, sx = cx, sy = 0, s = 1;
        race_project(track, a, camx, cx, vw, zr, 0, &sx, &sy, &s);
        // far strips cover many bands: show them plain so they don't flicker
        bool band = ck_race_depth(y - 1 - CK_RACE_HZ) - zr < 24 && ((int)(z / 32) & 1);
        float hw = CK_RACE_ROAD * s, kw = 7 * s;
        rect(x0, y, vw, 2, band ? T->ground : shade(T->ground, 0.9f), 1);
        rect(sx - hw - kw, y, kw, 2, band ? T->kerbA : T->kerbB, 1);
        rect(sx + hw, y, kw, 2, band ? T->kerbA : T->kerbB, 1);
        bool finish = z > CK_RACE_LEN && z < CK_RACE_LEN + 16;
        if (finish) for (int k = 0; k < 8; k++) rect(sx - hw + k * hw / 4, y, hw / 4, 2, ((int)(z / 8) + k) % 2 ? 0xFFFFFF : 0x1B1426, 1);
        else {
            rect(sx - hw, y, hw * 2, 2, band ? T->road : shade(T->road, 1.08f), 1);
            if (band) { float lw = fmaxf(1, 2 * s); rect(sx - 20 * s - lw / 2, y, lw, 2, T->line, 1); rect(sx + 20 * s - lw / 2, y, lw, 2, T->line, 1); }
        }
    }
    // things on the road, far ones first
    race_spr list[CK_RACE_OBST + CK_RACE_PADS + CK_RACE_CARS];
    int n = 0;
    for (int i = 0; i < CK_RACE_OBST; i++) {
        float od, ox, w, h; int kind;
        ck_race_obst(track, i, Hs->clock, &od, &ox, &kind, &w, &h);
        float zr = od - cam;
        if (zr > 6 && zr < CK_RACE_FAR) list[n++] = (race_spr){ zr, ox, 0, kind, 0, 0, false };
    }
    for (int i = 0; i < CK_RACE_PADS; i++) {
        float pd, px; int kind;
        ck_race_pad(track, i, &pd, &px, &kind);
        float zr = pd - cam;
        if (zr > 6 && zr < CK_RACE_FAR) list[n++] = (race_spr){ zr, px, 1, kind, 0, 0, false };
    }
    for (int i = 0; i < Hs->np; i++) {
        const ck_hhero *o = &Hs->gs[i];
        if (o == me) continue;
        float zr = o->a - cam;
        int col = skin_col(G, o->slot);
        if (zr > 6 && zr < CK_RACE_FAR) list[n++] = (race_spr){ zr, o->x, 2, (int)o->jz, col < CK_NCOLS ? ck_cols[col] : 0xE6E6F0, o->lean, o->boost > 0 };
    }
    for (int b = 0; b < Hs->nbots; b++) {   // the computer cars
        const ck_hhero *o = &Hs->bot[b];
        float zr = o->a - cam;
        if (zr > 6 && zr < CK_RACE_FAR) list[n++] = (race_spr){ zr, o->x, 2, (int)o->jz, ck_race_bot_col[b], o->lean, o->boost > 0 };
    }
    for (int i = 1; i < n; i++) for (int j = i; j > 0 && list[j].zr > list[j - 1].zr; j--) { race_spr t = list[j]; list[j] = list[j - 1]; list[j - 1] = t; }
    for (int i = 0; i < n; i++) {
        const race_spr *r = &list[i];
        float sx = 0, sy = 0, s = 0;
        if (!race_project(track, a, camx, cx, vw, r->zr, r->x, &sx, &sy, &s) || s * 20 < 1.5f) continue;
        if (r->what == 1) {   // pads lie flat: as tall as their 30 units of road look from here
            float sx2 = 0, sy2 = sy, s2 = 0;
            race_project(track, a, camx, cx, vw, r->zr + 30, r->x, &sx2, &sy2, &s2);
            float h = fmaxf(1, sy - sy2);
            if (r->kind == PAD_BOOST) {
                rect(sx - 14 * s, sy - h, 28 * s, h, 0xFF8A3D, 1);
                for (int k = 0; k < 3; k++) rect(sx - 10 * s, sy - h + h * (k * 0.33f + 0.08f), 20 * s, fmaxf(1, h * 0.14f), 0xFFE066, 1);   // arrows
            } else {
                rect(sx - 14 * s, sy - h - 6 * s, 28 * s, h + 6 * s, 0x8A6A4A, 1);   // a ramp rising to its far edge
                rect(sx - 14 * s, sy - h - 6 * s, 28 * s, fmaxf(1, 2 * s), 0xFFD23F, 1);
                for (int k = 0; k < 4; k++) rect(sx - 12 * s + k * 7 * s, sy - h - 4 * s, 3 * s, h + 3 * s, 0xA8865E, 1);
            }
        } else if (r->what == 2) car_back(sx, sy - r->kind * s, s, r->col, r->lean, r->boost, G->t);
        else race_thing(track, r->kind, sx, sy, s, G->t + i);
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
        race_view(G, Hs->track, x0, vw, k);
        int col = skin_col(G, k->slot);
        uint32_t hex;
        if (col < CK_NCOLS) hex = ck_cols[col];
        else { color_t rc = rainbow_col(G->t * 0.8f, 255); hex = (uint32_t)rc.r << 16 | (uint32_t)rc.g << 8 | rc.b; }
        float wob = k->rep > 0 ? sinf(k->rep * 0.6f) * 4 : 0, sx = 0, sy = 0, sc = 1;
        race_project(Hs->track, k->a, k->x * 0.7f, x0 + vw / 2, vw, CK_RACE_CAR_Z, k->x, &sx, &sy, &sc);
        if (k->jz > 0) rect(sx - 11 * sc, sy - 1.5f * sc, 22 * sc, 3 * sc, 0x000000, 0.3f);   // the shadow stays on the road
        car_back(sx + wob, sy - k->jz * sc, sc, hex, k->lean, k->boost > 0, G->t);
        // the progress bar and the player's label sit by the middle of the screen, back to back
        bool left = n > 1 && i == 1;
        float bx = left ? x0 + 7 : x0 + vw - 12;
        rect(bx, 20, 5, 150, 0x1B1426, 0.7f);
        for (int b = 0; b < Hs->nbots; b++) rect(bx, 167 - 146 * fminf(1, Hs->bot[b].a / CK_RACE_LEN), 5, 2, ck_race_bot_col[b], 1);
        rect(bx - 1, 166 - 146 * fminf(1, k->a / CK_RACE_LEN), 7, 4, k->slot ? 0x8FD3FF : 0xFFD23F, 1);
        if (!Hs->pickTrack) {   // your place in the race
            static const char *const ord[CK_RACE_CARS] = { "1st", "2nd", "3rd", "4th" };
            int rk = ck_race_rank(Hs, k);
            txt(F_MED, rk == 1 ? ST_YELLOW : ST_CREAM, left ? x0 + vw - 8 : x0 + 8, 22, left ? ALIGN_RIGHT : ALIGN_LEFT, "%s", ord[rk - 1]);
        }
        if (n > 1) txt(F_SMALL, k->slot ? ST_BLUE : ST_YELLOW, left ? x0 + 16 : x0 + vw - 16, 14, left ? ALIGN_LEFT : ALIGN_RIGHT, "P%d", k->slot + 1);
        if (k->boost > 0) txt(F_SMALL, ST_YELLOW, x0 + vw / 2, 112, ALIGN_CENTER, "BOOST!");
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
    if (Hs->win >= 0) {   // places when the race is over
        static const char *const ord[CK_RACE_CARS + 1] = { "-", "1st", "2nd", "3rd", "4th" };
        if (n > 1) snprintf(s, sizeof s, "P1 %s   P2 %s", ord[Hs->gs[0].place], ord[Hs->gs[1].place]);
        else snprintf(s, sizeof s, "%s!  %s", ord[Hs->gs[0].place], clock_str((int)(Hs->gs[0].best * 60), c));
        banner(F_BIG, ST_YELLOW, 0xFFD23F, 98, 120, 30, 119, s);
    }
    hint(H - 12, "Stick steer   A gas   B brake   START leave");
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
// a yellow and black hazard stripe along a line, n stripes long
static void hazard(float x0, float y0, float x1, float y1, float w, int n) {
    for (int k = 0; k < n; k++) {
        float a = (float)k / n, b = (float)(k + 1) / n;
        line(x0 + (x1 - x0) * a, y0 + (y1 - y0) * a, x0 + (x1 - x0) * b, y0 + (y1 - y0) * b, w, k % 2 ? 0x1B1426 : 0xFFD23F, 1);
    }
}
// The Clank-O-Matic: a tall riveted steel table with gear bumpers, bolt lanes, a spinner, a bank of
// drop targets, a saucer and a robot face whose eyes light up with the bolts. The table is drawn at
// CK_PIN_ZOOM and scrolled by P->cam, so everything goes through PX and PY.
static float pin_cam;
static float PX(float x) { return CK_PIN_OX + x * CK_PIN_ZOOM; }
static float PY(float y) { return (y - pin_cam) * CK_PIN_ZOOM; }
#define PZ(v) ((v) * CK_PIN_ZOOM)
static void pin_line(float x0, float y0, float x1, float y1, float w, uint32_t hex) { line(PX(x0), PY(y0), PX(x1), PY(y1), PZ(w), hex, 1); }
static void pin_dot(float x, float y, float r, uint32_t hex, float a) { shape(SPR_DOT, PX(x), PY(y), PZ(r), hex, a); }
// thin seams land on whole pixels, or a fractional scroll makes them come and go as the view moves
static void pin_rect(float x, float y, float w, float h, uint32_t hex) {
    float sx = PX(x), sy = PY(y), sw = PZ(w), sh = PZ(h);
    if (sh < 2) { sy = floorf(sy); sh = 2; }   // two lines tall, so a 480i field never drops it
    if (sw < 2) { sx = floorf(sx); sw = 1; }
    rect(sx, sy, sw, sh, hex, 1);
}
static void pin_hazard(float x0, float y0, float x1, float y1, float w, int n) { hazard(PX(x0), PY(y0), PX(x1), PY(y1), PZ(w), n); }
static const char *pin_msg(int m) {
    return m == PINMSG_BONUS ? "BONUS!" : m == PINMSG_LOCK ? "BALL LOCKED" : m == PINMSG_MULTI ? "MULTIBALL!" : m == PINMSG_DOUBLE ? "DOUBLE SCORE!" : "";
}
static void draw_pinball(const ck_game *G) {
    const ck_pinball *P = &G->house.pin;
    int lit = (P->lit & 1) + (P->lit >> 1 & 1) + (P->lit >> 2 & 1);
    bool party = P->litT > 0 && (P->litT / 6) % 2;
    pin_cam = P->cam;
    rect(0, 0, W, H, 0x2B2240, 1);
    for (int k = 0; k < 24; k++) shape(SPR_DOT, (k * 53) % 320, (k * 37) % 240, 1, 0x5A4F78, 1);
    pin_rect(10, 10, 220, CK_PIN_H - 10, 0x34404F);   // the steel plate, its seams and rivets
    for (int y = 50; y < CK_PIN_H; y += 50) pin_rect(10, y, 220, 1, 0x2E3A48);
    for (int x = 66; x < 212; x += 56) pin_rect(x, 10, 1, CK_PIN_H - 10, 0x2E3A48);
    for (int y = 50; y < CK_PIN_H; y += 50) for (int x = 66; x < 212; x += 56) pin_dot(x, y, 1.5f, 0x6A7888, 1);
    pin_rect(10, 10, 24, 240, 0x2C3644);   // the left orbit's lane
    // the robot face painted on the table
    pin_rect(87, 262, 48, 36, 0x4A5666);
    pin_rect(90, 265, 42, 30, 0x3E4856);
    pin_rect(110, 252, 2, 10, 0x8F8AA8);
    pin_dot(111, 251, 3, party ? 0xFFD23F : 0xE04F4F, 1);
    for (int e = 0; e < 2; e++) {
        pin_dot(101 + e * 20, 275, 5.5f, 0x1B2A3A, 1);
        if (party || lit > e) pin_dot(101 + e * 20, 275, 4, 0x5AD1FF, 1);
    }
    pin_rect(98, 284, 26, 6, 0x1B1426);   // the mouth grille glows when all three bolts are lit
    for (int k = 0; k < 5; k++) pin_rect(100 + k * 5, 285, 2, 4, party || lit >= 3 ? 0xFFD23F : 0x6A7888);
    // DOUBLE lamp under the drop targets, LOCK lamps by the saucer
    pin_dot(190, 224, 4, P->x2T > 0 && (P->x2T / 8) % 2 ? 0xFF8A3D : 0x5A3A2A, 1);
    for (int k = 0; k < 2; k++) pin_dot(140 + k * 20, 196, 3, (P->locks % 2) > k ? 0x5AD1FF : 0x2A3A4A, 1);
    pin_hazard(10, CK_PIN_DRAIN - 4, 212, CK_PIN_DRAIN - 4, 6, 18);   // the drain
    pin_rect(215, 436, 12, 6, 0xE04F4F);   // the plunger
    pin_rect(219, 442, 4, 10, 0x8F8AA8);
    for (int k = 0; k < CK_PIN_WALLS - 2; k++) { const ck_seg *w = &ck_pin_walls[k]; pin_line(w->x0, w->y0, w->x1, w->y1, 3, 0xC9C4E0); }
    for (int k = CK_PIN_WALLS - 2; k < CK_PIN_WALLS; k++) { const ck_seg *w = &ck_pin_walls[k]; pin_hazard(w->x0, w->y0, w->x1, w->y1, 4, 7); }
    pin_line(ck_pin_gate.x0, ck_pin_gate.y0, ck_pin_gate.x1, ck_pin_gate.y1, 2, 0xFFD23F);
    for (int k = 0; k < CK_PIN_SLINGS; k++) {   // slingshots flash red when they kick
        const ck_seg *w = &ck_pin_slings[k];
        pin_line(w->x0, w->y0, w->x1, w->y1, 4, P->sling[k] > 0 ? 0xFFFFFF : 0xE04F4F);
    }
    for (int k = 0; k < CK_PIN_DROPS; k++) {   // drop targets: orange plates, flat when down
        const ck_seg *w = &ck_pin_drops[k];
        bool down = P->drops & (1 << k);
        if (down) pin_line(w->x0, w->y0 + 2, w->x1, w->y1 - 2, 1.5f, 0x5A3A2A);
        else { pin_line(w->x0, w->y0, w->x1, w->y1, 5, P->dropT > 0 ? 0xFFFFFF : 0xFF8A3D); pin_line(w->x0 + 1.5f, w->y0 + 3, w->x1 + 1.5f, w->y1 - 3, 1.5f, 0xFFD23F); }
    }
    {   // the spinner: a flap that turns over as it spins
        const ck_seg *w = &ck_pin_spinner;
        float c = cosf(P->spinA * 6.2832f), h = 4 * fabsf(c) + 1;
        pin_rect(w->x0, w->y0 - 0.5f, w->x1 - w->x0, 1, 0x8F8AA8);
        pin_rect(w->x0 + 3, w->y0 - h / 2, w->x1 - w->x0 - 6, h, c > 0 ? 0x5AD1FF : 0x2E8FB8);
    }
    pin_dot(ck_pin_saucer[0], ck_pin_saucer[1], 8, 0x1B1426, 1);   // the saucer
    shape(SPR_RING, PX(ck_pin_saucer[0]), PY(ck_pin_saucer[1]), PZ(8), 0x8F8AA8, 1);
    for (int k = 0; k < CK_PIN_LIGHTS; k++) {   // bolt targets in the rollover lanes
        bool on = (P->lit & (1 << k)) || party;
        float x = ck_pin_lights[k][0], y = ck_pin_lights[k][1];
        pin_dot(x, y, 7, 0x1B1426, 1);
        shape(SPR_RING, PX(x), PY(y), PZ(7), on ? 0xFFD23F : 0x6A7888, 1);
        blit_o(SPR_BOLT, 0, PX(x), PY(y), (bopt){ .sx = PZ(1.1f), .sy = PZ(1.1f), .rot = on ? G->t * 3 : 0, .tinted = !on, .tint = RGBA32(90, 90, 110, 255) });
    }
    static const uint32_t bcol[CK_PIN_BUMPS] = { 0xFF5A5A, 0x5AD1FF, 0xC28BFF, 0x49D17A, 0x49D17A };
    for (int k = 0; k < CK_PIN_BUMPS; k++) {
        const ck_bump *b = &ck_pin_bumps[k];
        bool hit = P->flash[k] > 0;
        float r = b->r + (hit ? 2 : 0);
        if (b->r > 8) {   // big bumpers are gears that spin when hit
            if (hit) pin_dot(b->x, b->y, r + 3, 0xFFFFFF, 0.6f);
            blit_o(SPR_GEAR, 0, PX(b->x), PY(b->y), (bopt){ .sx = PZ(r / 9), .sy = PZ(r / 9), .rot = (k % 2 ? -1 : 1) * G->t * (hit ? 6 : 0.8f) });
            pin_dot(b->x, b->y, r * 0.45f, 0x1B1426, 1);
            pin_dot(b->x, b->y, r * 0.3f, hit ? 0xFFFFFF : bcol[k], 1);
        } else {   // small ones are rubber tyres
            pin_dot(b->x, b->y, r, hit ? 0xFFFFFF : 0x25222C, 1);
            pin_dot(b->x, b->y, r * 0.55f, 0x3C3846, 1);
            pin_dot(b->x, b->y, r * 0.28f, bcol[k], 1);
        }
    }
    for (int s = 0; s < CK_PIN_FLIPS; s++) {   // the flippers are robot arms
        float x0, y0, x1, y1;
        ck_pin_flipper(s, P->flip[s], &x0, &y0, &x1, &y1);
        pin_line(x0, y0, x1, y1, 7, 0x8F8AA8);
        pin_line(x0, y0, x1, y1, 3, 0xC9C4E0);
        pin_dot(x1, y1, 3.5f, 0xFF8A3D, 1);
        pin_dot(x0, y0, 4.5f, 0x5A5E6A, 1);
        pin_dot(x0, y0, 2, 0xFFD23F, 1);
    }
    bool lane = false;
    for (int i = 0; i < CK_PIN_MULTI; i++) {
        const ck_pball *b = &P->ball[i];
        if (!b->on) continue;
        lane |= ck_pin_in_lane(b);
        pin_dot(b->x, b->y, CK_PIN_R, 0xE6E6F0, 1);
        pin_dot(b->x - 1.2f, b->y - 1.2f, 1.4f, 0xFFFFFF, 1);
    }
    particles(G, 0);
    txt(F_SMALL, ST_YELLOW, 32, 12, ALIGN_CENTER, "CLANK-O");
    txt(F_SMALL, ST_YELLOW, 32, 22, ALIGN_CENTER, "MATIC");
    score_box(3, 30, "SCORE", P->score, ST_YELLOW);
    score_box(3, 66, "BALLS", P->balls, ST_CREAM);
    score_box(259, 30, "BEST", P->best, ST_GREEN);
    if (P->x2T > 0) txt(F_SMALL, ST_YELLOW, 288, 80, ALIGN_CENTER, "x2 %d", P->x2T / 60 + 1);
    if (P->msgT > 0) banner(F_MED, ST_YELLOW, 0xFFD23F, 100, 150, 22, 115, pin_msg(P->msg));
    if (P->over) { game_over(P->score, P->best); return; }
    if (lane) hint(H - 12, "A launch!   START leave");
    else hint(H - 12, "B left flippers   A right flippers   START leave");
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

// ---- Bolt Ball: a paddle each side, the ball, the bolt; web/arcade.js boltScene draws the same
static const char *bolt_name(const ck_game *G, int s, char *buf, int n) {
    const ck_house *Hs = &G->house;
    int w = Hs->bolt.who[s];
    if (w < 0) snprintf(buf, n, "CPU");
    else if (Hs->np == 1) snprintf(buf, n, "YOU");
    else snprintf(buf, n, "P%d", Hs->gs[w].slot + 1);
    return buf;
}
static uint32_t bolt_hex(const ck_game *G, int s) {
    int w = G->house.bolt.who[s];
    if (w < 0) return 0x9AA4B4;
    int col = skin_col(G, G->house.gs[w].slot);
    return col < CK_NCOLS ? ck_cols[col] : 0xFFFFFF;
}
static void draw_bolt(const ck_game *G) {
    const ck_house *Hs = &G->house;
    const ck_boltball *B = &Hs->bolt;
    char s[48], a[16], b[16];
    rect(0, 0, W, H, 0x14102A, 1);
    for (int k = 0; k < 30; k++) shape(SPR_DOT, (k * 97) % 320, CK_BOLT_TOP + (k * 53) % (CK_BOLT_BOT - CK_BOLT_TOP), ((k % 3) + 1) * 0.5f, 0x3A3456, 1);
    rect(0, CK_BOLT_TOP - 4, W, 4, 0x46386E, 1);
    rect(0, CK_BOLT_BOT, W, 4, 0x46386E, 1);
    for (int y = CK_BOLT_TOP + 4; y < CK_BOLT_BOT; y += 16) rect(W / 2 - 1, y, 2, 8, 0x46386E, 1);
    if (B->bolt) {   // the bolt, a little zigzag with a glow
        float x = B->boltX, y = B->boltY, gl = 0.25f + 0.15f * sinf(G->t * 6);
        shape(SPR_DOT, x, y, 11, 0x5AD1FF, gl);
        line(x + 3, y - 9, x - 3, y + 1, 2.5f, 0xFFD23F, 1);
        line(x - 3, y + 1, x + 3, y - 1, 2.5f, 0xFFD23F, 1);
        line(x + 3, y - 1, x - 3, y + 9, 2.5f, 0xFFD23F, 1);
    }
    for (int sd = 0; sd < 2; sd++) {   // the paddles: little robots standing on end
        if (B->state == BOLT_READY) break;
        float x = sd ? W - CK_BOLT_PX : CK_BOLT_PX - CK_BOLT_PW, y = B->py[sd] - B->ph[sd] / 2;
        uint32_t hex = bolt_hex(G, sd);
        if (B->big[sd] > 0) rect(x - 2, y - 2, CK_BOLT_PW + 4, B->ph[sd] + 4, 0x5AD1FF, B->big[sd] < 120 && (B->big[sd] / 8) % 2 ? 0.2f : 0.5f);
        rect(x, y, CK_BOLT_PW, B->ph[sd], hex, 1);
        rect(x + (sd ? 3 : 1), y + 3, 2, B->ph[sd] - 6, 0xFFFFFF, 0.35f);
        float ex = sd ? x - 1 : x + CK_BOLT_PW + 1;
        rect(sd ? x - 3 : x + CK_BOLT_PW, B->py[sd] - 5, 3, 10, hex, 1);   // its face
        shape(SPR_DOT, ex, B->py[sd] - 2, 1.2f, B->who[sd] < 0 ? 0xFF5A5A : 0x1B1426, 1);
        shape(SPR_DOT, ex, B->py[sd] + 2, 1.2f, B->who[sd] < 0 ? 0xFF5A5A : 0x1B1426, 1);
    }
    if (B->state == BOLT_PLAY || (B->state == BOLT_SERVE && (B->stateT / 8) % 2)) {
        uint32_t c = B->rally >= 8 ? 0xFF8A3D : 0xFFD23F;
        for (int k = 3; k >= 1; k--) shape(SPR_DOT, B->bx - B->vx * k * 1.5f, B->by - B->vy * k * 1.5f, CK_BOLT_R - k * 0.6f, c, 0.12f * (4 - k));
        shape(SPR_DOT, B->bx, B->by, CK_BOLT_R, c, 1);
        shape(SPR_DOT, B->bx - 1.2f, B->by - 1.2f, 1.3f, 0xFFF6D8, 1);
    }
    particles(G, 0);
    if (B->state != BOLT_READY) {
        txt(F_SMALL, B->who[0] < 0 ? ST_GREY : Hs->gs[B->who[0]].slot ? ST_BLUE : ST_YELLOW, 10, 15, ALIGN_LEFT, "%s", bolt_name(G, 0, a, sizeof a));
        txt(F_SMALL, B->who[1] < 0 ? ST_GREY : Hs->gs[B->who[1]].slot ? ST_BLUE : ST_YELLOW, W - 10, 15, ALIGN_RIGHT, "%s", bolt_name(G, 1, b, sizeof b));
        txt(F_MED, ST_CREAM, W / 2 - 14, 17, ALIGN_RIGHT, "%d", B->score[0]);
        txt(F_MED, ST_CREAM, W / 2 + 14, 17, ALIGN_LEFT, "%d", B->score[1]);
    }
    if (B->state == BOLT_READY) {
        banner(F_BIG, ST_YELLOW, 0xFFD23F, 54, 180, 34, 77, "BOLT BALL");
        hint(104, "First to 7 points wins");
        for (int i = 0; i < Hs->np; i++) {
            float cx = Hs->np == 1 ? W / 2 : i ? 240 : 80;
            int st = Hs->gs[i].slot ? ST_BLUE : ST_YELLOW;
            panel(cx - 56, 126, 112, 40, 0x1B1426, 0.85f);
            txt(F_MED, st, cx, 143, ALIGN_CENTER, "P%d", Hs->gs[i].slot + 1);
            txt(F_SMALL, B->ready[i] ? ST_GREEN : ST_CREAM, cx, 159, ALIGN_CENTER, "%s", B->ready[i] ? "READY!" : "A to play");
        }
        if (Hs->np == 2 && B->readyT > 0) {
            snprintf(s, sizeof s, "P%d vs the computer in %d...", Hs->gs[B->ready[0] ? 0 : 1].slot + 1, B->readyT / 60 + 1);
            hint(186, s);
        } else if (Hs->np == 1) hint(186, "You vs the computer");
        hint(H - 6, "A ready   START leave");
        return;
    }
    if (B->state == BOLT_WON) {
        snprintf(s, sizeof s, B->who[B->win] >= 0 && Hs->np == 1 ? "YOU WIN!" : "%s WINS!", bolt_name(G, B->win, a, sizeof a));
        banner(F_BIG, ST_YELLOW, 0xFFD23F, 80, 160, 34, 103, s);
        snprintf(s, sizeof s, "%d to %d", B->score[B->win], B->score[!B->win]);
        hint(134, s);
        if (B->stateT > 60) hint(152, "A play again   START leave");
        return;
    }
    hint(H - 6, "Stick up and down   START leave");
}

// ---- Jet Run: five sceneries, each with its own sky, far and near layers, ground and pipes;
// web/arcade.js runScene draws the same
static float run_hash(int k) { uint32_t x = (uint32_t)k * 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; return (x & 1023) / 1023.0f; }
static uint32_t mixc(uint32_t a, uint32_t b, float t) {
    int r = ((a >> 16) & 255) + (((int)((b >> 16) & 255) - (int)((a >> 16) & 255)) * t);
    int g = ((a >> 8) & 255) + (((int)((b >> 8) & 255) - (int)((a >> 8) & 255)) * t);
    int bl = (a & 255) + (((int)(b & 255) - (int)(a & 255)) * t);
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)bl;
}
static void run_sky(uint32_t top, uint32_t bot) { for (int k = 0; k < 12; k++) rect(0, k * 18, W, 18, mixc(top, bot, k / 11.0f), 1); }
// x of thing k in a layer that repeats every span px, scrolled at par times the run
static float run_lx(int k, float step, float sc, float par, float span) { return fmodf(fmodf(k * step - sc * par, span) + span, span) - 60; }
typedef struct { uint32_t body, light, dark, lip, accent; } run_pal;
static const run_pal run_pals[CK_RUN_THEMES] = {
    { 0x7D9A8A, 0xA5C4B2, 0x5F7A6C, 0x8FB09C, 0xC0703A },   // junk pipes
    { 0xB0663A, 0xE0925A, 0x7A4026, 0xC9784A, 0xFFD23F },   // sunset factory
    { 0xFF8FC7, 0xFFFFFF, 0xD0609A, 0xFFB8DC, 0x5AD1FF },   // candy clouds
    { 0xA8D8F0, 0xE8F8FF, 0x78A8C8, 0xC8E8F8, 0xFFFFFF },   // ice peaks
    { 0x2A2A48, 0x5AD1FF, 0x18182E, 0x3A3A60, 0xFF5AC8 },   // neon night
    { 0x6A7A8E, 0x9AAABE, 0x4A5868, 0x7A8AA0, 0xFFD23F },   // storm clouds
    { 0x5A4A48, 0x8A7A70, 0x3A2E2C, 0x6A5A56, 0xFF6A2A },   // lava caves
    { 0xD8DCE6, 0xFFFFFF, 0x9AA0B0, 0xE6E9F0, 0xFF5A5A },   // star lanes
    { 0x2A1A3A, 0x49F08A, 0x1A0E28, 0x3A2A50, 0xC28BFF },   // glitch zone
};
enum { TH_JUNK, TH_SUNSET, TH_CANDY, TH_ICE, TH_NEON, TH_STORM, TH_LAVA, TH_STARS, TH_GLITCH };
// a mountain or a stalactite out of stacked strips: w wide at its base y, h tall, pointing up (or down)
static void run_tri(float xc, float y, float w, float h, uint32_t col, bool down) {
    for (int k = 0; k < 8; k++) {
        float ww = w * (1 - k / 8.0f), hh = h / 8;
        rect(xc - ww / 2, down ? y + k * hh : y - (k + 1) * hh, ww, hh + 0.5f, col, 1);
    }
}
static void run_bg(int th, float sc, float t) {
    switch (th) {
    case 0:   // junk pipes: the zone's own blue sky, clouds and hills
        blit(SPR_JET_SKY, 0, 0, 0);
        for (int k = 0; k < 5; k++) blit(SPR_CLOUD, 0, run_lx(k, 110, sc, 0.15f, 550), 26 + (k % 3) * 24);
        { float ho = fmodf(fmodf(sc * 0.4f, 640) + 640, 640); blit(SPR_JET_HILLS, 0, -ho, CK_RUN_FLOOR - 46); if (ho > 320) blit(SPR_JET_HILLS, 0, 640 - ho, CK_RUN_FLOOR - 46); }
        for (int k = 0; k < 8; k++) { float x = run_lx(k, 70, sc, 0.7f, 560); rect(x, CK_RUN_FLOOR - 10 - run_hash(k) * 8, 14, 12 + run_hash(k) * 8, 0x6F5A48, 1); rect(x + 3, CK_RUN_FLOOR - 6 - run_hash(k) * 8, 6, 3, 0x9A8A70, 1); }   // junk heaps
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0x8A5233, 1); rect(0, CK_RUN_FLOOR, W, 3, 0xC98B55, 1);
        for (int k = 0; k < 12; k++) rect(run_lx(k, 32, sc, 1, 384), CK_RUN_FLOOR + 8 + (k % 3) * 5, 6, 2, 0x6F4026, 1);
        break;
    case 1:   // sunset factory: an orange sky, a low sun, two rows of smoking factories
        run_sky(0x5A3A8A, 0xFFA060);
        shape(SPR_DOT, 232, 150, 44, 0xFFC27A, 0.35f);
        shape(SPR_DOT, 232, 150, 30, 0xFFE08A, 1);
        for (int k = 0; k < 7; k++) {
            float x = run_lx(k, 90, sc, 0.18f, 630), h = 40 + run_hash(k) * 50, w = 50 + run_hash(k + 9) * 30;
            rect(x, CK_RUN_FLOOR - h, w, h, 0x7A3A5A, 1);
            rect(x + w * 0.6f, CK_RUN_FLOOR - h - 26, 8, 26, 0x7A3A5A, 1);
        }
        for (int k = 0; k < 6; k++) {
            float x = run_lx(k, 120, sc, 0.45f, 720), h = 30 + run_hash(k + 30) * 40, w = 60 + run_hash(k + 40) * 30;
            rect(x, CK_RUN_FLOOR - h, w, h, 0x4A2440, 1);
            for (int j = 0; j < 3; j++) rect(x + 8 + j * 16, CK_RUN_FLOOR - h + 8, 8, 6, (j + k) % 3 ? 0xFFB050 : 0x6A3050, 1);   // lit windows
            float cx = x + w - 14, cy = CK_RUN_FLOOR - h - 34;
            rect(cx, cy, 10, 34, 0x3A1A34, 1); rect(cx - 1, cy, 12, 3, 0xC0503A, 1);
            for (int j = 0; j < 4; j++) {   // smoke puffs rising off the stack
                float u = fmodf(t * 0.5f + j * 0.25f + run_hash(k) , 1);
                shape(SPR_DOT, cx + 5 - u * 18, cy - 4 - u * 40, 4 + u * 7, 0xC8A0B0, 0.55f * (1 - u));
            }
        }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0x3A2030, 1); rect(0, CK_RUN_FLOOR, W, 2, 0xFF9A5A, 1);
        for (int k = 0; k < 14; k++) rect(run_lx(k, 28, sc, 1, 392), CK_RUN_FLOOR + 6, 14, 4, 0x5A3048, 1);   // rail sleepers
        rect(0, CK_RUN_FLOOR + 5, W, 2, 0x9A6A6A, 1); rect(0, CK_RUN_FLOOR + 11, W, 2, 0x9A6A6A, 1);
        break;
    case TH_NEON:   // neon night: stars, a moon, a city with lit windows and glowing signs
        run_sky(0x08081E, 0x3A1A5A);
        for (int k = 0; k < 40; k++) shape(SPR_DOT, run_lx(k, 37, sc, 0.03f, 400), 6 + run_hash(k) * 110, 0.6f + run_hash(k + 3) * 0.8f, 0xFFFFFF, 0.4f + 0.6f * fabsf(sinf(t * 2 + k)));
        shape(SPR_DOT, 60, 44, 16, 0xFFF6D8, 0.2f); shape(SPR_DOT, 60, 44, 12, 0xF0E8D0, 1); shape(SPR_DOT, 56, 41, 2.5f, 0xD8D0B8, 1); shape(SPR_DOT, 64, 48, 1.8f, 0xD8D0B8, 1);
        for (int k = 0; k < 9; k++) {
            float x = run_lx(k, 64, sc, 0.2f, 576), h = 60 + run_hash(k + 50) * 70;
            rect(x, CK_RUN_FLOOR - h, 46, h, 0x1C1C40, 1);
            for (int wy = 0; wy < h - 10; wy += 10) for (int wx = 0; wx < 4; wx++) if (run_hash(k * 97 + wy + wx * 13) > 0.55f) rect(x + 5 + wx * 10, CK_RUN_FLOOR - h + 6 + wy, 4, 4, 0xFFE066, 0.7f);
        }
        for (int k = 0; k < 7; k++) {
            float x = run_lx(k, 100, sc, 0.5f, 700), h = 40 + run_hash(k + 70) * 50;
            rect(x, CK_RUN_FLOOR - h, 70, h, 0x101028, 1);
            uint32_t neon = k % 2 ? 0xFF5AC8 : 0x5AD1FF;
            float gl = 0.6f + 0.4f * sinf(t * 4 + k * 1.7f);
            rect(x + 8, CK_RUN_FLOOR - h + 10, 54, 3, neon, gl);   // a sign
            rect(x + 8, CK_RUN_FLOOR - h + 18, 30, 3, neon, gl * 0.8f);
            rect(x, CK_RUN_FLOOR - h, 70, 2, neon, 0.5f);
        }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0x14142A, 1); rect(0, CK_RUN_FLOOR, W, 2, 0x5AD1FF, 1);
        for (int k = 0; k < 10; k++) rect(run_lx(k, 40, sc, 1, 400), CK_RUN_FLOOR + 12, 20, 2, 0xFFE066, 0.8f);   // road markings
        break;
    case TH_STORM: {   // storm clouds: grey sky, rolling clouds, rain and lightning
        float cyc = fmodf(t, 4.3f);
        run_sky(0x2E3440, 0x6A7488);
        if (cyc < 0.25f) {   // lightning
            rect(0, 0, W, H, 0xFFFFFF, 0.5f * (1 - cyc / 0.25f));
            float lx = 40 + run_hash((int)(t / 4.3f)) * 240, ly = 0;
            for (int s = 0; s < 6; s++) { float nx = lx + (run_hash((int)(t / 4.3f) * 7 + s) - 0.5f) * 30, ny = ly + 22; line(lx, ly, nx, ny, 2.5f, 0xFFF6A0, 1); lx = nx; ly = ny; }
        }
        for (int k = 0; k < 8; k++) { float x = run_lx(k, 80, sc, 0.12f, 640); shape(SPR_DOT, x, 40 + run_hash(k) * 30, 30 + run_hash(k + 5) * 12, 0x4A5262, 1); }
        for (int k = 0; k < 7; k++) { float x = run_lx(k, 100, sc, 0.3f, 700); shape(SPR_DOT, x, 8 + run_hash(k + 20) * 14, 34, 0x2A303C, 1); shape(SPR_DOT, x + 30, 2, 28, 0x2A303C, 1); }
        for (int k = 0; k < 9; k++) { float x = run_lx(k, 72, sc, 0.55f, 648); shape(SPR_DOT, x, CK_RUN_FLOOR + 4, 26 + run_hash(k + 40) * 10, 0x3A4250, 1); }   // dark hills
        for (int k = 0; k < 50; k++) {   // rain
            float rx = fmodf(k * 53 - t * 90 - sc * 0.8f + 4000, 340) - 10, ry = fmodf(k * 31 + t * 260, 240);
            line(rx, ry, rx - 3, ry + 9, 1.5f, 0xB8C8E0, 0.7f);
        }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0x2A3038, 1); rect(0, CK_RUN_FLOOR, W, 2, 0x8A9AB0, 1);
        for (int k = 0; k < 6; k++) rect(run_lx(k, 70, sc, 1, 420), CK_RUN_FLOOR + 8 + (k % 2) * 6, 26, 2, 0x8A9AB0, 0.5f);   // puddles
        break; }
    case TH_CANDY:   // candy clouds: a pink sky, puffy clouds, candy hills and lollipops
        run_sky(0xFFB0D8, 0xFFE8C8);
        for (int k = 0; k < 6; k++) { float x = run_lx(k, 100, sc, 0.12f, 600), y = 30 + run_hash(k) * 50; shape(SPR_DOT, x, y, 16, 0xFFFFFF, 0.9f); shape(SPR_DOT, x + 16, y - 6, 18, 0xFFF0F8, 0.9f); shape(SPR_DOT, x + 32, y, 14, 0xFFFFFF, 0.9f); }
        for (int k = 0; k < 7; k++) { float x = run_lx(k, 90, sc, 0.3f, 630); shape(SPR_DOT, x, CK_RUN_FLOOR + 10, 40 + run_hash(k + 4) * 16, k % 2 ? 0xC28BFF : 0xFF8FC7, 1); shape(SPR_DOT, x - 10, CK_RUN_FLOOR - 18, 8, 0xFFFFFF, 0.5f); }
        for (int k = 0; k < 6; k++) {   // lollipops
            float x = run_lx(k, 110, sc, 0.6f, 660), h = 40 + run_hash(k + 9) * 30;
            rect(x - 1.5f, CK_RUN_FLOOR - h, 3, h, 0xFFFFFF, 1);
            shape(SPR_DOT, x, CK_RUN_FLOOR - h, 12, k % 3 == 0 ? 0xFF5A8A : k % 3 == 1 ? 0x5AD1FF : 0xFFD23F, 1);
            shape(SPR_DOT, x - 2, CK_RUN_FLOOR - h - 2, 6, 0xFFFFFF, 0.6f); shape(SPR_DOT, x - 2, CK_RUN_FLOOR - h - 2, 3, k % 3 == 0 ? 0xFF5A8A : k % 3 == 1 ? 0x5AD1FF : 0xFFD23F, 1);
        }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0xFFF0F6, 1);
        for (int k = 0; k < 16; k++) rect(run_lx(k, 24, sc, 1, 384), CK_RUN_FLOOR, 12, H - CK_RUN_FLOOR, 0xFF8FC7, 1);   // candy cane stripes
        rect(0, CK_RUN_FLOOR, W, 3, 0xFFFFFF, 1);
        break;
    case TH_ICE:   // ice peaks: snowy mountains, far and near, and snow falling
        run_sky(0x5A98D8, 0xD8F0FF);
        for (int k = 0; k < 6; k++) { float x = run_lx(k, 110, sc, 0.15f, 660) + 50, w = 120 + run_hash(k) * 60, h = 90 + run_hash(k + 3) * 50; run_tri(x, CK_RUN_FLOOR, w, h, 0xA8C8E8, false); run_tri(x, CK_RUN_FLOOR - h * 0.7f, w * 0.3f, h * 0.3f, 0xFFFFFF, false); }
        for (int k = 0; k < 6; k++) { float x = run_lx(k, 130, sc, 0.4f, 780) + 50, w = 100 + run_hash(k + 20) * 50, h = 60 + run_hash(k + 23) * 40; run_tri(x, CK_RUN_FLOOR, w, h, 0x7898C0, false); run_tri(x, CK_RUN_FLOOR - h * 0.72f, w * 0.28f, h * 0.28f, 0xF0F8FF, false); }
        for (int k = 0; k < 40; k++) { float fx = fmodf(k * 41 - t * 20 - sc * 0.5f + 4000, 330) - 5, fy = fmodf(k * 29 + t * (25 + (k % 3) * 10), 240); shape(SPR_DOT, fx + sinf(t * 2 + k) * 3, fy, 1 + (k % 3) * 0.5f, 0xFFFFFF, 0.9f); }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0xF0F8FF, 1); rect(0, CK_RUN_FLOOR, W, 3, 0xFFFFFF, 1);
        for (int k = 0; k < 8; k++) { float x = run_lx(k, 50, sc, 1, 400); rect(x, CK_RUN_FLOOR + 8 + (k % 2) * 6, 18, 2, 0xC8E0F0, 1); }
        break;
    case TH_LAVA:   // lava caves: stalactites, a red glow, embers rising off a lava floor
        run_sky(0x140606, 0x4A1A10);
        for (int k = 0; k < 6; k++) shape(SPR_DOT, run_lx(k, 110, sc, 0.1f, 660), 120 + run_hash(k) * 60, 40, 0xFF5A2A, 0.12f);
        for (int k = 0; k < 9; k++) { float x = run_lx(k, 50, sc, 0.25f, 450); run_tri(x, 0, 30 + run_hash(k) * 20, 40 + run_hash(k + 2) * 40, 0x2A1410, true); }
        for (int k = 0; k < 7; k++) { float x = run_lx(k, 80, sc, 0.5f, 560); run_tri(x, 0, 24 + run_hash(k + 30) * 14, 24 + run_hash(k + 31) * 30, 0x3A1E18, true); run_tri(x + 20, CK_RUN_FLOOR, 30, 30 + run_hash(k + 33) * 30, 0x3A1E18, false); }
        for (int k = 0; k < 24; k++) { float u = fmodf(t * 0.3f + run_hash(k), 1); shape(SPR_DOT, run_lx(k, 31, sc, 0.8f, 380) + sinf(t * 3 + k) * 4, CK_RUN_FLOOR - u * 200, 1.2f, k % 2 ? 0xFFB040 : 0xFF6A2A, 1 - u); }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0xFF6A2A, 1);
        for (int x = 0; x < W; x += 8) rect(x, CK_RUN_FLOOR - 2 + sinf(x * 0.15f + t * 3 + sc * 0.05f) * 2, 8, 4, 0xFFB040, 1);
        for (int k = 0; k < 6; k++) { float u = fmodf(t * 0.8f + run_hash(k + 50), 1); shape(SPR_DOT, run_lx(k, 64, sc, 1, 384), CK_RUN_FLOOR + 10, 2 + u * 3, 0xFFD23F, 1 - u); }   // bubbles
        break;
    case TH_GLITCH: {   // glitch zone: a purple grid, glitching blocks, a digital floor
        rect(0, 0, W, H, 0x0E0618, 1);
        for (int y = 10; y < CK_RUN_FLOOR; y += 20) rect(0, y, W, 1, 0x2A1240, 1);
        for (int k = 0; k < 12; k++) rect(run_lx(k, 32, sc, 0.5f, 384) + 60, 0, 1, CK_RUN_FLOOR, 0x2A1240, 1);
        int tick = (int)(t * 8);
        for (int k = 0; k < 10; k++) {   // blocks that flicker in and out
            float h1 = run_hash(tick * 13 + k), h2 = run_hash(tick * 7 + k * 3);
            if (h1 < 0.5f) continue;
            rect(h2 * 300, run_hash(k * 5 + tick) * 200, 10 + h1 * 30, 3 + h2 * 6, k % 2 ? 0x49F08A : 0xFF5AC8, 0.35f);
        }
        for (int k = 0; k < 8; k++) { float x = run_lx(k, 80, sc, 0.35f, 640); rect(x, CK_RUN_FLOOR - 50 - run_hash(k) * 40, 40, 90, 0x1A0E28, 1); rect(x + 4, CK_RUN_FLOOR - 46 - run_hash(k) * 40, 32, 2, 0x49F08A, 0.5f + 0.5f * sinf(t * 5 + k)); }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0x1A0E28, 1); rect(0, CK_RUN_FLOOR, W, 2, 0x49F08A, 1);
        for (int k = 0; k < 14; k++) rect(run_lx(k, 28, sc, 1, 392), CK_RUN_FLOOR + 6 + (k % 3) * 5, 6, 4, k % 2 ? 0xC28BFF : 0x49F08A, 0.6f);
        if (run_hash(tick) > 0.9f) rect(0, run_hash(tick + 1) * 200, W, 6, 0xFFFFFF, 0.15f);   // a glitch flicker
        break; }
    case TH_STARS:   // star lanes: deep space, three layers of stars, a ringed planet, a moon to fly over
        rect(0, 0, W, H, 0x05030F, 1);
        { float mx = 70 - fmodf(sc * 0.01f, 400); shape(SPR_DOT, mx, 150, 9, 0x8FA0C0, 1); shape(SPR_DOT, mx - 2, 148, 3, 0xB8C8E0, 1); }   // a little far moon
        for (int L = 0; L < 3; L++) for (int k = 0; k < 24; k++) shape(SPR_DOT, run_lx(k + L * 40, 23 + L * 7, sc, 0.04f + L * 0.12f, 400), run_hash(k * 3 + L * 100) * 200, 0.5f + L * 0.4f, L == 2 ? 0xFFFFFF : 0xB8C0E0, 0.5f + L * 0.25f);
        { float px = 250 - fmodf(sc * 0.02f, 400); shape(SPR_DOT, px, 62, 26, 0xC98A4B, 1); shape(SPR_DOT, px - 7, 55, 9, 0xE0A86A, 1); rect(px - 42, 61, 84, 3, 0xF0D8B0, 0.8f); }
        rect(0, CK_RUN_FLOOR, W, H - CK_RUN_FLOOR, 0x8F8AA8, 1); rect(0, CK_RUN_FLOOR, W, 2, 0xC0BCD4, 1);
        for (int k = 0; k < 8; k++) { float x = run_lx(k, 52, sc, 1, 416); shape(SPR_DOT, x, CK_RUN_FLOOR + 10 + (k % 2) * 6, 5 + (k % 3) * 2, 0x6E6A80, 1); }
        break;
    }
}
// one pipe from y0 to y1 with its lip at lipY
static void run_pipe(float x, float y0, float y1, float lipY, const run_pal *P, int th, float t) {
    if (y1 <= y0) return;
    rect(x + 2, y0, CK_RUN_W - 4, y1 - y0, P->body, 1);
    rect(x + 5, y0, 3, y1 - y0, P->light, th == TH_NEON || th == TH_GLITCH ? 0.8f : 1);
    rect(x + CK_RUN_W - 8, y0, 4, y1 - y0, P->dark, 1);
    rect(x - 2, lipY, CK_RUN_W + 4, 10, P->lip, 1);
    rect(x - 2, lipY, CK_RUN_W + 4, 2, P->light, 0.7f);
    rect(x + 6, lipY + 4, 8, 3, P->accent, th == TH_STARS || th == TH_GLITCH ? (sinf(t * 5 + x) > 0 ? 1 : 0.3f) : 1);
    shape(SPR_DOT, x + 2, lipY + 5, 1.3f, P->dark, 1); shape(SPR_DOT, x + CK_RUN_W - 2, lipY + 5, 1.3f, P->dark, 1);
}
static void draw_jetrun(const ck_game *G) {
    const ck_house *Hs = &G->house;
    const ck_jetrun *J = &Hs->run;
    int th = J->stage % CK_RUN_THEMES;
    const run_pal *P = &run_pals[th];
    float sc = J->scroll + (J->shake > 0 ? sinf(J->shake * 2.3f) * 3 : 0), t = J->t;
    char s[40];
    run_bg(th, sc, t);
    for (int k = 0; k < CK_RUN_COLS; k++) {
        const ck_runcol *c = &J->col[k];
        float x = c->x - sc;
        if (x < -40 || x > W + 10) continue;
        float gy = ck_run_gap_y(J, c);
        if (c->kind == RUNCOL_PILLAR) {   // a stack of junk blocks from the ground
            for (float y = gy; y < CK_RUN_FLOOR; y += 14) {
                int j = (int)((y - gy) / 14);
                rect(x + (j % 2), y, CK_RUN_W - (j % 2) * 2, 13, j % 2 ? P->lip : P->dark, 1);
                rect(x + 6, y + 4, CK_RUN_W - 12, 4, j % 2 ? P->light : P->body, 1);
            }
            rect(x - 2, gy - 3, CK_RUN_W + 4, 4, P->accent, 1);
        } else if (c->kind == RUNCOL_GEAR) {   // a spinning gear
            float mx = ck_run_mid(c) - sc, a = t * 2.5f + c->ph;
            shape(SPR_DOT, mx, gy, CK_RUN_GEAR_R - 3, P->dark, 1);
            for (int k = 0; k < 8; k++) shape(SPR_DOT, mx + cosf(a + k * 0.785f) * (CK_RUN_GEAR_R - 3), gy + sinf(a + k * 0.785f) * (CK_RUN_GEAR_R - 3), 4, P->body, 1);
            shape(SPR_DOT, mx, gy, CK_RUN_GEAR_R - 7, P->body, 1);
            for (int k = 0; k < 3; k++) line(mx, gy, mx + cosf(a + k * 2.09f) * 10, gy + sinf(a + k * 2.09f) * 10, 2, P->dark, 1);
            shape(SPR_DOT, mx, gy, 4, P->accent, 1);
        } else if (c->kind == RUNCOL_DRONE) {   // a glitch drone: a body, two eyes, a whirring rotor
            float mx = ck_run_mid(c) - sc;
            shape(SPR_DOT, mx, gy, CK_RUN_DRONE_R, 0xC23A6A, 1);
            shape(SPR_DOT, mx - 1, gy + 1, CK_RUN_DRONE_R - 3, 0xFF5A8A, 1);
            shape(SPR_DOT, mx - 4, gy - 1, 2.5f, 0xFFFFFF, 1); shape(SPR_DOT, mx + 2, gy - 1, 2.5f, 0xFFFFFF, 1);
            shape(SPR_DOT, mx - 5, gy - 1, 1.2f, 0x1B1426, 1); shape(SPR_DOT, mx + 1, gy - 1, 1.2f, 0x1B1426, 1);
            rect(mx - 1, gy - CK_RUN_DRONE_R - 4, 2, 4, 0x9AA1B0, 1);
            float rw = 6 + 6 * fabsf(sinf(t * 30 + c->ph));
            rect(mx - rw, gy - CK_RUN_DRONE_R - 5, rw * 2, 2, 0xE6E6F0, 1);
        } else {
            float gap = ck_run_gap(J, c), top = c->kind == RUNCOL_HANG ? gy : gy - gap / 2;
            run_pipe(x, 0, top, top - 10, P, th, t);
            if (c->kind != RUNCOL_HANG) run_pipe(x, gy + gap / 2, CK_RUN_FLOOR, gy + gap / 2, P, th, t);
            if (c->kind == RUNCOL_CRUSH)   // hazard stripes on the crushing ends
                for (int k = 0; k < 4; k++) { rect(x - 2 + k * 8, top - 10, 4, 10, 0xFFD23F, 1); rect(x - 2 + k * 8, gy + gap / 2, 4, 10, 0xFFD23F, 1); }
            if (c->amp) { shape(SPR_DOT, x + CK_RUN_W / 2, top - 5, 2, 0x5AD1FF, 0.6f + 0.4f * sinf(t * 8)); }   // a swaying gap's motor light
        }
    }
    for (int k = 0; k < CK_RUN_BOLTS; k++) {
        const ck_runbolt *b = &J->bolt[k];
        if (!b->on) continue;
        float x = b->x - sc;
        if (x < -10 || x > W + 10) continue;
        blit_o(SPR_BOLT, 0, x, b->y, (bopt){ .sx = fabsf(cosf(t * 5 + k)) * 0.8f + 0.2f, .sy = 1 });
    }
    if (J->speed > 2.2f && J->state == RUN_PLAY)   // speed lines
        for (int k = 0; k < 10; k++) { float lx = fmodf(k * 97 - J->scroll * 3 + 4000, 360) - 20; rect(lx, 20 + run_hash(k) * 180, 24 + (k % 3) * 10, 1.5f, 0xFFFFFF, 0.25f); }
    particles(G, sc);
    for (int i = 0; i < Hs->np; i++) {
        const __typeof__(J->ps[0]) *p = &J->ps[i];
        if (J->state == RUN_OVER || (J->state == RUN_PLAY && !p->on)) continue;
        if (p->inv > 0 && (p->inv / 4) % 2 == 0) continue;
        int slot = Hs->gs[i].slot, hero = G->prog.hero[slot], skin = skin_col(G, slot);
        int jf = 29 + (int)(t * 14) % 2;
        float fx = p->x, fy = p->y + CK_RUN_HH + 4;
        blit_o(hero_sprite(hero, skin), jf, fx, fy, skin_o((bopt){ 0 }, skin, t + i));
        hero_acc(wear_of(G, slot), hero, jf, fx, fy, 1, false, t);
    }
    // the stage's name swoops in, with a flash as the scenery changes
    if (J->stageT > 120 && J->state == RUN_PLAY && J->stage > 0) rect(0, 0, W, H, 0xFFFFFF, (J->stageT - 120) / 30.0f * 0.7f);
    if (J->stageT > 0 && J->state == RUN_PLAY) {
        float k = J->stageT > 130 ? (150 - J->stageT) / 20.0f : J->stageT < 20 ? J->stageT / 20.0f : 1;
        snprintf(s, sizeof s, "STAGE %d", J->stage + 1);
        dark_panel(W / 2 - 80, 78 - (1 - k) * 30, 160, 42);
        txt(F_SMALL, ST_CREAM, W / 2, 94 - (1 - k) * 30, ALIGN_CENTER, "%s", s);
        txt(F_MED, ST_YELLOW, W / 2, 112 - (1 - k) * 30, ALIGN_CENTER, "%s", ck_run_stage_names[th]);
    }
    if (J->lifeT > 0) banner(F_MED, ST_GREEN, 0x49D17A, 40, 120, 20, 54, "EXTRA LIFE!");
    dark_panel(4, 4, 62, 15);
    blit_o(SPR_BOLT, 0, 13, 11.5f, (bopt){ .sx = 4 / 4.2f, .sy = 4 / 4.2f });
    txt(F_MED, ST_PLAIN, 20, 16, ALIGN_LEFT, "x%d", J->bolts);
    txt(F_SMALL, ST_YELLOW, W / 2, 12, ALIGN_CENTER, "SCORE %d", J->score);
    txt(F_SMALL, ST_GREEN, W / 2, 22, ALIGN_CENTER, "BEST %d", J->best);
    for (int k = 0; k < CK_RUN_LIVES; k++) {   // the team's lives: little batteries
        float x = W - 18 - k * 16;
        rect(x, 6, 12, 8, 0x1B1426, 1); rect(x + 12, 8, 2, 4, 0x1B1426, 1);
        if (k < J->lives) rect(x + 1.5f, 7.5f, 9, 5, 0x49D17A, 1);
    }
    if (J->state == RUN_READY) {
        banner(F_BIG, ST_YELLOW, 0xFFD23F, 50, 170, 34, 73, "JET RUN");
        hint(102, "Up or A fly up   Down go down   Left and right too");
        for (int i = 0; i < Hs->np; i++) {
            float cx = Hs->np == 1 ? W / 2 : i ? 230 : 90;
            dark_panel(cx - 56, 120, 112, 40);
            txt(F_MED, Hs->gs[i].slot ? ST_BLUE : ST_YELLOW, cx, 137, ALIGN_CENTER, "P%d", Hs->gs[i].slot + 1);
            txt(F_SMALL, J->ps[i].on ? ST_GREEN : ST_CREAM, cx, 153, ALIGN_CENTER, "%s", J->ps[i].on ? "READY!" : "A to fly");
        }
        if (Hs->np == 2 && J->readyT > 0) { snprintf(s, sizeof s, "Taking off in %d...", J->readyT / 60 + 1); hint(180, s); }
        hint(H - 6, "A join   START leave");
    } else if (J->state == RUN_OVER) game_over(J->score, J->best);
}

static void pond_fish(int kind, float x, float y, int dir, float t) {
    const ck_catch *c = &ck_catches[kind];
    if (kind == CATCH_BOOT) {
        rect(x - 5, y - 8, 7, 9, c->col, 1); rect(x - 5, y - 1, 13, 5, c->col, 1); rect(x - 6, y + 3, 15, 2, 0x3A2A20, 1);
        return;
    }
    float L = c->len, w = sinf(t * 8) * 1.2f, d = (float)dir;
    if (kind == CATCH_PIKE) shape(SPR_DOT, x - d * L * 0.45f, y, L * 0.4f, c->col, 1);   // a long body
    if (kind != CATCH_MINNOW && kind != CATCH_SARDINE) {   // fins on the bigger fish
        shape(SPR_DOT, x - d * L * 0.1f, y - L * 0.45f, L * 0.22f, c->col2, 1);
        shape(SPR_DOT, x - d * L * 0.1f, y + L * 0.42f, L * 0.16f, c->col2, 1);
    }
    shape(SPR_DOT, x - d * L * 0.75f, y + w, L * 0.4f, kind == CATCH_KOI || kind == CATCH_TROUT ? c->col2 : c->col, 1);   // tail
    shape(SPR_DOT, x, y, L * 0.55f, c->col, 1);
    shape(SPR_DOT, x + d * L * 0.25f, y, L * 0.4f, c->col, 1);
    switch (kind) {
    case CATCH_PERCH: for (int k = -1; k <= 1; k++) rect(x + d * k * L * 0.25f - 1, y - L * 0.4f, 2, L * 0.7f, c->col2, 1); break;   // stripes
    case CATCH_TROUT: rect(x - L * 0.5f, y - 1, L, 2.5f, c->col2, 1); break;   // the rainbow along its side
    case CATCH_KOI: shape(SPR_DOT, x - d * L * 0.15f, y - L * 0.15f, L * 0.25f, c->col2, 1); shape(SPR_DOT, x + d * L * 0.3f, y - L * 0.2f, L * 0.15f, c->col2, 1); break;
    case CATCH_CATFISH: line(x + d * L * 0.55f, y + 1, x + d * L * 0.95f, y + 4, 2, c->col2, 1); line(x + d * L * 0.55f, y + 2, x + d * L * 0.85f, y + 7, 2, c->col2, 1); break;   // whiskers
    case CATCH_PIKE: for (int k = 0; k < 3; k++) shape(SPR_DOT, x - d * (k * L * 0.3f), y - L * 0.1f, 1.8f, c->col2, 1); break;
    case CATCH_ROBO:   // a clockwork fish: rivets and a blinking antenna
        for (int k = -1; k <= 1; k++) shape(SPR_DOT, x + d * k * L * 0.25f, y + L * 0.15f, 1.2f, 0x5A5E6A, 1);
        rect(x - 0.5f, y - L * 0.75f, 1.5f, L * 0.3f, 0x5A5E6A, 1);
        shape(SPR_DOT, x, y - L * 0.78f, 2, sinf(t * 6) > 0 ? c->col2 : 0x2A3A4A, 1); break;
    case CATCH_KING: blit_o(SPR_STAR, 0, x + d * L * 0.15f, y - L * 0.55f, (bopt){ .sx = 0.5f, .sy = 0.5f, .tinted = true, .tint = hexc(c->col2, 255) }); break;   // its crown
    }
    shape(SPR_DOT, x + d * L * 0.35f, y - L * 0.12f, kind == CATCH_ROBO ? 2 : 1.4f, kind == CATCH_ROBO ? c->col2 : 0x1B1426, 1);
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
        line(hx + 6, 78, tx, ty, 4, 0x4A2E1C, 1);   // the rod, thick enough to read on a TV
        line(hx + 6, 78, tx, ty, 2, 0xB07A4C, 1);
        if (R->state == ROD_CHARGE) {
            dark_panel(hx - 14, 30 + i * 8, 30, 7);
            rect(hx - 13, 31 + i * 8, 28 * R->power, 5, R->power > 0.8f ? 0xFF5A5A : 0xFFD23F, 1);
        }
        if (R->state == ROD_FLY) line(tx, ty, R->x, R->y, 2, 0xFFF6D8, 1);
        if (R->state == ROD_WAIT || R->state == ROD_BITE || R->state == ROD_REEL) {
            float dip = R->state == ROD_BITE ? 4 + sinf(t * 30) * 2 : (R->fish >= 0 && R->t < 6) ? 2 : sinf(t * 3) * 0.8f;
            float by = CK_POND_TOP - 2 + dip;
            float bx = R->state == ROD_REEL ? R->bob : R->x;
            line(tx, ty, bx, by, 2, 0xFFF6D8, 1);
            line(bx, by, R->x, R->y, 2, 0xFFF6D8, 0.7f);
            shape(SPR_DOT, bx, by, 3, 0xFFFFFF, 1); shape(SPR_DOT, bx, by - 1.5f, 2, 0xFF5A5A, 1);
            shape(SPR_RING, R->x, R->y, 2.5f, 0xC9C4E0, 1);
            if (R->state == ROD_BITE) banner(F_BIG, ST_YELLOW, 0xFF5A5A, by - 40, 30, 26, by - 21, "!");
        }
        if (R->state == ROD_REEL) {   // how tight the line is: player one's top left, two's top right
            float bx = i ? W - 108 : 4;
            dark_panel(bx, 4, 104, 16);
            txt(F_SMALL, slot ? ST_BLUE : ST_YELLOW, bx + 4, 15, ALIGN_LEFT, "P%d", i + 1);
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
    snprintf(s, sizeof s, "Caught %d   Pond level %d", Pd->caught, Pd->level + 1);
    dark_panel(W / 2 - 62, 22, 124, 14); txt(F_SMALL, ST_CREAM, W / 2, 32, ALIGN_CENTER, "%s", s);
    if (Pd->levelT > 0) banner(F_MED, ST_YELLOW, 0xFFD23F, 150, 180, 22, 165, "Bigger fish in the pond!");
    const ck_rod *R0 = &Pd->rod[G->house.last];   // the help follows whoever pressed last
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

// ---------------------------------------------------------------------------- the sandbox
// The tank is drawn from the core's cells each frame: the CPU writes one 16-bit pixel a cell
// (two at a time) into a small texture, and the RDP draws it twice the size. Three textures take
// turns, so the RDP is never still reading the one being written (there are three framebuffers).
#define SAND_BUFS 3
static surface_t sand_surf[SAND_BUFS];
static uint16_t sand_lut[256];
static int sand_k = -1;
static void sand_free(void) {
    if (sand_k < 0) return;
    rspq_wait();
    for (int k = 0; k < SAND_BUFS; k++) surface_free(&sand_surf[k]);
    sand_k = -1;
}
static void sand_tank(const ck_sandbox *S) {
    if (sand_k < 0) {
        for (int k = 0; k < SAND_BUFS; k++) sand_surf[k] = surface_alloc(FMT_RGBA16, CK_SAND_W, CK_SAND_H);
        for (int c = 0; c < 256; c++) sand_lut[c] = color_to_packed16(hexc(ck_sand_hex(c & ~SAND_MOVED), 255));
        sand_k = 0;
    }
    surface_t *sf = &sand_surf[sand_k];
    sand_k = (sand_k + 1) % SAND_BUFS;
    const uint8_t *cell = S->cell;
    for (int y = 0; y < CK_SAND_H; y++) {
        uint32_t *px = (uint32_t *)((uint8_t *)sf->buffer + y * sf->stride);   // uncached: straight to RDRAM
        for (int x = 0; x < CK_SAND_W; x += 2, cell += 2) *px++ = (uint32_t)sand_lut[cell[0]] << 16 | sand_lut[cell[1]];
    }
    mode_tex();
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    rdpq_tex_blit(sf, CK_SAND_X, CK_SAND_Y, &(rdpq_blitparms_t){ .scale_x = CK_SAND_CELL, .scale_y = CK_SAND_CELL });
}
// a tool's swatch: the material's colour, or the rubber, the sway (its little level), the wipe
static void sand_swatch(int k, float x, float y, float r, float lean) {
    if (k <= SANDTOOL_CLAY) shape(SPR_DOT, x, y, r, ck_sand_hex((k + 1) | (8 << 4)), 1);
    else if (k == SANDTOOL_ERASE) { shape(SPR_DOT, x, y, r, 0xFFF6E6, 1); txt(F_SMALL, ST_RED, x, y + 3, ALIGN_CENTER, "x"); }
    else if (k == SANDTOOL_TILT) {
        shape(SPR_DOT, x, y, r, 0x8FD3FF, 1);
        float a = asinf(lean);   // gravity's arrow, pointing down the way it leans
        for (int i = -2; i <= 3; i++) shape(SPR_DOT, x + sinf(a) * i * r * 0.22f, y + cosf(a) * i * r * 0.22f, i == 3 ? 2.2f : 1.3f, 0x1B1426, 1);
    } else { shape(SPR_DOT, x, y, r, 0xE04F4F, 1); txt(F_SMALL, ST_WHITE, x, y + 3, ALIGN_CENTER, "ALL"); }
}
static void draw_sand(const ck_game *G) {
    const ck_house *Hs = &G->house;
    const ck_sandbox *S = &Hs->sand;
    int tw = CK_SAND_W * CK_SAND_CELL, th = CK_SAND_H * CK_SAND_CELL;
    rect(0, 0, W, H, 0x5DBB63, 1);   // the garden
    rect(CK_SAND_X - 4, CK_SAND_Y - 4, tw + 8, th + 8, 0x8A5A3C, 1);   // the sandbox's wooden frame
    rect(CK_SAND_X - 3, CK_SAND_Y - 3, tw + 6, 2, 0xC98A4B, 1);
    sand_tank(S);
    // the tools along the bottom, each player's ringed
    for (int k = 0; k < CK_SAND_TOOLS; k++) {
        float x = 30 + k * 44, y = 231;
        sand_swatch(k, x, y, 7, S->lean / CK_SAND_LEAN * 0.5f);
        if (k == SANDTOOL_TILT && S->still) rect(x - 8, y - 1, 16, 2, 0xE04F4F, 1);
        for (int j = 0; j < Hs->np; j++) if (S->cur[j].tool == k) shape(SPR_RING, x, y, 9 + j * 2, Hs->gs[j].slot ? 0x8FD3FF : 0xFFD23F, 1);
    }
    // each cursor: a ring the brush's size, and what it pours in the middle
    for (int i = 0; i < Hs->np; i++) {
        const ck_sandcur *c = &S->cur[i];
        uint32_t pc = Hs->gs[i].slot ? 0x8FD3FF : 0xFFD23F;
        int sl = Hs->gs[i].slot; bool erase = sl >= 0 && sl < 2 && (G->in.held[sl] & CK_B);
        float r = (ck_sand_radius(c->size) + 0.5f) * CK_SAND_CELL;
        if (c->tool <= SANDTOOL_ERASE || erase) {
            shape(SPR_RING, c->x, c->y, r + 2, 0x1B1426, 0.7f);
            shape(SPR_RING, c->x, c->y, r + 1, erase ? 0xFFF6E6 : pc, 1);
        }
        sand_swatch(erase ? SANDTOOL_ERASE : c->tool, c->x + r + 6, c->y - r - 6, 4.5f, S->lean / CK_SAND_LEAN * 0.5f);
    }
    particles(G, 0);
    int t = S->cur[Hs->last].tool;
    hint(11, t == SANDTOOL_TILT ? (S->still ? "A sway on   B rub out   L R pick   START leave" : "A sway off   B rub out   L R pick   START leave")
           : t == SANDTOOL_CLEAR ? "A wipe it all   B rub out   L R pick   START leave"
           : "A pour   B rub out   L R pick   C size   START leave");
}

void draw_frame(const ck_game *G, bool scanlines, bool interlaced) {
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
    case SC_PAUSE: draw_pause(G, scanlines, interlaced); break;
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
    case SC_BOLT: draw_bolt(G); break;
    case SC_JETRUN: draw_jetrun(G); break;
    case SC_SAND: draw_sand(G); break;
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
