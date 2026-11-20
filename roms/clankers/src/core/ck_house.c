// The heroes' house and the little games in it. Mirrors houseScene, paintScene, aquaScene,
// carsScene and pianoScene in web/game.js (the layout is HOUSEART.LAYOUT in web/lib/houseart.js).
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ck_internal.h"

const ck_room ck_rooms[CK_HOUSE_ROOMS] = {
    { 0, 320, "The yard", 0x8FD3FF, 0xBDE8FF, 0x5DBB63 },
    { 320, 560, "Parts shop", 0xF3C77B, 0xE9B865, 0x8A5A3C },
    { 560, 800, "Kitchen", 0xBDE7D2, 0xA9DCC3, 0xD9CDB8 },
    { 800, 1040, "Paint studio", 0xFFF1D6, 0xF6E1BA, 0x9C7552 },
    { 1040, 1280, "Music room", 0xC9B8F0, 0xB8A3E9, 0x7A4E3A },
    { 1280, 1520, "Aquarium", 0x9FD8F0, 0x88CBE6, 0x5D7C99 },
    { 1520, 1760, "Bedroom", 0xFFD3E2, 0xFFC1D6, 0x8C6A55 },
    { 1760, 2040, "Garage", 0xB5BAC4, 0xA4AAB6, 0x6E7280 },
};
const ck_spot ck_spots[CK_HOUSE_SPOTS] = {
    { 440, SPOT_SHOP, FURN_COUNTER, "Parts shop" },
    { 612, SPOT_SNACK, FURN_FRIDGE, "Have a snack" },
    { 920, SPOT_PAINT, FURN_EASEL, "Paint the mural" },
    { 1160, SPOT_PIANO, FURN_PIANO, "Play the piano" },
    { 1400, SPOT_AQUA, FURN_TANK, "Swim with the fish" },
    { 1640, SPOT_BED, FURN_BED, "Sleep and save" },
    { 1900, SPOT_CARS, FURN_CAR, "Race the cars" },
};
const ck_decor ck_decors[CK_HOUSE_DECOR] = {
    { 120, FURN_TREE }, { 262, FURN_MAILBOX }, { 690, FURN_STOVE }, { 752, FURN_TABLE }, { 1010, FURN_PLANT },
    { 1240, FURN_DRUM }, { 1480, FURN_PLANT }, { 1720, FURN_LAMP }, { 2000, FURN_TOOLS },
};
const uint32_t ck_mural_pal[CK_MURAL_PAL] = { 0xE04F4F, 0xFF8A3D, 0xFFD23F, 0x49D17A, 0x5AD1FF, 0x3A6FC0, 0xC28BFF, 0xFF8FC7, 0xFFFFFF, 0x2B2230 };

#define HOUSE_SPEED 1.7f
#define HOUSE_JUMP (-4.4f)
#define HOUSE_GRAV 0.26f

static int dir_x(const ck_game *G, int s) { return (HELD(G, s, CK_RIGHT) ? 1 : 0) - (HELD(G, s, CK_LEFT) ? 1 : 0); }
static int dir_y(const ck_game *G, int s) { return (HELD(G, s, CK_DOWN) ? 1 : 0) - (HELD(G, s, CK_UP) ? 1 : 0); }
// a little rng the browser copies, so the babies start in the same places
static uint32_t hseed;
static float hrng(void) { hseed = (hseed * 1103515245u + 12345u) & 0x7fffffffu; return (float)hseed / 2147483648.0f; }

// Every rescued baby: per world, one for each level beaten (up to its four), in that world's colours.
static void find_babies(ck_game *G, uint32_t *out, int *n) {
    static ck_world w;
    *n = 0;
    for (int k = 1; k <= CK_WORLDS && *n < CK_HOUSE_BABIES; k++) {
        const ck_world *wd = NULL;
        const ck_prog *p = k == G->worldNum ? &G->prog : &G->saves.w[k - 1];
        if (p->magic != CK_SAVE_MAGIC && k != G->worldNum) continue;
        if (k == G->worldNum) wd = &G->world;
        else if (G->load) {
            char path[32];
            snprintf(path, sizeof path, "w%d/world.txt", k);
            char *text = G->load(path, G->loadUser);
            if (text && ck_parse_world(text, &w)) wd = &w;
            free(text);
        }
        if (!wd) continue;
        int nb = 0;
        for (int i = 0; i < wd->nnodes; i++) if (wd->nodes[i].kind == CK_NODE_LEVEL && ((p->beaten >> i) & 1)) nb++;
        if (nb > 4) nb = 4;
        for (int j = 0; j < nb && *n < CK_HOUSE_BABIES; j++) out[(*n)++] = wd->babies[j] ? wd->babies[j] : 0xFF8FC7;
    }
}

void ck_house_enter(ck_game *G, bool fromShop) {
    ck_house *H = &G->house;
    float x = fromShop ? (float)ck_spots[SPOT_SHOP].x : 200;
    H->np = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        ck_hhero *p = &H->ps[H->np];
        memset(p, 0, sizeof *p);
        p->slot = s; p->x = x - 10 + H->np * 20; p->y = CK_HOUSE_FLOOR; p->face = 1; p->ground = true;
        H->np++;
    }
    uint32_t cols[CK_HOUSE_BABIES];
    find_babies(G, cols, &H->nbabies);
    hseed = 7;
    for (int i = 0; i < H->nbabies; i++) {
        ck_hbaby *b = &H->babies[i];
        const ck_room *r = &ck_rooms[i % CK_HOUSE_ROOMS];
        b->hex = cols[i];
        b->x = r->x0 + 30 + hrng() * (r->x1 - r->x0 - 60);
        b->x0 = r->x0 + 14; b->x1 = r->x1 - 14;
        b->vx = (hrng() < 0.5f ? -1 : 1) * (0.3f + hrng() * 0.3f);
        b->y = 0; b->vy = 0;
        b->wait = (int)(hrng() * 120);
    }
    H->mode = HM_WALK;
    H->msgT = 0;
    H->cam = ck_clampf(x - CK_W / 2, 0, CK_HOUSE_W - CK_W);
    ck_go(G, SC_HOUSE);
}

int ck_house_near(const ck_hhero *p) {
    for (int k = 0; k < CK_HOUSE_SPOTS; k++) if (fabsf(p->x - ck_spots[k].x) < 22) return k;
    return -1;
}

static void say(ck_house *H, const char *s) { snprintf(H->msg, sizeof H->msg, "%s", s); H->msgT = 100; }

// the heroes as they come into one of the house's games
static void game_heroes(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) { memset(&H->gs[i], 0, sizeof H->gs[i]); H->gs[i].slot = H->ps[i].slot; H->gs[i].face = 1; }
}
static void paint_start(ck_game *G);
static void aqua_start(ck_game *G);
static void cars_start(ck_game *G);
static void piano_start(ck_game *G);

static void use_spot(ck_game *G, int k, ck_hhero *p) {
    ck_house *H = &G->house;
    switch (ck_spots[k].kind) {
    case SPOT_SHOP: ck_sfx(G, SFX_CONFIRM); G->shopFromHouse = true; ck_go(G, SC_SHOP); break;
    case SPOT_SNACK: {
        p->eat = 70;
        ck_sfx(G, SFX_SPROUT);
        ck_sparks(G, p->x - H->cam, p->y - 20, 8, 0xFFD23F);
        bool full = G->prog.lives >= 3;
        G->prog.lives = 3;
        ck_save(G);
        say(H, full ? "Yum! Crunchy bolts!" : "Yum! Batteries full!");
        break;
    }
    case SPOT_PAINT: ck_sfx(G, SFX_OPEN); paint_start(G); break;
    case SPOT_PIANO: ck_sfx(G, SFX_OPEN); piano_start(G); break;
    case SPOT_AQUA: ck_sfx(G, SFX_OPEN); aqua_start(G); break;
    case SPOT_CARS: ck_sfx(G, SFX_OPEN); cars_start(G); break;
    case SPOT_BED: ck_sfx(G, SFX_SELECT); H->mode = HM_SLOTS; H->pick = G->slot; break;
    }
}

static void back_to_house(ck_game *G) { ck_sfx(G, SFX_SELECT); G->scene = SC_HOUSE; G->t = 0; ck_play(G, SONG_SELECT); }

void ck_house_update(ck_game *G) {
    ck_house *H = &G->house;
    if (H->msgT > 0) H->msgT--;
    ck_step_parts(G);
    if (H->mode == HM_SLOTS) {
        if (ANYHIT(G, CK_UP)) { H->pick = (H->pick + CK_SLOTS - 1) % CK_SLOTS; ck_sfx(G, SFX_SELECT); }
        if (ANYHIT(G, CK_DOWN)) { H->pick = (H->pick + 1) % CK_SLOTS; ck_sfx(G, SFX_SELECT); }
        if (ANYHIT(G, CK_B) || ANYHIT(G, CK_START)) { H->mode = HM_WALK; ck_sfx(G, SFX_SELECT); }
        else if (ANYHIT(G, CK_A)) { ck_save_to_slot(G, H->pick); H->mode = HM_SLEEP; H->sleepT = 0; ck_sfx(G, SFX_RECORD); }
        return;
    }
    if (H->mode == HM_SLEEP) {
        H->sleepT += 1.0f / 60;
        if (H->sleepT > 2.6f) { H->mode = HM_WALK; snprintf(H->msg, sizeof H->msg, "Saved in slot %d!", G->slot + 1); H->msgT = 120; }
        return;
    }
    if (ANYHIT(G, CK_START)) { ck_sfx(G, SFX_SELECT); ck_save(G); ck_go_map(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->ps[i];
        int s = p->slot, want = dir_x(G, s);
        if (p->eat > 0) { p->eat--; p->vx = 0; }
        else {
            p->vx += (want * HOUSE_SPEED - p->vx) * 0.3f;
            if (want) p->face = want;
            if (HIT(G, s, CK_A) && p->ground) { p->vy = HOUSE_JUMP; p->ground = false; ck_sfx(G, SFX_JUMP); }
            if (HIT(G, s, CK_DOWN) && p->ground) {
                int k = ck_house_near(p);
                if (k >= 0) { use_spot(G, k, p); if (G->scene != SC_HOUSE || H->mode != HM_WALK) return; }
            }
        }
        p->vy += HOUSE_GRAV; p->x += p->vx; p->y += p->vy;
        if (p->y >= CK_HOUSE_FLOOR) { p->y = CK_HOUSE_FLOOR; p->vy = 0; p->ground = true; }
        p->x = ck_clampf(p->x, -4, CK_HOUSE_W - 10);
        p->anim += fabsf(p->vx);
        if (p->x < 0) { ck_sfx(G, SFX_SELECT); ck_save(G); ck_go_map(G); return; }
    }
    float mid = 0;
    for (int i = 0; i < H->np; i++) mid += H->ps[i].x;
    mid /= H->np > 0 ? H->np : 1;
    H->cam = ck_clampf(mid - CK_W / 2, 0, CK_HOUSE_W - CK_W);
    if (H->np > 1) for (int i = 0; i < H->np; i++) H->ps[i].x = ck_clampf(H->ps[i].x, H->cam + 8, H->cam + CK_W - 8);
    for (int i = 0; i < H->nbabies; i++) {
        ck_hbaby *b = &H->babies[i];
        if (b->wait > 0) { b->wait--; continue; }
        b->x += b->vx;
        if (b->x < b->x0 || b->x > b->x1) { b->vx = -b->vx; b->x = ck_clampf(b->x, b->x0, b->x1); }
        if (b->y == 0 && ck_rand(G) < 0.006f) b->vy = -1.8f;
        if (b->y < 0 || b->vy < 0) { b->vy += 0.12f; b->y += b->vy; if (b->y >= 0) { b->y = 0; b->vy = 0; } }
        if (ck_rand(G) < 0.003f) b->wait = 60 + (int)(ck_rand(G) * 90);
    }
}

// ---------------------------------------------------------------------------- the paint studio
// Fly with the jetpack (hold A), hold B to paint with your front hand, L and R (or C) change the
// colour. The wall stays painted until the console is switched off.
static void paint_start(ck_game *G) {
    ck_house *H = &G->house;
    game_heroes(G);
    for (int i = 0; i < H->np; i++) { ck_hhero *p = &H->gs[i]; p->x = 120 + i * 80; p->y = CK_MURAL_FLOOR; p->col = i ? 4 : 0; }
    ck_go(G, SC_PAINT);
}
void ck_mural_dab(ck_house *H, float px, float py, int c) {
    int cx = (int)floorf(px / CK_MURAL_CELL), cy = (int)floorf((py - CK_MURAL_Y) / CK_MURAL_CELL);
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || y < 0 || x >= CK_MURAL_W || y >= CK_MURAL_H) continue;
            H->mural[y * CK_MURAL_W + x] = (uint8_t)(c == CK_MURAL_ERASER ? 0 : c + 1);
        }
}
static void paint_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->gs[i];
        int s = p->slot, want = dir_x(G, s);
        p->vx += (want * 1.8f - p->vx) * 0.25f;
        if (want) p->face = want;
        p->jet = HELD(G, s, CK_A);
        if (p->jet) { p->vy = ck_maxf(-2.4f, p->vy - 0.32f); if ((int)(G->t * 60) % 6 == 0) ck_sfx(G, SFX_HOVER); }
        else p->vy = ck_minf(2.5f, p->vy + 0.14f);
        p->x = ck_clampf(p->x + p->vx, 10, CK_W - 10);
        p->y += p->vy;
        if (p->y > CK_MURAL_FLOOR) { p->y = CK_MURAL_FLOOR; p->vy = 0; }
        if (p->y < CK_MURAL_Y + 28) { p->y = CK_MURAL_Y + 28; p->vy = 0; }
        if (HIT(G, s, CK_C) || HIT(G, s, CK_R)) { p->col = (p->col + 1) % (CK_MURAL_PAL + 1); ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_L)) { p->col = (p->col + CK_MURAL_PAL) % (CK_MURAL_PAL + 1); ck_sfx(G, SFX_SELECT); }
        float bx = p->x + p->face * 9, by = p->y - 16;
        if (HELD(G, s, CK_B)) {
            if (!p->brush) { p->bx = bx; p->by = by; p->brush = true; }
            int n = (int)ceilf(hypotf(bx - p->bx, by - p->by) / 2);
            if (n < 1) n = 1;
            for (int k = 1; k <= n; k++) ck_mural_dab(H, p->bx + (bx - p->bx) * k / n, p->by + (by - p->by) * k / n, p->col);
            p->bx = bx; p->by = by;
        } else p->brush = false;
    }
}

// ---------------------------------------------------------------------------- the aquarium
static float aqua_floor(float x) {
    if (x >= CK_AQUA_BEACH_X) return CK_AQUA_BEACH_Y;
    if (x > CK_AQUA_BEACH_X - 30) return CK_AQUA_SEABED - (x - (CK_AQUA_BEACH_X - 30)) / 30 * (CK_AQUA_SEABED - CK_AQUA_BEACH_Y);
    return CK_AQUA_SEABED;
}
static void aqua_start(ck_game *G) {
    ck_house *H = &G->house;
    game_heroes(G);
    for (int i = 0; i < H->np; i++) { H->gs[i].x = 60 + i * 40; H->gs[i].y = 120; }
    hseed = 11;
    for (int i = 0; i < CK_AQUA_FISH; i++) {
        ck_fish *f = &H->fish[i];
        f->kind = i % 3; f->x = 20 + hrng() * 190; f->y = 70 + hrng() * 130;
        f->vx = (hrng() < 0.5f ? -1 : 1) * (0.4f + hrng() * 0.5f); f->scare = 0; f->ph = hrng() * 6;
    }
    for (int i = 0; i < CK_AQUA_SHRIMP; i++) {
        ck_shrimp *s = &H->shrimp[i];
        s->x = 20 + hrng() * 170; s->y = CK_AQUA_SEABED - 2; s->vx = s->vy = 0; s->face = hrng() < 0.5f ? -1 : 1; s->rest = (int)(hrng() * 200);
    }
    for (int i = 0; i < CK_AQUA_CRABS; i++) { H->crabs[i].x = 250 + i * 22; H->crabs[i].vx = (i % 2 ? -1 : 1) * 0.5f; H->crabs[i].snap = H->crabs[i].wait = 0; }
    ck_go(G, SC_AQUA);
}
static void bubble(ck_game *G, float x, float y, float vx, float vy, float r, int life) {
    if (G->nparts >= CK_MAXPARTS) return;
    ck_part *q = &G->parts[G->nparts++];
    memset(q, 0, sizeof *q);
    q->x = x; q->y = y; q->vx = vx; q->vy = vy; q->r = r; q->life = life; q->max = life; q->col = 0xDCFAFF; q->alpha = 220; q->kind = PART_PUFF;
}
static void aqua_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->gs[i];
        int s = p->slot, dx = dir_x(G, s), dy = dir_y(G, s);
        if (dx) p->face = dx;
        p->land = p->x >= CK_AQUA_BEACH_X && p->y <= CK_AQUA_BEACH_Y + 2;
        if (p->land) {
            p->vx += (dx * 1.6f - p->vx) * 0.3f;
            if (HIT(G, s, CK_A) && p->ground) { p->vy = -4; p->ground = false; ck_sfx(G, SFX_JUMP); }
            p->vy += 0.26f;
        } else {
            p->vx = ck_clampf((p->vx + dx * 0.14f) * 0.95f, -2, 2);
            p->vy = ck_clampf((p->vy + dy * 0.14f) * 0.95f + 0.01f, -2.4f, 2);
            if (HIT(G, s, CK_A)) { p->vy -= 1.6f; ck_sfx(G, SFX_FLAP); for (int k = 0; k < 3; k++) bubble(G, p->x + (ck_rand(G) - 0.5f) * 6, p->y - 26, (ck_rand(G) - 0.5f), -0.8f, 2, 30); }
        }
        if (HIT(G, s, CK_B)) { ck_sfx(G, SFX_POP); bubble(G, p->x + p->face * 8, p->y - 22, p->face * 0.6f, -0.8f, 4, 60); }
        p->x = ck_clampf(p->x + p->vx, 10, CK_W - 10);
        p->y += p->vy;
        float fl = aqua_floor(p->x);
        p->ground = false;
        if (p->y >= fl) { p->y = fl; p->vy = 0; p->ground = true; }
        if (!p->land && p->y < CK_AQUA_TOP + 22) { p->y = CK_AQUA_TOP + 22; p->vy = ck_maxf(0, p->vy); }
        if (!p->land && p->x > CK_AQUA_BEACH_X - 2 && p->y > CK_AQUA_BEACH_Y + 4) p->x = CK_AQUA_BEACH_X - 2;
        if (!p->land && --p->bubT <= 0) { p->bubT = 40 + (int)(ck_rand(G) * 30); bubble(G, p->x, p->y - 30, 0, -0.7f, 1.8f, 70); }
    }
    for (int i = 0; i < CK_AQUA_FISH; i++) {
        ck_fish *f = &H->fish[i];
        const ck_hhero *fear = NULL;
        for (int k = 0; k < H->np; k++) if (!H->gs[k].land && hypotf(H->gs[k].x - f->x, H->gs[k].y - 18 - f->y) < 34) fear = &H->gs[k];
        if (fear) { f->vx = (f->x > fear->x ? 1 : -1) * 1.6f; f->scare = 40; }
        else if (f->scare > 0 && --f->scare == 0) f->vx = (f->vx > 0 ? 1 : -1) * 0.6f;
        f->x += f->vx;
        f->y += sinf(G->t * 1.5f + f->ph) * 0.25f;
        if (f->x < 12) { f->x = 12; f->vx = fabsf(f->vx); }
        if (f->x > CK_AQUA_BEACH_X - 16) { f->x = CK_AQUA_BEACH_X - 16; f->vx = -fabsf(f->vx); }
        f->y = ck_clampf(f->y, CK_AQUA_TOP + 14, CK_AQUA_SEABED - 14);
    }
    for (int i = 0; i < CK_AQUA_SHRIMP; i++) {
        ck_shrimp *s = &H->shrimp[i];
        bool scared = false;
        for (int k = 0; k < H->np; k++) if (!H->gs[k].land && fabsf(H->gs[k].x - s->x) < 24 && H->gs[k].y > CK_AQUA_SEABED - 40) scared = true;
        if (s->y >= CK_AQUA_SEABED - 2 && (scared || --s->rest <= 0)) {
            s->face = scared ? (H->gs[0].x > s->x ? -1 : 1) : (ck_rand(G) < 0.5f ? -1 : 1);
            s->vx = s->face * 1.2f; s->vy = -1.6f; s->rest = 120 + (int)(ck_rand(G) * 160);
        }
        s->vy += 0.05f;
        s->x = ck_clampf(s->x + s->vx, 12, CK_AQUA_BEACH_X - 40);
        s->y += s->vy;
        s->vx *= 0.98f;
        if (s->y >= CK_AQUA_SEABED - 2) { s->y = CK_AQUA_SEABED - 2; s->vy = 0; s->vx = 0; }
    }
    for (int i = 0; i < CK_AQUA_CRABS; i++) {
        ck_crab *c = &H->crabs[i];
        bool close = false;
        for (int k = 0; k < H->np; k++) if (H->gs[k].land && fabsf(H->gs[k].x - c->x) < 22) close = true;
        if (close && c->snap <= 0) { c->snap = 30; ck_sfx(G, SFX_TICK); }
        if (c->snap > 0) c->snap--;
        if (c->wait > 0) { c->wait--; continue; }
        c->x += c->vx;
        if (c->x < CK_AQUA_BEACH_X + 10 || c->x > CK_W - 12) c->vx = -c->vx;
        if (ck_rand(G) < 0.01f) c->wait = 30 + (int)(ck_rand(G) * 60);
    }
}

// ---------------------------------------------------------------------------- the garage race
static void cars_reset(ck_game *G) {
    ck_house *H = &G->house;
    float lane = CK_TRACK_CY - (CK_TRACK_OY + CK_TRACK_IY) / 2.0f;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *c = &H->gs[i];
        int slot = c->slot;
        memset(c, 0, sizeof *c);
        c->slot = slot; c->face = 1;
        c->x = CK_TRACK_CX - 22; c->y = lane + (H->np > 1 ? (i ? 9 : -9) : 0);
    }
    H->count = 3; H->win = -1; H->endT = 0; H->clock = 0;
}
static void cars_start(ck_game *G) { game_heroes(G); cars_reset(G); ck_go(G, SC_CARS); }
static float ell(float x, float y, float rx, float ry) { float a = (x - CK_TRACK_CX) / rx, b = (y - CK_TRACK_CY) / ry; return a * a + b * b; }
static void cars_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    if (H->count > 0) {
        int was = (int)ceilf(H->count);
        H->count -= 1.0f / 60;
        if ((int)ceilf(H->count) != was) ck_sfx(G, H->count <= 0 ? SFX_CONFIRM : SFX_TICK);
        return;
    }
    if (H->win >= 0) { if ((H->endT += 1.0f / 60) > 4) cars_reset(G); return; }
    H->clock += 1.0f / 60;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *c = &H->gs[i];
        int s = c->slot, steer = dir_x(G, s);
        c->a += steer * 0.055f * ck_clampf(c->s / 1.5f, -1, 1);
        if (HELD(G, s, CK_A)) c->s = ck_minf(3.2f, c->s + 0.07f);
        else if (HELD(G, s, CK_B)) c->s = ck_maxf(-1.2f, c->s - 0.09f);
        else c->s *= 0.985f;
        float px = c->x;
        c->x += cosf(c->a) * c->s;
        c->y += sinf(c->a) * c->s;
        float eo = ell(c->x, c->y, CK_TRACK_OX, CK_TRACK_OY), ei = ell(c->x, c->y, CK_TRACK_IX, CK_TRACK_IY);
        if (eo > 1 || ei < 1) {
            c->s = ck_clampf(c->s * 0.94f, -1.2f, 1.3f);
            if ((int)(G->t * 60) % 8 == 0) ck_puff(G, c->x, c->y, 1, 0x78C878, 200);
        }
        if (eo > 1.18f) { float k = sqrtf(1.18f / eo); c->x = CK_TRACK_CX + (c->x - CK_TRACK_CX) * k; c->y = CK_TRACK_CY + (c->y - CK_TRACK_CY) * k; }
        if (ei < 0.75f) { float k = sqrtf(0.75f / ei); c->x = CK_TRACK_CX + (c->x - CK_TRACK_CX) * k; c->y = CK_TRACK_CY + (c->y - CK_TRACK_CY) * k; }
        if (c->y > CK_TRACK_CY && px >= CK_TRACK_CX && c->x < CK_TRACK_CX) c->half = true;
        c->lapT += 1.0f / 60;
        if (c->y < CK_TRACK_CY && px < CK_TRACK_CX && c->x >= CK_TRACK_CX && c->half) {
            c->half = false;
            c->laps++;
            c->best = c->best > 0 ? ck_minf(c->best, c->lapT) : c->lapT;
            c->lapT = 0;
            ck_sfx(G, SFX_GEAR);
            if (c->laps >= CK_RACE_LAPS && H->win < 0) { H->win = i; ck_sfx(G, SFX_FLAG); ck_confetti(G, 160, 40, 40); }
        }
    }
    if (H->np > 1) {
        ck_hhero *a = &H->gs[0], *b = &H->gs[1];
        float d = hypotf(a->x - b->x, a->y - b->y);
        if (d < 14 && d > 0.01f) {
            float k = (14 - d) / 2 / d, ex = (a->x - b->x) * k, ey = (a->y - b->y) * k;
            a->x += ex; a->y += ey; b->x -= ex; b->y -= ey;
            a->s *= 0.9f; b->s *= 0.9f;
            ck_sfx(G, SFX_KICK);
        }
    }
}

// ---------------------------------------------------------------------------- the piano
static const int SEMI_WHITE[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
void ck_piano_key(int i, float *x, bool *black) {
    int o = i / 12, s = i % 12, w = SEMI_WHITE[s];
    if (w >= 0) { *x = CK_KEYS_X + (o * 7 + w) * CK_KEYS_W + CK_KEYS_W / 2.0f; *black = false; }
    else { *x = CK_KEYS_X + (o * 7 + SEMI_WHITE[s - 1] + 1) * CK_KEYS_W; *black = true; }
}
static void piano_start(ck_game *G) {
    ck_house *H = &G->house;
    game_heroes(G);
    for (int i = 0; i < H->np; i++) H->gs[i].key = i ? 16 : 7;
    memset(H->lit, 0, sizeof H->lit);
    H->nnotes = 0;
    ck_go(G, SC_PIANO);
}
static void play_key(ck_game *G, ck_hhero *p, int i) {
    ck_house *H = &G->house;
    static const uint32_t cols[6] = { 0xFF5A5A, 0xFFD23F, 0x49D17A, 0x5AD1FF, 0xC28BFF, 0xFF8FC7 };
    if (G->nnotes < 4) G->notes[G->nnotes++] = 261.63f * powf(2, i / 12.0f);
    H->lit[i] = 14;
    p->bounce = 10;
    float x;
    bool black;
    ck_piano_key(i, &x, &black);
    if (H->nnotes < 16) H->notes[H->nnotes++] = (ck_note_fx){ x, 140, 60, cols[i % 6] };
}
static void piano_update(ck_game *G) {
    ck_house *H = &G->house;
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->gs[i];
        int s = p->slot, d = dir_x(G, s);
        if (!d) p->rep = 0;
        else if (p->rep == 0 || (p->rep > 14 && p->rep % 5 == 0)) {
            int was = p->key;
            p->key = p->key + d < 0 ? 0 : p->key + d > CK_KEYS_N - 1 ? CK_KEYS_N - 1 : p->key + d;
            if (p->key != was && HELD(G, s, CK_A) && p->rep > 0) play_key(G, p, p->key);
        }
        if (d) p->rep++;
        if (HIT(G, s, CK_A)) play_key(G, p, p->key);
        if (HIT(G, s, CK_B)) play_key(G, p, p->key + 12 < CK_KEYS_N ? p->key + 12 : p->key - 12 >= 0 ? p->key - 12 : p->key);
        if (p->bounce > 0) p->bounce--;
    }
    for (int i = 0; i < CK_KEYS_N; i++) if (H->lit[i]) H->lit[i]--;
    int n = 0;
    for (int i = 0; i < H->nnotes; i++) { H->notes[i].y -= 0.8f; if (--H->notes[i].life > 0) H->notes[n++] = H->notes[i]; }
    H->nnotes = n;
}

void ck_house_step(ck_game *G) {
    switch (G->scene) {
    case SC_HOUSE: ck_house_update(G); break;
    case SC_PAINT: paint_update(G); break;
    case SC_AQUA: aqua_update(G); break;
    case SC_CARS: cars_update(G); break;
    case SC_PIANO: piano_update(G); break;
    }
}
