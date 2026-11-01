// The game as a whole: scenes (title, player select, map, levels, pause), progress, particles,
// and the sound queue. The flow from web/game.js; drawing is left to each frontend.
#include <math.h>
#include <string.h>

#include "ck_internal.h"

const uint32_t ck_cols[CK_NCOLS] = { 0x5AD1FF, 0xFFD23F, 0xFF7A59, 0x49F08A, 0xC28BFF, 0xFF8FC7, 0x7FA7C9, 0xF5F1E8 };
const char *const ck_col_names[CK_NCOLS] = { "Sky", "Sunny", "Tangerine", "Lime", "Grape", "Bubblegum", "Steel", "Snow" };
const char *const ck_power_names[5] = { "", "MEGA BATTERY!", "RAINBOW OVERDRIVE!", "BUBBLE BLASTER!", "MAGNET HELMET!" };

void ck_prog_fresh(ck_prog *p) {
    memset(p, 0, sizeof *p);
    p->magic = CK_SAVE_MAGIC;
    p->lives = 3;
    p->little = 1;
    p->hero[0] = HERO_ROBOT; p->col[0] = 0; p->joined[0] = 1;
    p->hero[1] = HERO_OSTRICH; p->col[1] = 1; p->joined[1] = 0;
}

float ck_rand(ck_game *G) {
    // xorshift32
    uint32_t x = G->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    G->rng = x;
    return (float)(x >> 8) / 16777216.0f;
}

void ck_sfx(ck_game *G, int id) { if (G->nsfx < CK_MAXSFX) G->sfx[G->nsfx++] = (uint8_t)id; }
void ck_play(ck_game *G, int song) { G->song = song; }
void ck_save(ck_game *G) { G->progDirty = true; }

// ---------------------------------------------------------------------------- particles

static ck_part *part(ck_game *G) {
    if (G->nparts >= CK_MAXPARTS) return NULL;
    ck_part *p = &G->parts[G->nparts++];
    memset(p, 0, sizeof *p);
    return p;
}

void ck_puff(ck_game *G, float x, float y, int n, uint32_t col, uint8_t alpha) {
    for (int i = 0; i < n; i++) {
        ck_part *p = part(G);
        if (!p) return;
        p->x = x; p->y = y; p->vx = (ck_rand(G) - 0.5f) * 2; p->vy = -ck_rand(G) * 1.2f;
        p->life = 26 + (int)(ck_rand(G) * 10); p->max = 36; p->r = 2 + ck_rand(G) * 2;
        p->col = col; p->alpha = alpha; p->kind = PART_PUFF;
    }
}

void ck_sparks(ck_game *G, float x, float y, int n, uint32_t col) {
    for (int i = 0; i < n; i++) {
        ck_part *p = part(G);
        if (!p) return;
        float a = ck_rand(G) * CK_PI * 2, sp = 1 + ck_rand(G) * 2;
        p->x = x; p->y = y; p->vx = cosf(a) * sp; p->vy = sinf(a) * sp - 1;
        p->life = 30; p->max = 30; p->r = 2.5f; p->col = col; p->alpha = 255; p->kind = PART_STAR;
    }
}

void ck_confetti(ck_game *G, float x, float y, int n) {
    static const uint32_t c[6] = { 0xFF8FC7, 0x5AD1FF, 0xFFD23F, 0x49F08A, 0xC28BFF, 0xFF7A59 };
    for (int i = 0; i < n; i++) {
        ck_part *p = part(G);
        if (!p) return;
        p->x = x + (ck_rand(G) - 0.5f) * 200; p->y = y - ck_rand(G) * 60;
        p->vx = (ck_rand(G) - 0.5f) * 1.5f; p->vy = ck_rand(G) * 1.5f;
        p->life = 120 + (int)(ck_rand(G) * 60); p->max = 180; p->r = 2; p->col = c[i % 6]; p->alpha = 255;
        p->kind = PART_CONF; p->rot = ck_rand(G) * 6; p->g = 0.02f;
    }
}

void ck_debris(ck_game *G, float x, float y) {
    for (int i = 0; i < 8; i++) {
        ck_part *p = part(G);
        if (!p) return;
        p->x = x + ck_rand(G) * 12; p->y = y + ck_rand(G) * 12;
        p->vx = (ck_rand(G) - 0.5f) * 3; p->vy = -1 - ck_rand(G) * 3;
        p->life = 40; p->max = 40; p->r = 2.5f; p->col = i % 2 ? 0xB07A44 : 0x7D5128; p->alpha = 255;
        p->kind = PART_CHUNK; p->g = 0.25f;
    }
}

void ck_step_parts(ck_game *G) {
    int keep = 0;
    for (int i = 0; i < G->nparts; i++) {
        ck_part *p = &G->parts[i];
        p->x += p->vx; p->y += p->vy; p->vy += p->g;
        if (p->kind == PART_PUFF) { p->vx *= 0.92f; p->vy *= 0.92f; }
        if (p->kind == PART_CONF) p->rot += 0.2f;
        if (--p->life > 0) G->parts[keep++] = *p;
    }
    G->nparts = keep;
}

// ---------------------------------------------------------------------------- setup

bool ck_game_init(ck_game *G, const char *world_text, const ck_prog *saved) {
    memset(G, 0, sizeof *G);
    G->rng = 0x2545F491u;
    if (!ck_parse_world(world_text, &G->world)) return false;
    if (saved && saved->magic == CK_SAVE_MAGIC) G->prog = *saved;
    else ck_prog_fresh(&G->prog);
    G->prog.joined[0] = 1;
    if (G->prog.at >= G->world.nnodes) G->prog.at = 0;
    if (G->prog.lives < 1) G->prog.lives = 3;
    ck_map_reset(G);
    G->scene = SC_INTRO;
    G->song = SONG_LULLABY;
    return true;
}

bool ck_game_add_level(ck_game *G, const char *stem, const char *text) {
    bool any = false;
    for (int i = 0; i < G->world.nnodes; i++) {
        if (strcmp(G->world.nodes[i].level, stem)) continue;
        if (ck_parse_level(text, &G->defs[i])) { G->hasDef[i] = true; any = true; }
    }
    return any;
}

static void go(ck_game *G, int scene) {
    G->scene = scene;
    G->t = 0;
    switch (scene) {
    case SC_TITLE: ck_play(G, SONG_TITLE); break;
    case SC_SELECT: G->ready[0] = G->ready[1] = false; ck_play(G, SONG_SELECT); break;
    case SC_MAP:
        if (G->keepMap) G->keepMap = false;
        else ck_map_reset(G);
        ck_play(G, SONG_MAP);
        break;
    case SC_SHOP: ck_play(G, SONG_MAP); break;
    }
}

// ---------------------------------------------------------------------------- scenes

static void select_update(ck_game *G) {
    ck_prog *P = &G->prog;
    for (int i = 0; i < 2; i++) {
        if (!P->joined[i]) {
            if (HIT(G, i, CK_A) || HIT(G, i, CK_START)) { P->joined[i] = 1; ck_sfx(G, SFX_CONFIRM); }
            continue;
        }
        if (G->ready[i]) {
            if (HIT(G, i, CK_B)) { G->ready[i] = false; ck_sfx(G, SFX_SELECT); }
            continue;
        }
        if (HIT(G, i, CK_LEFT) || HIT(G, i, CK_RIGHT)) { P->hero[i] = P->hero[i] == HERO_ROBOT ? HERO_OSTRICH : HERO_ROBOT; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, i, CK_UP)) { P->col[i] = (uint8_t)((P->col[i] + CK_NCOLS - 1) % CK_NCOLS); ck_sfx(G, SFX_SELECT); }
        if (HIT(G, i, CK_DOWN)) { P->col[i] = (uint8_t)((P->col[i] + 1) % CK_NCOLS); ck_sfx(G, SFX_SELECT); }
        if (HIT(G, i, CK_A)) { G->ready[i] = true; ck_sfx(G, SFX_CONFIRM); }
        if (HIT(G, i, CK_B) && i == 1) { P->joined[i] = 0; ck_sfx(G, SFX_SELECT); }
    }
    int joined = P->joined[0] + P->joined[1];
    bool allReady = (!P->joined[0] || G->ready[0]) && (!P->joined[1] || G->ready[1]);
    if ((allReady && joined) || ANYHIT(G, CK_START)) { ck_save(G); ck_sfx(G, SFX_OPEN); go(G, SC_MAP); }
}

static void map_update(ck_game *G) {
    static const int dirs[4] = { CK_LEFT, CK_RIGHT, CK_UP, CK_DOWN };
    const ck_world *W = &G->world;
    for (int i = 0; i < 2; i++) {
        for (int d = 0; d < 4; d++)
            if (HIT(G, i, dirs[d]) && ck_map_step(G, dirs[d])) ck_sfx(G, SFX_STEP);
        if (HIT(G, i, CK_A) || HIT(G, i, CK_START)) {
            int n = ck_map_here(G);
            if (n < 0) continue;
            G->prog.at = (uint8_t)n;
            ck_save(G);
            int k = W->nodes[n].kind;
            if ((k == CK_NODE_LEVEL || k == CK_NODE_BOSS || k == CK_NODE_BONUS) && G->hasDef[n]) {
                ck_sfx(G, SFX_CONFIRM);
                G->snap.valid = false;
                G->scene = SC_LEVEL;
                ck_level_start(G, n, false, false);
                return;
            } else if (k == CK_NODE_SHOP) { ck_sfx(G, SFX_CONFIRM); go(G, SC_SHOP); return; }
            else if (k == CK_NODE_EXIT) { ck_sfx(G, SFX_NOPE); ck_map_say(G, "World 2 is coming soon!"); }
            else ck_map_say(G, "Home sweet home");
        }
    }
    ck_map_update(G, G->map.now + 1.0f / 60);
    int n = ck_map_here(G);
    if (n >= 0 && G->prog.at != n) { G->prog.at = (uint8_t)n; ck_save(G); }
}

static void level_update(ck_game *G) {
    int out = ck_level_update(G);
    int id = G->lv.node;
    switch (out) {
    case LV_FINISH:
        ck_level_finish(G);
        G->keepMap = true;   // the map is already built and paved
        go(G, SC_MAP);
        break;
    case LV_RETRY:
        ck_level_start(G, id, true, true);
        break;
    case LV_GAMEOVER:
        G->prog.lives = 3;
        G->prog.at = (uint8_t)id;
        ck_save(G);
        ck_map_reset(G);
        ck_map_say(G, "Out of tries! Have another go");
        G->keepMap = true;
        go(G, SC_MAP);
        break;
    case LV_PAUSE: G->scene = SC_PAUSE; G->t = 0; break;
    case LV_HATCH: G->scene = SC_JET; break;
    }
}

// ---------------------------------------------------------------------------- the story intro

// Mirrors introScene in web/game.js: the songs and sounds land on the same seconds.
static void intro_update(ck_game *G) {
    ck_intro *I = &G->intro;
    float t = G->t, was = t - 1.0f / 60;
    if (was < 4.5f && t >= 4.5f) ck_play(G, SONG_STORM);
    if (was < 11 && t >= 11) ck_play(G, SONG_TITLE);
    if (t > 4.8f && t < 10.5f && ck_rand(G) < 0.025f) {
        I->flash = 1;
        I->boltLife = 10;
        I->boltX = 120 + ck_rand(G) * 160;
        ck_sfx(G, SFX_THUNDER);
    }
    if (was < 8 && t >= 8) ck_sfx(G, SFX_WHOOSH);
    if (was < 11.6f && t >= 11.6f) ck_sfx(G, SFX_JUMP);
    if (was < 12 && t >= 12) ck_sfx(G, SFX_BOOST);
    if (I->flash > 0) I->flash -= 0.08f;
    if (I->boltLife > 0) {
        I->boltLife--;
        // the browser redraws the zigzag at random every frame, so it crackles
        for (int k = 0; k < CK_INTRO_BOLT; k++) I->boltDx[k] = (ck_rand(G) - 0.5f) * 18;
    }
    if (t > 15.2f || (t > 0.4f && (ANYHIT(G, CK_START) || ANYHIT(G, CK_A)))) go(G, SC_TITLE);
}

void ck_game_step(ck_game *G, uint16_t pad1, uint16_t pad2) {
    G->in.prev[0] = G->in.held[0];
    G->in.prev[1] = G->in.held[1];
    G->in.held[0] = pad1;
    G->in.held[1] = pad2;
    G->nsfx = 0;
    G->t += 1.0f / 60;
    switch (G->scene) {
    case SC_INTRO: intro_update(G); break;
    case SC_TITLE:
        if (ANYHIT(G, CK_START) || ANYHIT(G, CK_A)) { ck_sfx(G, SFX_CONFIRM); go(G, SC_SELECT); }
        break;
    case SC_SELECT: select_update(G); break;
    case SC_MAP: map_update(G); break;
    case SC_SHOP:
        ck_map_update(G, G->map.now + 1.0f / 60);
        if (ANYHIT(G, CK_A) || ANYHIT(G, CK_B) || ANYHIT(G, CK_START)) { ck_sfx(G, SFX_SELECT); G->keepMap = true; go(G, SC_MAP); }
        break;
    case SC_LEVEL: level_update(G); break;
    case SC_JET:
        if (ck_jet_update(G)) { ck_level_come_back(G); G->scene = SC_LEVEL; }
        break;
    case SC_PAUSE:
        if (ANYHIT(G, CK_START) || ANYHIT(G, CK_A)) { ck_sfx(G, SFX_SELECT); G->scene = SC_LEVEL; }
        else if (ANYHIT(G, CK_B)) { ck_sfx(G, SFX_SELECT); go(G, SC_MAP); }
        break;
    }
}
