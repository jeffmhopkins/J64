// The game as a whole: scenes (title, player select, map, levels, pause), progress, particles,
// and the sound queue. The flow from web/game.js; drawing is left to each frontend.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

// A save can come back damaged (a half-finished write, another game's EEPROM): keep every
// field in range so nothing indexes past an array.
static void sanitize(ck_game *G) {
    ck_prog *P = &G->prog;
    for (int i = 0; i < 2; i++) {
        P->hero[i] = P->hero[i] == HERO_OSTRICH ? HERO_OSTRICH : HERO_ROBOT;
        P->col[i] = (uint8_t)(P->col[i] % CK_NCOLS);
        P->joined[i] = P->joined[i] ? 1 : 0;
    }
    P->joined[0] = 1;
    P->little = P->little ? 1 : 0;
    for (int i = 0; i < CK_MAXNODES; i++) { P->got[i] &= 7; P->stars[i] &= 7; }
    if (P->at >= G->world.nnodes) P->at = 0;
    if (P->lives < 1 || P->lives > 9) P->lives = 3;
}

_Static_assert(sizeof(ck_saves) <= 512, "the save must fit a 4K EEPROM");

const ck_accdef ck_acc[ACC_COUNT] = {
    [ACC_PARTY] = { "Party hat", SLOT_HEAD, 0, 15 },
    [ACC_FLOWER] = { "Flower", SLOT_HEAD, 0, 15 },
    [ACC_BEANIE] = { "Beanie", SLOT_HEAD, 0, 20 },
    [ACC_BASEBALL] = { "Ball cap", SLOT_HEAD, 0, 20 },
    [ACC_CHEF] = { "Chef hat", SLOT_HEAD, 0, 25 },
    [ACC_PROPELLER] = { "Propeller cap", SLOT_HEAD, 0, 30 },
    [ACC_COWBOY] = { "Cowboy hat", SLOT_HEAD, 0, 30 },
    [ACC_PIRATE] = { "Pirate hat", SLOT_HEAD, 0, 35 },
    [ACC_TOPHAT] = { "Top hat", SLOT_HEAD, 0, 40 },
    [ACC_WIZARD] = { "Wizard hat", SLOT_HEAD, 0, 45 },
    [ACC_CROWN] = { "Crown", SLOT_HEAD, 0, 60 },
    [ACC_RAINBOWCAP] = { "Rainbow cap", SLOT_HEAD, 1, 80 },
    [ACC_GLASSES] = { "Eyeglasses", SLOT_FACE, 0, 15 },
    [ACC_SHADES] = { "Cool shades", SLOT_FACE, 0, 25 },
    [ACC_HEARTS] = { "Heart glasses", SLOT_FACE, 0, 25 },
    [ACC_STARS] = { "Star glasses", SLOT_FACE, 0, 25 },
    [ACC_GOGGLES] = { "Goggles", SLOT_FACE, 0, 30 },
    [ACC_MUSTACHE] = { "Mustache", SLOT_FACE, 0, 20 },
    [ACC_MONOCLE] = { "Monocle", SLOT_FACE, 0, 35 },
    [ACC_BOWTIE] = { "Bow tie", SLOT_NECK, 0, 15 },
    [ACC_SCARF] = { "Scarf", SLOT_NECK, 0, 20 },
    [ACC_NECKTIE] = { "Necktie", SLOT_NECK, 0, 20 },
    [ACC_BANDANA] = { "Bandana", SLOT_NECK, 0, 20 },
    [ACC_MEDAL] = { "Gold medal", SLOT_NECK, 0, 50 },
    [ACC_SWEATER] = { "Stripy sweater", SLOT_BODY, 0, 30 },
    [ACC_OVERALLS] = { "Overalls", SLOT_BODY, 0, 30 },
    [ACC_JERSEY] = { "Team jersey", SLOT_BODY, 0, 35 },
    [ACC_TUX] = { "Fancy suit", SLOT_BODY, 0, 50 },
    [ACC_SUPER] = { "Super suit", SLOT_BODY, 0, 60 },
    [ACC_DISCO] = { "Disco suit", SLOT_BODY, 1, 90 },
    [ACC_SNEAKERS] = { "Sneakers", SLOT_FEET, 0, 20 },
    [ACC_BOOTS] = { "Boots", SLOT_FEET, 0, 25 },
    [ACC_RAINBOOTS] = { "Rain boots", SLOT_FEET, 0, 25 },
    [ACC_FLIPPERS] = { "Flippers", SLOT_FEET, 0, 30 },
    [ACC_SKATES] = { "Roller skates", SLOT_FEET, 0, 40 },
    [ACC_LIGHTUP] = { "Light-up shoes", SLOT_FEET, 1, 70 },
};

// An older save has no closet, and a damaged one may wear what it does not own: keep it sane.
static void sanitize_closet(ck_closet *c) {
    if (c->magic != CK_CLOSET_MAGIC) { memset(c, 0, sizeof *c); c->magic = CK_CLOSET_MAGIC; }
    for (int a = ACC_COUNT; a < (int)sizeof c->own * 8; a++) c->own[a >> 3] &= (uint8_t)~(1u << (a & 7));
    for (int s = 0; s < 2; s++)
        for (int k = 0; k < SLOT_COUNT; k++) {
            int a = c->wear[s][k] - 1;
            if (a < 0 || a >= ACC_COUNT || ck_acc[a].slot != k || !CK_OWNS(c, a)) c->wear[s][k] = 0;
        }
    if (c->magic2 != CK_CLOSET_MAGIC2) { c->magic2 = CK_CLOSET_MAGIC2; c->paints = 0; c->gear = 0; c->rainbow = 0; }
    if (c->magic3 != CK_CLOSET_MAGIC3) { c->magic3 = CK_CLOSET_MAGIC3; c->pad3 = 0; c->fished = 0; }
    c->paints |= CK_FREE_PAINTS;
    c->gear &= CK_GEAR_GUN | CK_GEAR_JET | CK_GEAR_RAINBOW;
    c->rainbow = c->gear & CK_GEAR_RAINBOW ? c->rainbow & 3 : 0;
}

int ck_shop_cost(int item) {
    if (item >= 0 && item < ACC_COUNT) return ck_acc[item].cost;
    if (item >= SHOP_PAINT0 && item < SHOP_PAINT0 + CK_NCOLS) return (CK_FREE_PAINTS >> (item - SHOP_PAINT0)) & 1 ? 0 : CK_PAINT_COST;
    if (item == SHOP_RAINBOW) return CK_RAINBOW_COST;
    if (item == SHOP_GUN) return CK_GUN_COST;
    if (item == SHOP_JET) return CK_JET_COST;
    return 0;
}
const char *ck_shop_name(int item, char *buf, int n) {
    if (item >= 0 && item < ACC_COUNT) snprintf(buf, n, "%s", ck_acc[item].name);
    else if (item == SHOP_ROBOT) snprintf(buf, n, "Robot");
    else if (item == SHOP_OSTRICH) snprintf(buf, n, "Robot ostrich");
    else if (item >= SHOP_PAINT0 && item < SHOP_PAINT0 + CK_NCOLS) snprintf(buf, n, "%s paint", ck_col_names[item - SHOP_PAINT0]);
    else if (item == SHOP_RAINBOW) snprintf(buf, n, "Rainbow paint");
    else if (item == SHOP_GUN) snprintf(buf, n, "Bubble gun");
    else if (item == SHOP_JET) snprintf(buf, n, "Super jetpack");
    else snprintf(buf, n, "?");
    return buf;
}

int ck_wearing(const ck_game *G, int s, int slot) {
    if (s < 0 || s > 1 || slot < 0 || slot >= SLOT_COUNT) return -1;
    return (int)G->saves.closet.wear[s][slot] - 1;
}

void ck_saves_read(ck_saves *s, const void *raw) {
    uint32_t magic;
    memcpy(&magic, raw, sizeof magic);
    memset(s, 0, sizeof *s);
    if (magic == CK_SAVES_MAGIC) memcpy(s, raw, sizeof *s);
    else if (magic == CK_SAVE_MAGIC) memcpy(&s->w[0], raw, sizeof s->w[0]);   // a save from before Worlds 2 to 4
    if (magic != CK_SAVES_MAGIC) memset(&s->closet, 0, sizeof s->closet);
    s->magic = CK_SAVES_MAGIC;
    sanitize_closet(&s->closet);
    if (s->world < 1 || s->world > CK_WORLDS) s->world = 1;
    for (int i = 0; i < CK_WORLDS; i++)
        if (s->w[i].magic != CK_SAVE_MAGIC) memset(&s->w[i], 0, sizeof s->w[i]);
}

const ck_saves *ck_game_saves(ck_game *G) {
    G->saves.magic = CK_SAVES_MAGIC;
    G->saves.world = (uint8_t)G->worldNum;
    G->saves.w[G->worldNum - 1] = G->prog;
    if (G->slot >= 0 && G->slot < CK_SLOTS) G->slots[G->slot] = G->saves;
    return &G->saves;
}

void ck_game_slots(ck_game *G, const ck_saves *slots) {
    for (int k = 0; k < CK_SLOTS; k++) {
        G->slots[k] = slots[k];
        if (G->slots[k].magic != CK_SAVES_MAGIC) memset(&G->slots[k], 0, sizeof G->slots[k]);
    }
    G->slot = 0;
}

ck_slot_info ck_slot_summary(const ck_game *G, int k) {
    ck_slot_info r = { false, 1, 0 };
    if (k < 0 || k >= CK_SLOTS) return r;
    const ck_saves *s = k == G->slot ? &G->saves : &G->slots[k];
    if (s->magic != CK_SAVES_MAGIC) return r;
    for (int i = 0; i < CK_WORLDS; i++) {
        const ck_prog *p = k == G->slot && i == G->worldNum - 1 ? &G->prog : &s->w[i];
        if (p->magic != CK_SAVE_MAGIC) continue;
        r.used = true;
        r.bolts += p->bolts;
    }
    r.world = k == G->slot ? G->worldNum : s->world >= 1 && s->world <= CK_WORLDS ? s->world : 1;
    return r;
}

void ck_save_to_slot(ck_game *G, int k) {
    if (k < 0 || k >= CK_SLOTS) return;
    ck_game_saves(G);
    G->slot = k;
    G->slots[k] = G->saves;
    G->saveNow = true;
    G->progDirty = true;
}

bool ck_game_init(ck_game *G, const char *world_text, const ck_prog *saved) {
    memset(G, 0, sizeof *G);
    G->rng = 0x2545F491u;
    G->worldNum = 1;
    G->saves.magic = CK_SAVES_MAGIC;
    G->saves.world = 1;
    sanitize_closet(&G->saves.closet);
    if (!ck_parse_world(world_text, &G->world)) return false;
    if (saved && saved->magic == CK_SAVE_MAGIC) G->prog = *saved;
    else ck_prog_fresh(&G->prog);
    sanitize(G);
    ck_map_reset(G);
    G->scene = SC_INTRO;
    G->song = SONG_LULLABY;
    return true;
}

// Read world n's map and levels through the loader. False (and nothing changed) if it has no map.
static bool load_world(ck_game *G, int n) {
    static ck_world w;
    char path[48];
    if (!G->load || n < 1 || n > CK_WORLDS) return false;
    snprintf(path, sizeof path, "w%d/world.txt", n);
    char *text = G->load(path, G->loadUser);
    bool ok = text && ck_parse_world(text, &w);
    free(text);
    if (!ok) return false;
    G->world = w;
    G->worldNum = n;
    memset(G->hasDef, 0, sizeof G->hasDef);
    for (int i = 0; i < w.nnodes; i++) {
        if (!w.nodes[i].level[0] || G->hasDef[i]) continue;
        snprintf(path, sizeof path, "w%d/%s.txt", n, w.nodes[i].level);
        char *t = G->load(path, G->loadUser);
        if (t) ck_game_add_level(G, w.nodes[i].level, t);
        free(t);
    }
    return true;
}

static bool world_exists(ck_game *G, int n) {
    char path[32];
    if (!G->load || n < 1 || n > CK_WORLDS) return false;
    snprintf(path, sizeof path, "w%d/world.txt", n);
    char *t = G->load(path, G->loadUser);
    bool ok = t != NULL;
    free(t);
    return ok;
}

// Make world n's progress the current one. The heroes, the tries left and the little-kid
// speed come along from the world you were in.
static void use_world_prog(ck_game *G, int n, const ck_prog *from) {
    const ck_prog *had = &G->saves.w[n - 1];
    if (had->magic == CK_SAVE_MAGIC) G->prog = *had;
    else ck_prog_fresh(&G->prog);
    if (from) {
        G->prog.lives = from->lives; G->prog.little = from->little;
        memcpy(G->prog.hero, from->hero, 2); memcpy(G->prog.col, from->col, 2); memcpy(G->prog.joined, from->joined, 2);
    }
    sanitize(G);
}

bool ck_game_boot(ck_game *G, ck_loader load, void *user, const ck_saves *saved) {
    memset(G, 0, sizeof *G);
    G->rng = 0x2545F491u;
    G->load = load;
    G->loadUser = user;
    if (saved) G->saves = *saved;
    if (G->saves.magic != CK_SAVES_MAGIC) { memset(&G->saves, 0, sizeof G->saves); G->saves.magic = CK_SAVES_MAGIC; }
    sanitize_closet(&G->saves.closet);
    int n = G->saves.world >= 1 && G->saves.world <= CK_WORLDS ? G->saves.world : 1;
    if (!load_world(G, n) && (n == 1 || !load_world(G, (n = 1)))) return false;
    // the heroes come from whichever world was saved last
    const ck_prog *from = NULL;
    for (int i = 0; i < CK_WORLDS; i++) if (G->saves.w[i].magic == CK_SAVE_MAGIC) { from = &G->saves.w[i]; break; }
    if (G->saves.w[n - 1].magic == CK_SAVE_MAGIC) from = &G->saves.w[n - 1];
    ck_prog keep;
    if (from) keep = *from;
    use_world_prog(G, n, from ? &keep : NULL);
    ck_map_reset(G);
    G->scene = SC_INTRO;
    G->song = SONG_LULLABY;
    return true;
}

static void go(ck_game *G, int scene);
void ck_go(ck_game *G, int scene) { go(G, scene); }

// The title's slot picker: play another slot's game (the one being played is kept as it is).
static void use_slot(ck_game *G, int k) {
    if (k == G->slot || k < 0 || k >= CK_SLOTS || !G->load) return;
    ck_game_saves(G);
    static ck_saves all[CK_SLOTS];
    memcpy(all, G->slots, sizeof all);
    ck_loader load = G->load;
    void *user = G->loadUser;
    if (!ck_game_boot(G, load, user, &all[k])) { ck_game_boot(G, load, user, &all[0]); k = 0; }
    memcpy(G->slots, all, sizeof all);
    G->slot = k;
}
static void slots_update(ck_game *G) {
    if (ANYHIT(G, CK_UP)) { G->slotPick = (G->slotPick + CK_SLOTS - 1) % CK_SLOTS; ck_sfx(G, SFX_SELECT); }
    if (ANYHIT(G, CK_DOWN)) { G->slotPick = (G->slotPick + 1) % CK_SLOTS; ck_sfx(G, SFX_SELECT); }
    if (ANYHIT(G, CK_B)) { ck_sfx(G, SFX_SELECT); go(G, SC_TITLE); return; }
    if (ANYHIT(G, CK_A) || ANYHIT(G, CK_START)) { ck_sfx(G, SFX_CONFIRM); use_slot(G, G->slotPick); go(G, SC_SELECT); }
}

bool ck_game_enter_world(ck_game *G, int n, int at) {
    if (n == G->worldNum || !world_exists(G, n)) return false;
    ck_prog was = G->prog;
    G->saves.w[G->worldNum - 1] = was;
    if (!load_world(G, n)) return false;
    G->saves.world = (uint8_t)n;
    use_world_prog(G, n, &was);
    for (int i = 0; i < G->world.nnodes; i++)
        if (G->world.nodes[i].kind == at) { G->prog.at = (uint8_t)i; break; }
    ck_save(G);
    ck_map_reset(G);
    char msg[48];
    snprintf(msg, sizeof msg, "%s!", G->world.name);
    for (char *c = msg; *c; c++) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 32);
    ck_map_say(G, msg);
    G->keepMap = true;
    go(G, SC_MAP);
    return true;
}

void ck_game_new_game(ck_game *G) {
    ck_prog was = G->prog;
    memset(G->saves.w, 0, sizeof G->saves.w);
    memset(&G->saves.closet, 0, sizeof G->saves.closet);
    sanitize_closet(&G->saves.closet);
    if (G->worldNum != 1) load_world(G, 1);
    G->saves.world = 1;
    ck_prog_fresh(&G->prog);
    memcpy(G->prog.hero, was.hero, 2); memcpy(G->prog.col, was.col, 2); memcpy(G->prog.joined, was.joined, 2);
    G->prog.little = was.little;
    sanitize(G);
    G->snap.valid = false;
    ck_map_reset(G);
    ck_save(G);
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
        ck_play(G, G->world.song ? G->world.song : SONG_MAP);
        break;
    case SC_SHOP:
        G->shopMsgT = 0;
        ck_play(G, G->world.song ? G->world.song : SONG_MAP);
        break;
    case SC_MENU: G->menuPick = 0; G->menuAsk = false; break;
    case SC_SLOTS: G->slotPick = G->slot; ck_play(G, SONG_TITLE); break;
    case SC_HOUSE: case SC_PAINT: case SC_AQUA: case SC_FISH: ck_play(G, SONG_SELECT); break;
    case SC_CARS: ck_play(G, ck_race_song(G->house.track)); break;
    case SC_PINBALL: ck_play(G, SONG_PINBALL); break;
    case SC_BLAST: ck_play(G, SONG_BLASTER); break;
    case SC_PIANO: ck_play(G, SONG_NONE); break;
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
        // colours: the free ones and the paint pots bought at the parts shop
        int step = HIT(G, i, CK_UP) ? CK_NCOLS - 1 : HIT(G, i, CK_DOWN) ? 1 : 0;
        if (step) {
            int c = P->col[i] % CK_NCOLS;
            for (int k = 0; k < CK_NCOLS; k++) { c = (c + step) % CK_NCOLS; if ((G->saves.closet.paints >> c) & 1) break; }
            P->col[i] = (uint8_t)c;
            G->saves.closet.rainbow &= (uint8_t)~(1u << i);
            ck_sfx(G, SFX_SELECT);
        }
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
        if (HIT(G, i, CK_START)) { ck_sfx(G, SFX_SELECT); go(G, SC_MENU); return; }   // START opens the map menu
        if (HIT(G, i, CK_A)) {
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
            } else if (k == CK_NODE_SHOP) { ck_sfx(G, SFX_CONFIRM); G->shopFromHouse = false; ck_go(G, SC_SHOP); return; }
            else if (k == CK_NODE_HOME) { ck_sfx(G, SFX_CONFIRM); ck_house_enter(G, false); return; }
            else if (k == CK_NODE_EXIT) {
                // through the gate to the next world, with its little film first
                if (G->worldNum >= CK_WORLDS || world_exists(G, G->worldNum + 1)) { ck_sfx(G, SFX_CONFIRM); ck_save(G); ck_cut_start(G, G->worldNum); return; }
                char msg[48];
                snprintf(msg, sizeof msg, "World %d is coming soon!", G->worldNum + 1);
                ck_sfx(G, SFX_NOPE); ck_map_say(G, msg);
            } else if (G->worldNum > 1) { ck_sfx(G, SFX_CONFIRM); ck_game_enter_world(G, G->worldNum - 1, CK_NODE_EXIT); return; }
            else ck_map_say(G, "Home sweet home");
        }
    }
    ck_map_update(G, G->map.now + 1.0f / 60);
    int n = ck_map_here(G);
    if (n >= 0 && G->prog.at != n) { G->prog.at = (uint8_t)n; ck_save(G); }
}

// START on the map: Keep playing, Change heroes, or New game (which asks once more first).
static void back_to_map(ck_game *G) { G->keepMap = true; go(G, SC_MAP); }
void ck_go_map(ck_game *G) { back_to_map(G); }

// The map menu. "Player 2 leaves" shows only while a second player is in the game.
int ck_menu_items(const ck_game *G, int *out) {
    int n = 0;
    out[n++] = MENU_KEEP; out[n++] = MENU_HEROES;
    if (G->prog.joined[1] && G->prog.joined[0]) out[n++] = MENU_DROP;
    out[n++] = MENU_NEW;
    return n;
}
static void menu_update(ck_game *G) {
    ck_map_update(G, G->map.now + 1.0f / 60);
    int items[4], ni = ck_menu_items(G, items);
    int n = G->menuAsk ? 2 : ni;
    if (G->menuPick >= n) G->menuPick = 0;
    if (ANYHIT(G, CK_UP)) { G->menuPick = (G->menuPick + n - 1) % n; ck_sfx(G, SFX_SELECT); }
    if (ANYHIT(G, CK_DOWN)) { G->menuPick = (G->menuPick + 1) % n; ck_sfx(G, SFX_SELECT); }
    if (ANYHIT(G, CK_B) || (ANYHIT(G, CK_START) && !G->menuAsk)) {
        ck_sfx(G, SFX_SELECT);
        if (G->menuAsk) { G->menuAsk = false; G->menuPick = ni - 1; } else back_to_map(G);
        return;
    }
    if (!ANYHIT(G, CK_A) && !ANYHIT(G, CK_START)) return;
    if (G->menuAsk) {
        if (G->menuPick == 1) { ck_sfx(G, SFX_OPEN); ck_game_new_game(G); go(G, SC_SELECT); }
        else { ck_sfx(G, SFX_SELECT); G->menuAsk = false; G->menuPick = ni - 1; }
        return;
    }
    switch (items[G->menuPick]) {
    case MENU_KEEP: ck_sfx(G, SFX_SELECT); back_to_map(G); break;
    case MENU_HEROES: ck_sfx(G, SFX_CONFIRM); go(G, SC_SELECT); break;
    case MENU_DROP: G->prog.joined[1] = 0; ck_sfx(G, SFX_SELECT); ck_save(G); back_to_map(G); break;
    default: ck_sfx(G, SFX_NOPE); G->menuAsk = true; G->menuPick = 0; break;
    }
}

// The parts shop: each player moves their own cursor; A buys an accessory, then wears or takes it off.
#define SHOP_COLS 12   // the same grid as the web shop: hats; face and neck; outfits and shoes; bots and gear
static bool shop_owns(const ck_closet *c, int item) {
    if (item < ACC_COUNT) return CK_OWNS(c, item);
    if (item >= SHOP_PAINT0 && item < SHOP_PAINT0 + CK_NCOLS) return (c->paints >> (item - SHOP_PAINT0)) & 1;
    if (item == SHOP_RAINBOW) return (c->gear & CK_GEAR_RAINBOW) != 0;
    if (item == SHOP_GUN) return c->gear & CK_GEAR_GUN;
    if (item == SHOP_JET) return (c->gear & CK_GEAR_JET) != 0;
    return true;   // the heroes are free
}
static void shop_act(ck_game *G, int s) {
    int a = G->shopCur[s], cost = ck_shop_cost(a);
    ck_closet *c = &G->saves.closet;
    char name[24];
    ck_shop_name(a, name, sizeof name);
    if (!shop_owns(c, a)) {
        if (G->prog.bolts < cost) {
            ck_sfx(G, SFX_NOPE);
            snprintf(G->shopMsg, sizeof G->shopMsg, "Need %d more bolts", cost - G->prog.bolts);
            G->shopMsgT = 90;
            return;
        }
        G->prog.bolts = (uint16_t)(G->prog.bolts - cost);
        G->map.bolts = G->prog.bolts;
        if (a < ACC_COUNT) { c->own[a >> 3] |= (uint8_t)(1u << (a & 7)); c->wear[s][ck_acc[a].slot] = (uint8_t)(a + 1); }
        else if (a == SHOP_GUN) c->gear |= CK_GEAR_GUN;
        else if (a == SHOP_JET) c->gear |= CK_GEAR_JET;
        else if (a == SHOP_RAINBOW) { c->gear |= CK_GEAR_RAINBOW; c->rainbow |= (uint8_t)(1u << s); G->prog.col[s] = CK_RAINBOW_COL; }
        else { c->paints |= (uint8_t)(1u << (a - SHOP_PAINT0)); G->prog.col[s] = (uint8_t)(a - SHOP_PAINT0); c->rainbow &= (uint8_t)~(1u << s); }
        ck_sfx(G, SFX_BUY);
        if (a == SHOP_GUN) snprintf(G->shopMsg, sizeof G->shopMsg, "Bubble gun! Z shoots");
        else if (a == SHOP_JET) snprintf(G->shopMsg, sizeof G->shopMsg, "Hold A to glide far!");
        else snprintf(G->shopMsg, sizeof G->shopMsg, "%s!", name);
        G->shopMsgT = a >= SHOP_GUN ? 150 : 90;
    } else if (a < ACC_COUNT) {
        uint8_t *w = &c->wear[s][ck_acc[a].slot];
        if (*w == a + 1) { *w = 0; ck_sfx(G, SFX_SELECT); } else { *w = (uint8_t)(a + 1); ck_sfx(G, SFX_CONFIRM); }
    } else if (a == SHOP_ROBOT || a == SHOP_OSTRICH) {
        G->prog.hero[s] = a == SHOP_ROBOT ? HERO_ROBOT : HERO_OSTRICH;
        ck_sfx(G, SFX_CONFIRM);
    } else if (a == SHOP_GUN || a == SHOP_JET) {
        snprintf(G->shopMsg, sizeof G->shopMsg, a == SHOP_GUN ? "In a level, Z shoots bubbles" : "Hold A in the air to glide");
        G->shopMsgT = 120;
        ck_sfx(G, SFX_SELECT);
    } else if (a == SHOP_RAINBOW) {
        G->prog.col[s] = CK_RAINBOW_COL; c->rainbow |= (uint8_t)(1u << s); ck_sfx(G, SFX_CONFIRM);
    } else { G->prog.col[s] = (uint8_t)(a - SHOP_PAINT0); c->rainbow &= (uint8_t)~(1u << s); ck_sfx(G, SFX_CONFIRM); }
    ck_save(G);
}
bool ck_shop_owns(const ck_game *G, int item) { return shop_owns(&G->saves.closet, item); }
bool ck_rainbow_skin(const ck_game *G, int slot) { return slot >= 0 && slot < 2 && ((G->saves.closet.rainbow >> slot) & 1); }
static void shop_update(ck_game *G) {
    ck_map_update(G, G->map.now + 1.0f / 60);
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        int *cur = &G->shopCur[s];
        if (*cur < 0 || *cur >= SHOP_COUNT) *cur = 0;
        if (HIT(G, s, CK_LEFT)) { *cur = (*cur + SHOP_COUNT - 1) % SHOP_COUNT; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_RIGHT)) { *cur = (*cur + 1) % SHOP_COUNT; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_UP) && *cur >= SHOP_COLS) { *cur -= SHOP_COLS; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_DOWN) && *cur + SHOP_COLS < SHOP_COUNT) { *cur += SHOP_COLS; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_A)) shop_act(G, s);
        if (HIT(G, s, CK_B) || HIT(G, s, CK_START)) {
            ck_sfx(G, SFX_SELECT);
            ck_save(G);
            if (G->shopFromHouse) { G->shopFromHouse = false; ck_house_enter(G, true); } else back_to_map(G);
            return;
        }
    }
    if (G->shopMsgT > 0) G->shopMsgT--;
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
    G->nnotes = 0;
    G->t += 1.0f / 60;
    switch (G->scene) {
    case SC_INTRO: intro_update(G); break;
    case SC_TITLE:
        if (ANYHIT(G, CK_START) || ANYHIT(G, CK_A)) { ck_sfx(G, SFX_CONFIRM); go(G, SC_SLOTS); }
        break;
    case SC_SLOTS: slots_update(G); break;
    case SC_HOUSE: case SC_PAINT: case SC_AQUA: case SC_CARS: case SC_PIANO: case SC_PINBALL: case SC_BLAST: case SC_FISH: ck_house_step(G); break;
    case SC_SELECT: select_update(G); break;
    case SC_MAP: map_update(G); break;
    case SC_SHOP: shop_update(G); break;
    case SC_MENU: menu_update(G); break;
    case SC_LEVEL: level_update(G); break;
    case SC_CUT:
        if (ck_cut_update(G)) {
            if (G->cut < CK_WORLDS && ck_game_enter_world(G, G->cut + 1, CK_NODE_HOME)) break;
            G->keepMap = false;
            go(G, SC_TITLE);
        }
        break;
    case SC_JET:
        if (ck_jet_update(G)) { ck_level_come_back(G); G->scene = SC_LEVEL; }
        break;
    case SC_PAUSE:
        if (ANYHIT(G, CK_START) || ANYHIT(G, CK_A)) { ck_sfx(G, SFX_SELECT); G->scene = SC_LEVEL; }
        else if (ANYHIT(G, CK_B)) { ck_sfx(G, SFX_SELECT); go(G, SC_MAP); }
        break;
    }
}
