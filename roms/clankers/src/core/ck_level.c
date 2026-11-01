// Playing a level: heroes, Glitchies, crates, power-ups, the crane boss, the bolt shower and the
// jetpack side zone. A line-by-line port of levelScene() and jetScene() in web/game.js.
#include <math.h>
#include <string.h>

#include "ck_internal.h"

#define TS CK_TS
#define W  CK_W
#define H  CK_H

static const char CRATE_CHARS[] = "X?$1234";
static const char CRATE_HOLDS[] = { 0, 'G', 'o', 'M', 'R', 'B', 'N' };

static bool solid_char(char c) { return c == '#' || c == '=' || c == 'X' || c == 'D'; }

static char tile_at(const ck_level *lv, int tx, int ty) {
    if (tx < 0 || tx >= lv->w) return '#';
    if (ty < 0 || ty >= lv->h) return '.';
    return lv->tiles[ty][tx];
}
static bool is_solid(const ck_level *lv, int tx, int ty) { return solid_char(tile_at(lv, tx, ty)); }
static int fl(float v) { return (int)floorf(v); }

static void move_body(const ck_level *lv, ck_body *b) {
    b->x += b->vx;
    int x0 = fl(b->x / TS), x1 = fl((b->x + b->w - 0.01f) / TS), y0 = fl(b->y / TS), y1 = fl((b->y + b->h - 0.01f) / TS);
    b->hitWall = 0;
    if (b->vx > 0) {
        for (int ty = y0; ty <= y1; ty++) if (is_solid(lv, x1, ty)) { b->x = x1 * TS - b->w - 0.01f; b->vx = 0; b->hitWall = 1; break; }
    } else if (b->vx < 0) {
        for (int ty = y0; ty <= y1; ty++) if (is_solid(lv, x0, ty)) { b->x = (x0 + 1) * TS + 0.01f; b->vx = 0; b->hitWall = -1; break; }
    }
    float prevBottom = b->y + b->h;
    b->y += b->vy;
    b->ground = false;
    b->bumped = false;
    x0 = fl(b->x / TS);
    x1 = fl((b->x + b->w - 0.01f) / TS);
    if (b->vy > 0) {
        int ty = fl((b->y + b->h) / TS);
        for (int tx = x0; tx <= x1; tx++) {
            char t = tile_at(lv, tx, ty);
            if (solid_char(t) || (t == '-' && prevBottom <= ty * TS + 0.5f)) { b->y = ty * TS - b->h; b->vy = 0; b->ground = true; break; }
        }
    } else if (b->vy < 0) {
        int ty = fl(b->y / TS);
        for (int tx = x0; tx <= x1; tx++)
            if (is_solid(lv, tx, ty)) { b->y = (ty + 1) * TS; b->vy = 0; b->bumped = true; b->bumpTx = (int16_t)tx; b->bumpTy = (int16_t)ty; break; }
    }
}

typedef struct { float x, y, w, h; } rect;
static bool overlap_r(rect a, rect b) { return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y; }
static rect R(const ck_body *b) { return (rect){ b->x, b->y, b->w, b->h }; }
static bool overlap(const ck_body *a, const ck_body *b) { return overlap_r(R(a), R(b)); }

// ---------------------------------------------------------------------------- building

static ck_ent *spawn(ck_level *lv, int type) {
    ck_ent *e = NULL;
    if (lv->nents < CK_MAXENTS) e = &lv->ents[lv->nents++];
    else
        for (int i = lv->entCount; i < CK_MAXENTS; i++)
            if (lv->ents[i].dead && lv->ents[i].type != E_BUTTON) { e = &lv->ents[i]; break; }
    if (!e) e = &lv->ents[CK_MAXENTS - 1];   // full: reuse the last slot rather than crash
    memset(e, 0, sizeof *e);
    e->type = (uint8_t)type;
    e->uid = -1;
    return e;
}

static ck_ent *pickup(ck_game *G, int type, float x, float y) {
    ck_ent *e = spawn(&G->lv, type);
    e->x = x - 5; e->y = y - 5; e->w = 10; e->h = 10;
    e->t = ck_rand(G) * 6;
    return e;
}

static void enemy(ck_game *G, int kind, float x, float groundY) {
    static const float dims[3][2] = { { 14, 16 }, { 14, 18 }, { 14, 14 } };
    ck_ent *e = spawn(&G->lv, E_ENEMY);
    e->kind = (uint8_t)kind;
    e->x = x; e->y = groundY - dims[kind][1]; e->w = dims[kind][0]; e->h = dims[kind][1];
    e->vx = kind == EN_BUMBLE ? -0.45f : 0;
    e->face = -1;
    e->state = ES_GLITCHY;
    e->t = ck_rand(G) * 3;
    e->hopT = (int)(60 + ck_rand(G) * 60);
}

static ck_ent *box(ck_level *lv, int type, float x, float y, float w, float h) {
    ck_ent *e = spawn(lv, type);
    e->x = x; e->y = y; e->w = w; e->h = h;
    return e;
}

static void build(ck_game *G, int node) {
    ck_level *lv = &G->lv;
    const ck_leveldef *def = &G->defs[node];
    const int w = def->w, h = CK_ROWS;
    lv->def = def; lv->node = node; lv->w = w; lv->h = h;
    memset(lv->tiles, '.', sizeof lv->tiles);
    memset(lv->contents, 0, sizeof lv->contents);
    lv->nents = 0;
    lv->startX = 2 * TS; lv->startY = 12 * TS;
    int gearN = 0, outX = -1, outY = -1;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if (def->rows[y][x] == 'h') { outX = x; outY = y; }
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        char c = def->rows[y][x];
        const char *cr = strchr(CRATE_CHARS, c);
        float top = (float)((y + 1) * TS);
        if (c == '#' || c == '=' || c == '-' || c == 'D') lv->tiles[y][x] = c;
        else if (c && cr) { lv->tiles[y][x] = 'X'; lv->contents[y][x] = CRATE_HOLDS[cr - CRATE_CHARS]; }
        else if (c == 'o') { ck_ent *b = pickup(G, E_BOLT, x * TS + 8, y * TS + 8); b->orig = true; }
        else if (c == 'G') { ck_ent *e = pickup(G, E_GEAR, x * TS + 8, y * TS + 8); e->id = (int16_t)gearN++; }
        else if (c == 'b') enemy(G, EN_BUMBLE, x * TS + 2, top);
        else if (c == 's') enemy(G, EN_SPROCKET, x * TS + 2, top);
        else if (c == 'g') enemy(G, EN_GRUMBLE, x * TS + 2, top);
        else if (c == 'T') box(lv, E_TYRE, x * TS, top - 9, 16, 9);
        else if (c == 'L') box(lv, E_LEVER, x * TS + 3, top - 14, 10, 14);
        else if (c == 'v') box(lv, E_SIGN, x * TS + 2, top - 20, 12, 20);
        else if (c == 'P') box(lv, E_PAD, x * TS - 4, top - 6, 24, 6);
        else if (c == '@') { lv->startX = x * TS + 2; lv->startY = top; }
        else if (c == 'F') box(lv, E_FLAG, x * TS + 2, top - 34, 12, 34);
        else if (c == 'H') {
            if (outX >= 0) {
                ck_ent *e = box(lv, E_HATCH, x * TS - 4, top - 4, 24, 4);
                e->outx = outX * TS + 8; e->outy = (outY + 1) * TS;
                box(lv, E_HATCHOUT, outX * TS - 4, (outY + 1) * TS - 4, 24, 4);
            }
        } else if (c == 'k') {
            ck_ent *e = box(lv, E_MOVER, x * TS - 8, y * TS + 60, 32, 6);
            e->kind = MV_HOOK; e->px = x * TS + 8; e->py = y * TS + 4; e->len = 56;
        } else if (c == 'C') {
            int gy = y + 2;
            while (gy < h && !solid_char(lv->tiles[gy][x]) && !strchr("#=X?$1234", def->rows[gy][x])) gy++;
            ck_ent *e = box(lv, E_MOVER, x * TS, y * TS, 32, 32);
            e->kind = MV_CRUSHER; e->home = y * TS; e->low = gy * TS - 32; e->state = CR_UP; e->timer = 60 + x % 40;
        } else if (c == 'r') {
            int dx = 0, dy = 0, n = 0;
            if (x + 1 < w && def->rows[y][x + 1] == '~') dx = 1;
            else if (x > 0 && def->rows[y][x - 1] == '~') dx = -1;
            else if (y > 0 && def->rows[y - 1][x] == '|') dy = -1;
            else if (y + 1 < h && def->rows[y + 1][x] == '|') dy = 1;
            if (dx || dy) for (;;) {
                int yy = y + dy * (n + 1), xx = x + dx * (n + 1);
                if (yy < 0 || yy >= h || xx < 0 || xx >= w || def->rows[yy][xx] != (dx ? '~' : '|')) break;
                n++;
            }
            ck_ent *e = box(lv, E_MOVER, x * TS, y * TS, 32, 6);
            e->kind = MV_RAFT;
            e->ax = x * TS; e->ay = y * TS; e->bx = (x + dx * n) * TS; e->by2 = (y + dy * n) * TS;
            e->period = ck_maxf(2, hypotf((float)(dx * n), (float)(dy * n)) / 1.1f) * 2;
        }
    }
    int crateBolts = 0, origBolts = 0, foes = 0;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if (lv->contents[y][x] == 'o') crateBolts++;
    for (int i = 0; i < lv->nents; i++) {
        lv->ents[i].uid = (int16_t)i;
        if (lv->ents[i].orig) origBolts++;
        if (lv->ents[i].type == E_ENEMY) foes++;
    }
    lv->entCount = lv->nents;
    lv->boltTotal = origBolts + 3 * crateBolts;
    lv->foeTotal = foes;
    lv->doorT = -1; lv->bolts = 0; lv->boltGot = 0;
    for (int k = 0; k < 3; k++) { lv->got[k] = (G->prog.got[node] >> k) & 1; lv->gotNow[k] = false; }
}

bool ck_has(const ck_player *p, int power) {
    for (int k = 0; k < p->nstack; k++) if (p->stack[k] == power) return true;
    return false;
}
static float base_h(const ck_player *p) {
    return roundf((p->hero == HERO_ROBOT ? 26 : 30) * (ck_has(p, PW_MEGA) ? 1.5f : 1));
}

int ck_level_song(const ck_level *lv) {
    if (lv->def->song) return lv->def->song;
    return lv->def->kind == CK_KIND_BOSS ? SONG_BOSS : lv->def->kind == CK_KIND_BONUS ? SONG_BONUS : SONG_LEVEL;
}

void ck_level_start(ck_game *G, int node, bool retry, bool useSnap) {
    ck_level *lv = &G->lv;
    memset(lv, 0, sizeof *lv);
    build(G, node);
    const ck_snap *sn = &G->snap;
    if (useSnap && sn->valid) {
        memcpy(lv->tiles, sn->tiles, sizeof lv->tiles);
        lv->bolts = sn->bolts; lv->boltGot = sn->boltGot; lv->doorT = sn->doorT;
        memcpy(lv->gotNow, sn->gotNow, sizeof lv->gotNow);
        for (int u = 0; u < lv->entCount; u++) {
            if (sn->gone[u >> 3] & (1 << (u & 7))) lv->ents[u].dead = true;
            if (lv->ents[u].type == E_FLAG && u == sn->flag) lv->ents[u].on = true;
        }
        lv->startX = sn->x; lv->startY = sn->y;
        lv->timeF = sn->timeF;
    } else G->snap.valid = false;
    // the party: whoever joined on the select screen
    lv->nplayers = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        ck_player *p = &lv->players[lv->nplayers];
        memset(p, 0, sizeof *p);
        p->slot = s; p->i = lv->nplayers; p->hero = G->prog.hero[s]; p->col = G->prog.col[s] % CK_NCOLS;
        p->hex = ck_cols[p->col];
        p->h = base_h(p);
        p->x = lv->startX + p->i * 14; p->y = lv->startY - p->h; p->w = 12;
        p->face = 1;
        p->safeX = lv->startX; p->safeY = lv->startY - 30;
        p->inv = 90;
        lv->nplayers++;
    }
    if (G->prog.lives < 1) { G->prog.lives = 3; ck_save(G); }
    G->nparts = 0;
    lv->nameT = retry ? 120 : 150;
    lv->retry = retry;
    lv->clearT = -1; lv->dyingT = -1;
    if (lv->def->kind == CK_KIND_BOSS) { lv->hasCrane = true; lv->crane.rope = 140; }
    if (lv->def->kind == CK_KIND_BONUS) { lv->hasShower = true; lv->showerLeft = 30 * 60; lv->showerDone = -1; }
    ck_play(G, ck_level_song(lv));
}

static float speed(const ck_game *G) { return G->prog.little ? 1 : 1.5f; }

int ck_foes_left(const ck_level *lv) {
    int n = 0;
    for (int i = 0; i < lv->nents; i++) {
        const ck_ent *e = &lv->ents[i];
        if (e->type == E_ENEMY && !e->dead && e->state != ES_HAPPY) n++;
    }
    return n;
}

static void lose_life(ck_game *G, ck_player *p) {
    ck_level *lv = &G->lv;
    if (p->dead || p->bubble || lv->dyingT >= 0 || lv->clearT >= 0) return;
    G->prog.lives = G->prog.lives > 0 ? G->prog.lives - 1 : 0;
    p->nstack = 0; p->rainbowT = 0; p->magnetT = 0;
    ck_save(G);
    ck_player *buddy = NULL;
    for (int k = 0; k < lv->nplayers; k++) {
        ck_player *q = &lv->players[k];
        if (q != p && !q->dead && !q->bubble) { buddy = q; break; }
    }
    if (buddy && G->prog.lives > 0) {
        // co-op: float back to your buddy in a bubble, the level carries on
        p->bubble = 1; p->vx = 0; p->vy = 0; p->wob = 0;
        p->x = ck_clampf(p->x, lv->camX + 8, lv->camX + W - 20);
        p->y = ck_minf(p->y, H - 50);
        ck_sfx(G, SFX_OUCH);
        return;
    }
    for (int k = 0; k < lv->nplayers; k++) { ck_player *q = &lv->players[k]; if (!q->bubble) q->dead = q == p; }
    lv->nameT = 0; p->dead = true; p->vy = p->y > H ? -9 : -5.5f; p->vx = 0;
    if (p->y > H) p->y = H;
    lv->dyingT = 0;
    ck_play(G, SONG_NONE);
    ck_sfx(G, SFX_OUCH);
}

static void out_of_time(ck_game *G) {
    ck_level *lv = &G->lv;
    G->prog.lives = G->prog.lives > 0 ? G->prog.lives - 1 : 0;
    ck_save(G);
    for (int k = 0; k < lv->nplayers; k++) {
        ck_player *q = &lv->players[k];
        q->dead = true; q->bubble = 0; q->vy = -5.5f; q->vx = 0; q->nstack = 0;
    }
    lv->nameT = 0; lv->dyingT = 0; lv->timeUp = true;
    ck_play(G, SONG_NONE);
    ck_sfx(G, SFX_OUCH);
}

static void set_h(ck_player *p, float h) { p->y += p->h - h; p->h = h; }

static bool can_stand(const ck_level *lv, const ck_player *p, float h) {
    float top = p->y + p->h - h;
    int x0 = fl(p->x / TS), x1 = fl((p->x + p->w - 0.01f) / TS);
    for (int ty = fl(top / TS); ty <= fl((p->y - 0.01f) / TS); ty++)
        for (int tx = x0; tx <= x1; tx++) if (is_solid(lv, tx, ty)) return false;
    return true;
}

static void set_stack(ck_player *p, const uint8_t *stack, int n) {
    if (p->low) { p->low = false; p->slideT = 0; }
    for (int k = 0; k < n && k < 2; k++) p->stack[k] = stack[k];
    p->nstack = n > 2 ? 2 : n;
    set_h(p, base_h(p));
}

static void wobble(ck_game *G, ck_player *p, float fromX) {
    ck_level *lv = &G->lv;
    if (p->rainbowT > 0) return;
    if (p->nstack) {
        set_stack(p, p->stack, p->nstack - 1);
        p->inv = 110; p->wob = 14; p->vx = (p->x + p->w / 2 < fromX ? -2 : 2); p->vy = -2;
        ck_sfx(G, SFX_SHRINK);
        ck_puff(G, p->x + p->w / 2, p->y + p->h / 2, 8, 0xFFF8E6, 230);
        return;
    }
    if (lv->bolts <= 0) { lose_life(G, p); return; }   // no bolts to drop: that bump costs a life
    p->vx = (p->x + p->w / 2 < fromX ? -2.4f : 2.4f); p->vy = -2.6f; p->wob = 26; p->inv = 100;
    ck_sfx(G, SFX_WOBBLE);
    int drop = lv->bolts < 3 ? lv->bolts : 3;
    lv->bolts -= drop;
    for (int k = 0; k < drop; k++) {
        ck_ent *b = pickup(G, E_BOLT, p->x + p->w / 2, p->y + 6);
        b->fly = true; b->vx = (k - 1) * 1.2f + (ck_rand(G) - 0.5f); b->vy = -3 - ck_rand(G); b->delay = 40;
    }
}

static void reboot(ck_game *G, ck_ent *e) {
    if (e->state != ES_GLITCHY) return;
    e->state = ES_HAPPY; e->happyT = 0; e->vx = 0;
    ck_sfx(G, SFX_REBOOT);
    ck_sparks(G, e->x + e->w / 2, e->y + 4, 8, 0xBFF6FF);
    ck_ent *b = pickup(G, E_BOLT, e->x + e->w / 2, e->y);
    b->fly = true; b->vy = -3; b->vx = (ck_rand(G) - 0.5f) * 1.5f; b->delay = 10;
}

static void shut_crate(ck_game *G, ck_ent *e) {
    e->state = ES_SHELL; e->shellT = 0; e->vx = 0; e->kickT = 12; e->bounces = 0;
    ck_sfx(G, SFX_STOMP);
    ck_sparks(G, e->x + e->w / 2, e->y + 2, 5, 0xFFE2A8);
}
static void kick_crate(ck_game *G, ck_ent *e, int dir, int by) {
    e->state = ES_KICKED; e->dir = (int8_t)dir; e->spd = 3.2f; e->vx = 3.2f * dir; e->kickT = 14; e->by = (int16_t)by; e->bounces = 0;
    ck_sfx(G, SFX_KICK);
    ck_puff(G, e->x + e->w / 2, e->y + e->h, 4, 0xFFF8E6, 230);
}
static void wake_crate(ck_game *G, ck_ent *e) {
    e->state = ES_GLITCHY; e->vx = 0; e->vy = -2.5f; e->hopT = 60;
    ck_sfx(G, SFX_WOBBLE);
    ck_puff(G, e->x + e->w / 2, e->y + e->h / 2, 5, 0xFFF8E6, 230);
}

static int power_of(char c) {
    switch (c) {
    case 'M': return PW_MEGA;
    case 'R': return PW_RAINBOW;
    case 'B': return PW_BUBBLE;
    case 'N': return PW_MAGNET;
    }
    return PW_NONE;
}

static void break_crate(ck_game *G, int tx, int ty) {
    ck_level *lv = &G->lv;
    if (tile_at(lv, tx, ty) != 'X') return;
    lv->tiles[ty][tx] = '.';
    ck_debris(G, tx * TS, ty * TS);
    ck_sfx(G, SFX_CRATE);
    char c = lv->contents[ty][tx];
    if (c == 'o')
        for (int k = 0; k < 3; k++) {
            ck_ent *b = pickup(G, E_BOLT, tx * TS + 8, ty * TS + 8);
            b->orig = true; b->fly = true; b->vx = (k - 1) * 1.1f; b->vy = -3.2f; b->delay = 12;
        }
    if (power_of(c)) {
        ck_ent *e = box(lv, E_POWER, tx * TS + 2, ty * TS + 2, 12, 12);
        e->kind = (uint8_t)power_of(c); e->vy = -2.2f; e->rise = 18; e->dir = 1;
        ck_sfx(G, SFX_SPROUT);
    }
    if (c == 'G') {
        // crates hold the level's gear slot that is not otherwise placed
        bool placed[3] = { false, false, false };
        for (int i = 0; i < lv->nents; i++)
            if (lv->ents[i].type == E_GEAR && !lv->ents[i].dead && lv->ents[i].id < 3) placed[lv->ents[i].id] = true;
        ck_ent *gg = pickup(G, E_GEAR, tx * TS + 8, ty * TS + 8);
        gg->id = 0;
        for (int k = 0; k < 3; k++) if (!placed[k] && !lv->gotNow[k]) { gg->id = (int16_t)k; break; }
        gg->fly = true; gg->vy = -3.5f; gg->delay = 14;
    }
    lv->contents[ty][tx] = 0;
}

static void open_doors(ck_game *G) {
    if (G->lv.doorT >= 0) return;
    G->lv.doorT = 0;
    ck_sfx(G, SFX_DOOR);
}

static void collect(ck_game *G, ck_ent *e) {
    ck_level *lv = &G->lv;
    if (e->dead) return;
    e->dead = true;
    if (e->type == E_BOLT) { lv->bolts++; if (e->orig) lv->boltGot++; ck_sfx(G, SFX_BOLT); ck_sparks(G, e->x + 5, e->y + 5, 3, 0xFFD23F); }
    if (e->type == E_GEAR) { if (e->id >= 0 && e->id < 3) lv->gotNow[e->id] = true; ck_sfx(G, SFX_GEAR); ck_sparks(G, e->x + 5, e->y + 5, 14, 0xFFD23F); }
}

static void press_button(ck_game *G, ck_ent *e) {
    ck_level *lv = &G->lv;
    if (e->dead || !lv->hasCrane) return;
    e->dead = true;
    lv->crane.hits++; lv->crane.ouch = 50; lv->crane.shake = 30;
    ck_sfx(G, SFX_BUTTON); ck_sfx(G, SFX_CLANK);
    ck_sparks(G, e->x + 7, e->y, 10, 0xFF8A8A);
    if (lv->crane.hits >= 3) {
        lv->crane.fixed = true;
        ck_sfx(G, SFX_REBOOT); ck_sfx(G, SFX_OPEN);
        ck_confetti(G, 160, 60, 30);
    }
}

static void spawn_button(ck_game *G) {
    ck_level *lv = &G->lv;
    static const float buttons[3][2] = { { 3, 12 }, { 4.5f, 8 }, { 14.5f, 8 } };
    if (!lv->hasCrane || lv->crane.fixed || lv->crane.hits >= 3) return;
    for (int i = 0; i < lv->nents; i++) if (lv->ents[i].type == E_BUTTON && !lv->ents[i].dead) return;
    const float *b = buttons[lv->crane.hits];
    box(lv, E_BUTTON, b[0] * TS + 1, b[1] * TS - 6, 14, 6);
}

static rect action_box(const ck_player *p) {
    if (p->hero == HERO_ROBOT)
        return p->face > 0 ? (rect){ p->x + p->w, p->y + p->h - 18, 30, 10 } : (rect){ p->x - 30, p->y + p->h - 18, 30, 10 };
    return p->face > 0 ? (rect){ p->x + p->w - 2, p->y + p->h - 16, 18, 14 } : (rect){ p->x - 16, p->y + p->h - 16, 18, 14 };
}

static void do_action(ck_game *G, ck_player *p) {
    ck_level *lv = &G->lv;
    rect bx = action_box(p);
    for (int i = 0; i < lv->nents; i++) {
        ck_ent *e = &lv->ents[i];
        if (e->dead || !overlap_r(bx, R(&e->body))) continue;
        if (e->type == E_ENEMY) reboot(G, e);
        else if (e->type == E_LEVER && !e->on) { e->on = true; ck_sfx(G, SFX_LEVER); open_doors(G); }
        else if (e->type == E_BOLT && p->hero == HERO_ROBOT && e->delay <= 0) collect(G, e);
        else if (e->type == E_BUTTON) press_button(G, e);
    }
    int x0 = fl(bx.x / TS), x1 = fl((bx.x + bx.w) / TS), y0 = fl(bx.y / TS), y1 = fl((bx.y + bx.h) / TS);
    for (int ty = y0; ty <= y1; ty++) for (int tx = x0; tx <= x1; tx++) break_crate(G, tx, ty);
}

static void start_clear(ck_game *G) {
    ck_level *lv = &G->lv;
    if (lv->clearT >= 0) return;
    lv->clearT = 0;
    ck_play(G, SONG_RESCUE);
    const ck_leveldef *def = lv->def;
    bool kindLevel = def->kind == CK_KIND_LEVEL;
    bool sweep = lv->boltGot >= lv->boltTotal && ck_foes_left(lv) == 0;
    bool fast = def->par ? lv->timeF <= def->par * 60 : def->time != 0;
    uint32_t best = G->prog.best[lv->node];
    ck_result *r = &lv->result;
    memset(r, 0, sizeof *r);
    r->time = lv->timeF; r->sweep = sweep; r->fast = fast;
    r->record = best != 0 && (uint32_t)lv->timeF < best;
    r->first = best == 0;
    r->hasStars = kindLevel;
    r->stars[0] = true; r->stars[1] = sweep; r->stars[2] = fast;
    for (int k = 0; k < 3; k++) r->had[k] = (G->prog.stars[lv->node] >> k) & 1;
    lv->hasResult = true;
    for (int k = 0; k < lv->nplayers; k++) { ck_player *p = &lv->players[k]; p->vx = 0; p->cheer = true; p->bubble = 0; }
    ck_confetti(G, 160, 20, 50);
}

void ck_level_finish(ck_game *G) {
    ck_level *lv = &G->lv;
    int id = lv->node;
    int before = G->prog.got[id], now = before;
    for (int k = 0; k < 3; k++) if (lv->gotNow[k]) now |= 1 << k;
    int newG = __builtin_popcount((unsigned)now) - __builtin_popcount((unsigned)before);
    G->prog.got[id] = (uint8_t)now;
    if (lv->hasResult) {
        uint32_t t = (uint32_t)(lv->result.time ? lv->result.time : 1);
        if (!G->prog.best[id] || t < G->prog.best[id]) G->prog.best[id] = t;
        if (lv->result.hasStars) {
            int s = G->prog.stars[id];
            for (int k = 0; k < 3; k++) if (lv->result.stars[k]) s |= 1 << k;
            G->prog.stars[id] = (uint8_t)s;
        }
    }
    G->prog.at = (uint8_t)id;
    ck_map_reset(G);
    ck_map_beat(G, lv->bolts, newG);
    ck_save(G);
}

// ---------------------------------------------------------------------------- enemies

static void update_enemy(ck_game *G, ck_ent *e) {
    ck_level *lv = &G->lv;
    float sp = speed(G);
    e->vy = ck_minf(e->vy + 0.3f, 5);
    if (e->state == ES_BUBBLED) {
        e->bubT++; e->y -= 0.5f; e->x += sinf(e->bubT * 0.15f) * 0.4f;
        if (e->bubT > 70) { e->state = ES_GLITCHY; ck_sfx(G, SFX_POP); reboot(G, e); }
        return;
    }
    if (e->state == ES_HAPPY) {
        e->happyT++; e->vx = 0;
        if (e->happyT == 24) {
            e->dead = true;
            ck_puff(G, e->x + e->w / 2, e->y + e->h / 2, 10, 0xFFF8E6, 230);
            ck_sparks(G, e->x + e->w / 2, e->y + e->h / 2, 8, 0xBFF6FF);
            ck_sfx(G, SFX_POP);
        }
        return;
    }
    if (e->kickT > 0) e->kickT--;
    if (e->state == ES_CARRIED) {
        ck_player *c = NULL;
        for (int k = 0; k < lv->nplayers; k++) if (lv->players[k].carry == e) c = &lv->players[k];
        if (!c || c->bubble || c->dead || c->wob > 0 || c->low) {
            if (c) c->carry = NULL;
            e->state = ES_SHELL; if (e->shellT > 420) e->shellT = 420; e->kickT = 20; e->vy = -1;
            return;
        }
        e->x = c->x + c->w / 2 - e->w / 2; e->y = c->y - e->h + 3; e->vy = 0;
        if (e->shellT > 300) e->shellT = 300;
        return;
    }
    if (e->state == ES_SHELL) {
        e->vx = 0;
        move_body(lv, &e->body);
        if (e->y > H + 30) e->dead = true;
        if (++e->shellT > 600) wake_crate(G, e);   // wiggles from 480, then pops back open
        return;
    }
    if (e->state == ES_KICKED) {
        if (e->ground) e->spd = 3.2f;
        e->vx = e->spd * e->dir;
        int pdir = e->dir;
        move_body(lv, &e->body);
        if (e->hitWall) {
            int tx = pdir > 0 ? fl((e->x + e->w + 1) / TS) : fl((e->x - 1) / TS);
            for (int ty = fl(e->y / TS); ty <= fl((e->y + e->h - 1) / TS); ty++) break_crate(G, tx, ty);
            e->dir = (int8_t)-pdir;
            ck_sfx(G, SFX_CLANK);
            ck_puff(G, pdir > 0 ? e->x + e->w : e->x, e->y + e->h / 2, 3, 0xFFF8E6, 230);
            if (++e->bounces > 6) { e->state = ES_GLITCHY; reboot(G, e); return; }
        }
        if (e->bumped) break_crate(G, e->bumpTx, e->bumpTy);
        for (int i = 0; i < lv->nents; i++) {
            ck_ent *o = &lv->ents[i];
            if (o == e || o->dead || !overlap(&o->body, &e->body)) continue;
            if (o->type == E_ENEMY && (o->state == ES_GLITCHY || o->state == ES_SHELL || o->state == ES_KICKED)) {
                o->state = ES_GLITCHY; reboot(G, o); ck_sfx(G, SFX_CLANK);
            } else if (o->type == E_LEVER && !o->on) { o->on = true; ck_sfx(G, SFX_LEVER); open_doors(G); }
            else if (o->type == E_BUTTON) press_button(G, o);
        }
        if (e->y > H + 30 || ck_absf(e->x - lv->camX - W / 2) > W * 1.5f) e->dead = true;
        return;
    }
    if (e->kind == EN_BUMBLE) {
        e->vx = 0.45f * sp * e->face;
        // turn at walls and ledges
        float ahead = e->face > 0 ? e->x + e->w + 1 : e->x - 1;
        int below = fl((e->y + e->h + 2) / TS);
        if (!is_solid(lv, fl(ahead / TS), below) && tile_at(lv, fl(ahead / TS), below) != '-') e->face = (int8_t)-e->face;
    } else if (e->kind == EN_SPROCKET) {
        e->vx = 0;
        if (e->ground && --e->hopT <= 0) { e->vy = -3.6f; e->hopT = (int)(80 / sp); }
    } else {
        ck_player *near = NULL;
        float nd = 1e9f;
        for (int k = 0; k < lv->nplayers; k++) {
            float d = ck_absf(lv->players[k].x - e->x);
            if (d < nd) { nd = d; near = &lv->players[k]; }
        }
        if (near && nd < 120) e->face = near->x > e->x ? 1 : -1;
        e->vx = (nd < 120 ? 0.4f : 0.2f) * sp * e->face;
        float ahead = e->face > 0 ? e->x + e->w + 1 : e->x - 1;
        int below = fl((e->y + e->h + 2) / TS);
        if (!is_solid(lv, fl(ahead / TS), below)) { e->face = (int8_t)-e->face; e->vx = 0; }
    }
    move_body(lv, &e->body);
    if (e->hitWall) e->face = (int8_t)-e->face;
    if (e->y > H + 30) e->dead = true;
}

// ---------------------------------------------------------------------------- one tick

static void update_players(ck_game *G, int *outcome) {
    ck_level *lv = &G->lv;
    for (int pi = 0; pi < lv->nplayers; pi++) {
        ck_player *p = &lv->players[pi];
        if (p->bubble) {
            ck_player *b = p;
            for (int k = 0; k < lv->nplayers; k++) if (&lv->players[k] != p && !lv->players[k].bubble) { b = &lv->players[k]; break; }
            p->bubble++;
            float tx = b->x - b->face * 4, ty = b->y - 26;
            p->x += (tx - p->x) * 0.05f;
            p->y += (ty - p->y) * 0.05f + sinf(lv->t * 4) * 0.3f;
            if ((p->bubble > 50 && hypotf(tx - p->x, ty - p->y) < 14) || (HIT(G, p->slot, CK_A) && p->bubble > 50)) {
                p->bubble = 0; p->inv = 90; p->vy = -2;
                ck_sfx(G, SFX_POP);
                ck_puff(G, p->x + p->w / 2, p->y + p->h / 2, 8, 0xBEF0FF, 230);
            }
            continue;
        }
        const int i = p->slot;
        p->anim += ck_absf(p->vx) * 0.055f + 0.004f;
        if (p->inv > 0) p->inv--;
        if (p->jetT > 0) p->jetT--;
        if (p->actT > 0) p->actT--;
        if (p->rainbowT > 0 && --p->rainbowT == 0) {
            bool other = false;
            for (int k = 0; k < lv->nplayers; k++) if (lv->players[k].rainbowT > 0) other = true;
            if (!other) ck_play(G, ck_level_song(lv));
        }
        if (p->magnetT > 0) p->magnetT--;
        float max = (p->hero == HERO_ROBOT ? 1.75f : 2.2f) * (p->rainbowT > 0 ? 1.35f : 1), acc = p->ground ? 0.32f : 0.2f;
        int dir = 0;
        if (p->wob > 0) p->wob--;
        else dir = (HELD(G, i, CK_RIGHT) ? 1 : 0) - (HELD(G, i, CK_LEFT) ? 1 : 0);
        if (dir) { p->vx = ck_clampf(p->vx + dir * acc, -max, max); p->face = dir; }
        else if (p->wob <= 0) { p->vx *= p->ground ? 0.7f : 0.94f; if (ck_absf(p->vx) < 0.06f) p->vx = 0; }
        if (p->ground) { p->coyote = 7; p->boost = false; }
        else if (p->coyote > 0) p->coyote--;
        if (HIT(G, i, CK_DOWN) && p->ground && !p->low && p->wob <= 0) {
            for (int k = 0; k < lv->nents; k++) {
                ck_ent *e = &lv->ents[k];
                if (e->type != E_HATCH || e->dead) continue;
                if (!overlap_r(R(&p->body), (rect){ e->x + 2, e->y - 6, e->w - 4, 12 })) continue;
                ck_sfx(G, SFX_HATCH);
                for (int q = 0; q < lv->nplayers; q++) {
                    ck_player *pq = &lv->players[q];
                    pq->bubble = 0;
                    if (pq->low) { set_h(pq, base_h(pq)); pq->low = false; }
                }
                ck_jet_start(G, k);
                *outcome = LV_HATCH;
                return;
            }
        }
        // Down: crouch when standing, slide when running. Low = 14 px tall, so you fit under 1-tile gaps.
        float fullH = base_h(p);
        if (HELD(G, i, CK_DOWN) && p->ground && p->wob <= 0 && !p->low) {
            if (ck_absf(p->vx) > 1.1f) { p->slideT = 36; p->vx = p->face * 3.2f; ck_sfx(G, SFX_SLIDE); }
            set_h(p, ck_has(p, PW_MEGA) ? 20 : 14);
            p->low = true;
        }
        if (p->low) {
            if (p->slideT > 0) {
                p->slideT--;
                p->vx = p->face * ck_maxf(0.9f, ck_absf(p->vx) * 0.975f);
                if (lv->frame % 4 == 0) ck_puff(G, p->x + p->w / 2 - p->face * 6, p->y + p->h, 1, 0xFFF8E6, 230);
            } else p->vx = ck_clampf(p->vx, -0.9f, 0.9f);
            if ((!HELD(G, i, CK_DOWN) && p->slideT <= 0) || !p->ground) {
                if (can_stand(lv, p, fullH)) { set_h(p, fullH); p->low = false; p->slideT = 0; }
                else if (p->slideT <= 0 && !HELD(G, i, CK_DOWN) && dir) p->vx = dir * 0.9f;
            }
        }
        if (HIT(G, i, CK_A) && p->wob <= 0 && p->low) {
            if (can_stand(lv, p, fullH)) { set_h(p, fullH); p->low = false; p->slideT = 0; }
        }
        if (HIT(G, i, CK_A) && p->wob <= 0 && !p->low) {
            if (p->coyote > 0) {
                p->vy = p->hero == HERO_ROBOT ? -5.5f : -6.1f; p->coyote = 0; p->ground = false;
                ck_sfx(G, SFX_JUMP);
                ck_puff(G, p->x + p->w / 2, p->y + p->h, 3, 0xFFF8E6, 230);
            } else if (p->hero == HERO_ROBOT && !p->boost) {
                p->vy = -4.8f; p->boost = true; p->jetT = 22;
                ck_sfx(G, SFX_BOOST);
            }
        }
        if (!HELD(G, i, CK_A) && p->vy < -2.2f && p->jetT <= 0 && !p->spring) p->vy = -2.2f;
        p->flapping = false;
        if (p->hero == HERO_OSTRICH && HELD(G, i, CK_A) && p->vy > 0.6f && !p->ground) {
            p->vy = 0.6f; p->flapping = true;
            if (p->flapT-- <= 0) { ck_sfx(G, SFX_FLAP); p->flapT = 14; }
        }
        p->vy = ck_minf(p->vy + 0.3f, 6);
        if (p->vy >= 0) p->spring = false;
        if (p->actT <= 0) p->shoot = false;
        ck_ent *nearCrate = NULL;
        if (!p->carry && HIT(G, i, CK_B) && p->actT <= 0 && p->wob <= 0 && !p->low) {
            rect reach = { p->x - 10, p->y - 4, p->w + 20, p->h + 8 };
            for (int k = 0; k < lv->nents; k++) {
                ck_ent *e = &lv->ents[k];
                if (e->type == E_ENEMY && e->state == ES_SHELL && !e->dead && overlap_r(R(&e->body), reach)) { nearCrate = e; break; }
            }
        }
        if (p->carry && HIT(G, i, CK_B)) {
            ck_ent *e = p->carry;
            p->carry = NULL;
            e->x = p->face > 0 ? p->x + p->w + 1 : p->x - e->w - 1;
            e->y = ck_minf(e->y, p->y + p->h - e->h);
            if (is_solid(lv, fl((e->x + e->w / 2) / TS), fl((e->y + e->h / 2) / TS))) e->x = p->x + p->w / 2 - e->w / 2;
            kick_crate(G, e, p->face, p->slot);
            e->vy = HELD(G, i, CK_UP) ? -6.5f : -2;
            if (HELD(G, i, CK_UP)) e->spd = 1.6f;
            p->actT = 12; p->shoot = true;
        } else if (nearCrate) {
            p->carry = nearCrate; nearCrate->state = ES_CARRIED; nearCrate->vx = 0;
            ck_sfx(G, SFX_GRAB);
            p->actT = 8; p->shoot = true;
        } else if (HIT(G, i, CK_B) && p->actT <= 0 && p->wob <= 0 && ck_has(p, PW_BUBBLE)) {
            int mine = 0;
            for (int k = 0; k < lv->nents; k++) if (lv->ents[k].type == E_SHOT && !lv->ents[k].dead && lv->ents[k].owner == p->slot) mine++;
            if (mine < 2) {
                ck_ent *s = box(lv, E_SHOT, p->face > 0 ? p->x + p->w : p->x - 10, p->y + p->h * 0.3f, 10, 10);
                s->owner = (int16_t)p->slot; s->vx = p->face * 2.4f + p->vx * 0.4f; s->life = 110;
                p->actT = 12; p->shoot = true;
                ck_sfx(G, SFX_BLOW);
            }
        } else if (HIT(G, i, CK_B) && p->actT <= 0 && p->wob <= 0) {
            p->actT = 22;
            ck_sfx(G, p->hero == HERO_ROBOT ? SFX_GRAB : SFX_KICK);
        }
        if (p->actT >= 6 && p->actT <= 18 && !p->shoot) do_action(G, p);
        if (p->onMover) { p->x += p->onMover->dx; p->y += p->onMover->dy; }
        bool wasGround = p->ground || p->onMover;
        float pvy = p->vy, prevBottom = p->y + p->h;
        move_body(lv, &p->body);
        p->onMover = NULL;
        for (int k = 0; k < lv->nents; k++) {
            ck_ent *m = &lv->ents[k];
            if (m->type != E_MOVER || m->kind != MV_CRUSHER || !overlap(&p->body, &m->body) || !(prevBottom > m->y + 4)) continue;
            float out = p->x + p->w / 2 < m->x + 16 ? m->x - p->w - 0.5f : m->x + m->w + 0.5f;
            if (m->state == CR_DROP && p->y + p->h > m->y + m->h - 10) { p->x = out; if (p->inv <= 0) wobble(G, p, m->x + 16); else p->vx = 0; }
            else { p->x = out; p->vx = 0; }
        }
        if (p->vy >= 0 && !p->ground)
            for (int k = 0; k < lv->nents; k++) {
                ck_ent *m = &lv->ents[k];
                if (m->type != E_MOVER) continue;
                if (p->x + p->w > m->x + 1 && p->x < m->x + m->w - 1 && prevBottom <= m->y + 2 + ck_maxf(0, m->dy) && p->y + p->h >= m->y - 0.5f) {
                    p->y = m->y - p->h; p->vy = 0; p->ground = true; p->onMover = m;
                    break;
                }
            }
        if (ck_has(p, PW_MEGA)) {
            if (p->hitWall) {
                int tx = p->hitWall > 0 ? fl((p->x + p->w + 1) / TS) : fl((p->x - 1) / TS);
                for (int ty = fl(p->y / TS); ty <= fl((p->y + p->h - 1) / TS); ty++) break_crate(G, tx, ty);
            }
            if (p->ground && !wasGround && pvy > 3) { lv->shake = 10; ck_sfx(G, SFX_STOMP); ck_puff(G, p->x + p->w / 2, p->y + p->h, 6, 0xFFF8E6, 230); }
        }
        if (p->bumped) break_crate(G, p->bumpTx, p->bumpTy);
        if (p->ground && !wasGround) ck_puff(G, p->x + p->w / 2, p->y + p->h, 2, 0xFFF8E6, 230);
        if (p->ground) {
            int l = fl((p->x + 1) / TS), r = fl((p->x + p->w - 1) / TS), ty = fl((p->y + p->h + 1) / TS);
            char a = tile_at(lv, l, ty), b = tile_at(lv, r, ty);
            bool okA = a == '#' || a == '=' || a == '-', okB = b == '#' || b == '=' || b == '-';
            if (okA && okB) { p->safeX = p->x; p->safeY = p->y; }
        }
        if (p->y > H + 30) lose_life(G, p);
        if (p->x < 0) { p->x = 0; p->vx = 0; }
        if (p->x > lv->w * TS - p->w) { p->x = lv->w * TS - p->w; p->vx = 0; }
    }
}

static void update_ents(ck_game *G) {
    ck_level *lv = &G->lv;
    for (int ei = 0; ei < lv->nents; ei++) {
        ck_ent *e = &lv->ents[ei];
        if (e->dead) continue;
        e->t += 1.0f / 60;
        if (e->type == E_BOLT || e->type == E_GEAR) {
            if (e->delay > 0) e->delay--;
            ck_player *mag = NULL;
            if (e->delay <= 0)
                for (int k = 0; k < lv->nplayers; k++) {
                    ck_player *p = &lv->players[k];
                    if (p->magnetT > 0 && !p->bubble && hypotf(p->x + p->w / 2 - e->x - 5, p->y + p->h / 2 - e->y - 5) < 150) { mag = p; break; }
                }
            if (mag) {
                e->fly = false;
                e->x += (mag->x + mag->w / 2 - 5 - e->x) * 0.12f;
                e->y += (mag->y + mag->h / 2 - 5 - e->y) * 0.12f;
            } else if (e->fly) {
                e->vy = ck_minf(e->vy + 0.2f, 4);
                float pv = e->vy;
                move_body(lv, &e->body);
                if (e->ground) {
                    e->vy = pv > 1.2f ? -pv * 0.4f : 0;
                    e->vx *= 0.8f;
                    if (ck_absf(e->vy) < 0.3f && ck_absf(e->vx) < 0.1f) e->fly = false;
                }
                if (e->y > H + 20) e->dead = true;
            }
            for (int k = 0; k < lv->nplayers; k++) {
                ck_player *p = &lv->players[k];
                if (!p->bubble && e->delay <= 0 && overlap(&p->body, &e->body)) collect(G, e);
            }
        } else if (e->type == E_ENEMY) {
            update_enemy(G, e);
            if (e->dead) continue;
            if (e->state == ES_BUBBLED)
                for (int k = 0; k < lv->nplayers; k++) {
                    ck_player *p = &lv->players[k];
                    if (!p->bubble && overlap(&p->body, &e->body)) { e->bubT = 999; if (p->vy > 0) p->vy = -4; }
                }
            if ((e->state == ES_SHELL || e->state == ES_KICKED) && e->kickT <= 0)
                for (int k = 0; k < lv->nplayers; k++) {
                    ck_player *p = &lv->players[k];
                    if (p->bubble || p->carry || !overlap(&p->body, &e->body)) continue;
                    bool fromTop = p->vy > 0 && p->y + p->h - p->vy <= e->y + 5;
                    if (e->state == ES_KICKED && fromTop) {
                        e->state = ES_SHELL; e->shellT = 0; e->vx = 0; e->kickT = 10; p->vy = -4.4f;
                        ck_sfx(G, SFX_STOMP);
                        ck_puff(G, e->x + e->w / 2, e->y + e->h, 4, 0xFFF8E6, 230);
                    } else if (e->state == ES_SHELL) {
                        kick_crate(G, e, p->x + p->w / 2 < e->x + e->w / 2 ? 1 : -1, p->slot);
                        if (fromTop) p->vy = -4.4f;
                    }
                    break;
                }
            if (e->state == ES_GLITCHY)
                for (int k = 0; k < lv->nplayers; k++) {
                    ck_player *p = &lv->players[k];
                    if (p->bubble || !overlap(&p->body, &e->body)) continue;
                    if (p->slideT > 0 || p->rainbowT > 0) { reboot(G, e); continue; }
                    if (p->vy > 0 && p->y + p->h - p->vy <= e->y + 5) {
                        if (e->kind == EN_GRUMBLE) shut_crate(G, e); else reboot(G, e);
                        bool spr = e->kind == EN_SPROCKET;
                        p->vy = HELD(G, p->slot, CK_A) ? (spr ? -8.6f : -6.2f) : (spr ? -7.6f : -4.4f);
                        p->spring = spr; p->boost = false;
                    } else if (p->inv <= 0) wobble(G, p, e->x + e->w / 2);
                }
        } else if (e->type == E_POWER) {
            e->t += 1.0f / 60;
            if (e->rise > 0) {
                e->rise--; e->y += e->vy * 0.5f;
                if (e->rise == 0) { e->vy = 0; e->vx = 0.7f * e->dir; }
            } else {
                e->vy = ck_minf(e->vy + 0.25f, 4);
                move_body(lv, &e->body);
                if (e->hitWall) e->dir = (int8_t)-e->dir;
                e->vx = 0.7f * e->dir;
                if (e->kind == PW_RAINBOW && e->ground) e->vy = -3.6f;
                if (e->y > H + 20) e->dead = true;
                for (int k = 0; k < lv->nplayers; k++) {
                    ck_player *p = &lv->players[k];
                    if (p->bubble || e->dead || !overlap(&p->body, &e->body)) continue;
                    e->dead = true;
                    ck_sfx(G, SFX_POWER);
                    ck_sparks(G, e->x + 6, e->y + 6, 14, 0xFFF6A0);
                    strcpy(lv->toast, ck_power_names[e->kind]);
                    lv->toastT = 110;
                    if (e->kind == PW_MEGA || e->kind == PW_BUBBLE) {
                        uint8_t st[2]; int n = 0;
                        for (int s = 0; s < p->nstack; s++) if (p->stack[s] != e->kind) st[n++] = p->stack[s];
                        if (n < 2) st[n++] = e->kind;
                        set_stack(p, st, n);
                    } else if (e->kind == PW_RAINBOW) { p->rainbowT = 480; ck_play(G, SONG_RAINBOW); }
                    else p->magnetT = 1200;
                }
            }
        } else if (e->type == E_SHOT) {
            e->t += 1; e->x += e->vx; e->vx *= 0.985f; e->y += sinf(e->t * 0.2f) * 0.4f - 0.06f;
            int tx = fl((e->x + 5) / TS), ty = fl((e->y + 5) / TS);
            bool pop = --e->life <= 0;
            if (tile_at(lv, tx, ty) == 'X') { break_crate(G, tx, ty); pop = true; }
            else if (is_solid(lv, tx, ty)) pop = true;
            for (int k = 0; k < lv->nents; k++) {
                ck_ent *en = &lv->ents[k];
                if (en->type == E_ENEMY && !en->dead && en->state == ES_GLITCHY && overlap(&e->body, &en->body)) {
                    en->state = ES_BUBBLED; en->bubT = 0; en->vx = 0; pop = true;
                    ck_sfx(G, SFX_BLOW);
                    break;
                }
            }
            if (pop) { e->dead = true; ck_puff(G, e->x + 5, e->y + 5, 3, 0xBEF0FF, 230); }
        } else if (e->type == E_TYRE) {
            if (e->sq > 0) e->sq -= 0.08f;
            for (int k = 0; k < lv->nplayers; k++) {
                ck_player *p = &lv->players[k];
                if (p->vy > 0 && p->x + p->w > e->x + 1 && p->x < e->x + e->w - 1 && p->y + p->h >= e->y && p->y + p->h - p->vy <= e->y + 6) {
                    p->y = e->y - p->h; p->vy = HELD(G, p->slot, CK_A) ? -9.4f : -8.4f; p->spring = true; p->boost = false; e->sq = 1;
                    ck_sfx(G, SFX_SPRING);
                }
            }
        } else if (e->type == E_LEVER) {
            if (!e->on)
                for (int k = 0; k < lv->nplayers; k++)   // bumping it counts too
                    if (overlap_r(R(&lv->players[k].body), (rect){ e->x, 0, e->w, e->y + e->h })) { e->on = true; ck_sfx(G, SFX_LEVER); open_doors(G); break; }
        } else if (e->type == E_BUTTON) {
            for (int k = 0; k < lv->nplayers; k++) {
                ck_player *p = &lv->players[k];
                if (p->vy > 0 && overlap(&p->body, &e->body) && p->y + p->h - p->vy <= e->y + 4) { press_button(G, e); p->vy = -5; }
            }
        } else if (e->type == E_FLAG) {
            if (!e->on)
                for (int k = 0; k < lv->nplayers; k++) {
                    ck_player *p = &lv->players[k];
                    if (p->bubble || !overlap(&p->body, &e->body)) continue;
                    e->on = true;
                    ck_sfx(G, SFX_FLAG);
                    ck_sparks(G, e->x + 6, e->y + 4, 14, 0x9DFFC0);
                    strcpy(lv->toast, "CHECKPOINT!");
                    lv->toastT = 100;
                    ck_snap *sn = &G->snap;
                    sn->valid = true;
                    sn->x = e->x + 6; sn->y = e->y + e->h; sn->flag = e->uid; sn->timeF = lv->timeF;
                    sn->bolts = lv->bolts; sn->boltGot = lv->boltGot; sn->doorT = lv->doorT;
                    memcpy(sn->gotNow, lv->gotNow, sizeof sn->gotNow);
                    memcpy(sn->tiles, lv->tiles, sizeof sn->tiles);
                    memset(sn->gone, 0, sizeof sn->gone);
                    for (int u = 0; u < lv->entCount; u++) if (lv->ents[u].dead) sn->gone[u >> 3] |= (uint8_t)(1 << (u & 7));
                    break;
                }
        } else if (e->type == E_PAD) {
            for (int k = 0; k < lv->nplayers; k++) {   // the whole column above the pad counts
                ck_player *p = &lv->players[k];
                if (!p->bubble && overlap_r(R(&p->body), (rect){ e->x + 4, 0, e->w - 8, e->y + 6 })) start_clear(G);
            }
        }
    }
}

static void update_crane(ck_game *G) {
    ck_level *lv = &G->lv;
    float sp = G->prog.little ? 0.9f : 1.25f;
    lv->crane.ang = lv->crane.fixed ? lv->crane.ang * 0.96f : 0.95f * cosf(lv->t * sp);   // starts out at the side
    if (lv->crane.shake > 0) lv->crane.shake--;
    if (lv->crane.ouch > 0) lv->crane.ouch--;
    if (lv->frame > 60) spawn_button(G);
    lv->crane.rope = lv->crane.fixed ? ck_maxf(40, lv->crane.rope - 1.2f) : 140;   // fixed: reel the magnet up
    float mx = 160 + sinf(lv->crane.ang) * lv->crane.rope, my = 24 + cosf(lv->crane.ang) * lv->crane.rope;
    lv->crane.mx = mx; lv->crane.my = my;
    if (!lv->crane.fixed)
        for (int k = 0; k < lv->nplayers; k++) {
            ck_player *p = &lv->players[k];
            float cx = ck_clampf(mx, p->x, p->x + p->w), cy = ck_clampf(my, p->y, p->y + p->h);
            if (!p->bubble && (cx - mx) * (cx - mx) + (cy - my) * (cy - my) < 11 * 11 && p->inv <= 0) wobble(G, p, mx);
        }
    if (lv->crane.fixed && !lv->crane.padShown && lv->crane.ouch <= 0) {
        lv->crane.padShown = true;
        box(lv, E_PAD, 9 * TS - 4, 12 * TS - 6, 24, 6);
        ck_sfx(G, SFX_OPEN);
    }
}

static void update_movers(ck_game *G) {
    ck_level *lv = &G->lv;
    bool little = G->prog.little;
    for (int k = 0; k < lv->nents; k++) {
        ck_ent *m = &lv->ents[k];
        if (m->type != E_MOVER || m->dead) continue;
        float ox = m->x, oy = m->y, sp = little ? 1 : 1.25f;
        if (m->kind == MV_RAFT) {
            float q = (1 - cosf(lv->t * sp * CK_PI * 2 / m->period)) / 2;
            m->x = m->ax + (m->bx - m->ax) * q;
            m->y = m->ay + (m->by2 - m->ay) * q;
        } else if (m->kind == MV_CRUSHER) {
            m->timer--;
            if (m->state == CR_UP && m->timer <= 0) { m->state = CR_WARN; m->timer = little ? 48 : 34; }
            else if (m->state == CR_WARN && m->timer <= 0) { m->state = CR_DROP; m->vy = 1; }
            else if (m->state == CR_DROP) {
                m->vy = ck_minf(m->vy + 0.7f, 8); m->y += m->vy;
                if (m->y >= m->low) {
                    m->y = m->low; m->state = CR_DOWN; m->timer = 40; lv->shake = 8;
                    ck_sfx(G, SFX_STOMP);
                    ck_puff(G, m->x + 4, m->y + 32, 4, 0xFFF8E6, 230);
                    ck_puff(G, m->x + 28, m->y + 32, 4, 0xFFF8E6, 230);
                }
            } else if (m->state == CR_DOWN && m->timer <= 0) m->state = CR_RISE;
            else if (m->state == CR_RISE) {
                m->y -= 0.8f * sp;
                if (m->y <= m->home) { m->y = m->home; m->state = CR_UP; m->timer = little ? 120 : 80; }
            }
        } else {
            m->ang = 0.75f * sinf(lv->t * 1.3f * sp);
            m->x = m->px + sinf(m->ang) * m->len - m->w / 2;
            m->y = m->py + cosf(m->ang) * m->len;
        }
        m->dx = m->x - ox;
        m->dy = m->y - oy;
    }
}

int ck_level_update(ck_game *G) {
    ck_level *lv = &G->lv;
    lv->t += 1.0f / 60;
    lv->frame++;
    if (lv->nameT > 0) lv->nameT--;
    float maxCam = ck_maxf(0, lv->w * TS - W);
    if (lv->clearT >= 0) {
        lv->clearT++;
        if (lv->clearT % 25 == 0) ck_confetti(G, lv->camX + 160, 0, 8);
        float sx = 0;
        for (int k = 0; k < lv->nplayers; k++) {
            ck_player *p = &lv->players[k];
            p->vy += 0.3f;
            move_body(lv, &p->body);
            sx += p->x;
        }
        if (lv->nplayers) lv->camX += (ck_clampf(sx / lv->nplayers - W / 2 + 6, 0, maxCam) - lv->camX) * 0.12f;
        if (lv->hasResult && lv->result.hasStars)
            for (int k = 0; k < 3; k++) if (lv->clearT == 70 + k * 18 && lv->result.stars[k]) ck_sfx(G, SFX_STAR0 + k);
        if (lv->hasResult && lv->result.record && lv->clearT == 140) ck_sfx(G, SFX_RECORD);
        ck_step_parts(G);
        if (lv->clearT > 330 || (lv->clearT > 90 && (ANYHIT(G, CK_A) || ANYHIT(G, CK_START)))) return LV_FINISH;
        return LV_PLAYING;
    }
    if (lv->dyingT >= 0) {
        lv->dyingT++;
        for (int k = 0; k < lv->nplayers; k++) {
            ck_player *p = &lv->players[k];
            if (p->dead && lv->dyingT > 30) { p->vy += 0.3f; p->y += p->vy; }
        }
        ck_step_parts(G);
        if (lv->dyingT == 150) return G->prog.lives <= 0 ? LV_GAMEOVER : LV_RETRY;
        return LV_PLAYING;
    }
    if (ANYHIT(G, CK_START) && lv->frame > 20) { ck_sfx(G, SFX_SELECT); return LV_PAUSE; }
    lv->timeF++;
    const ck_leveldef *def = lv->def;
    if (def->time && lv->timeF >= def->time * 60) { out_of_time(G); ck_step_parts(G); return LV_PLAYING; }
    if (def->time && lv->timeF >= (def->time - 10) * 60 && lv->timeF % 60 == 0) ck_sfx(G, SFX_TICK);
    update_movers(G);
    int outcome = LV_PLAYING;
    update_players(G, &outcome);
    if (outcome != LV_PLAYING) return outcome;
    // two players: a player left far behind hops over to their buddy
    if (lv->nplayers == 2 && !lv->players[0].bubble && !lv->players[1].bubble) {
        ck_player *a = &lv->players[0], *b = &lv->players[1];
        float dx = a->x - b->x;
        if (ck_absf(dx) > 250) {
            ck_player *lag = dx > 0 ? b : a, *lead = dx > 0 ? a : b;
            ck_puff(G, lag->x + 6, lag->y + 15, 6, 0xFFF8E6, 230);
            lag->x = lead->x - lead->face * 14; lag->y = lead->y - 6; lag->vx = 0; lag->vy = -2; lag->inv = 40;
            ck_sfx(G, SFX_HOP);
            ck_puff(G, lag->x + 6, lag->y + 15, 6, 0xFFF8E6, 230);
        }
    }
    if (lv->doorT >= 0) {
        lv->doorT++;
        if (lv->doorT == 40)
            for (int y = 0; y < lv->h; y++) for (int x = 0; x < lv->w; x++) if (lv->tiles[y][x] == 'D') lv->tiles[y][x] = '.';
    }
    if (lv->hasCrane) update_crane(G);
    if (lv->hasShower) {
        if (lv->showerLeft > 0) {
            lv->showerLeft--;
            if (lv->showerLeft % 9 == 0) {
                ck_ent *b = pickup(G, E_BOLT, 20 + ck_rand(G) * 280, -6);
                b->fly = true; b->vy = 0.5f; b->vx = 0; b->delay = 0; b->rain = true;
            }
        } else if (lv->showerDone < 0) { lv->showerDone = 0; ck_sfx(G, SFX_OPEN); }
        else if (++lv->showerDone > 120) start_clear(G);
    }
    update_ents(G);
    // camera
    float lead = 0;
    for (int k = 0; k < lv->nplayers; k++) lead += lv->players[k].x;
    if (lv->nplayers) lead /= lv->nplayers;
    if (lv->nplayers == 1) lead += lv->players[0].face * 24;
    float target = ck_clampf(lead - W / 2 + 6, 0, maxCam);
    lv->camX += (target - lv->camX) * 0.12f;
    if (lv->shake > 0) lv->shake--;
    if (lv->toastT > 0 && --lv->toastT <= 0) lv->toast[0] = 0;
    ck_step_parts(G);
    return LV_PLAYING;
}

// ---------------------------------------------------------------------------- jetpack side zone
// No gravity: Up or A goes up, Down goes down, letting go hovers. A bump knocks bolts loose.

static void jet_pipes(ck_jet *J, float x, float gy, float gap) {
    if (J->nobs + 2 > 32) return;
    J->obs[J->nobs++] = (ck_obstacle){ x, -10, 26, gy - gap / 2 + 10, 0 };
    J->obs[J->nobs++] = (ck_obstacle){ x, gy + gap / 2, 26, 260, 1 };
}
static void jet_pillar(ck_jet *J, float x, float top) {
    if (J->nobs < 32) J->obs[J->nobs++] = (ck_obstacle){ x, top, 26, 260, 2 };
}
static void jet_bolt(ck_game *G, ck_jet *J, float x, float y) {
    if (J->nitems < 48) J->items[J->nitems++] = (ck_item){ x - 5, y - 5, 10, 10, ck_rand(G) * 6, false };
}

void ck_jet_start(ck_game *G, int hatch) {
    ck_jet *J = &G->jet;
    memset(J, 0, sizeof *J);
    J->hatch = hatch;
    // jet1, JUNK PIPES: the only zone so far
    strcpy(J->name, "JUNK PIPES");
    J->len = 1400; J->speed = 1.15f;
    static const float gapsY[8] = { 120, 90, 150, 110, 70, 140, 100, 160 };
    for (int k = 0; k < 8; k++) {
        float x = 360 + k * 130, gy = gapsY[k];
        if (k % 3 == 2) jet_pillar(J, x, gy + 34);   // a junk pillar from the ground only
        else jet_pipes(J, x, gy, 78);
        for (int j = 0; j < 3; j++) jet_bolt(G, J, x + 10 + j * 14, gy + (k % 3 == 2 ? -10 : 0));
        jet_bolt(G, J, x + 75, (gy + gapsY[(k + 1) % 8]) / 2);
    }
    for (int k = 0; k < G->lv.nplayers; k++) {
        const ck_player *p = &G->lv.players[k];
        J->ps[k].x = 50; J->ps[k].y = 90 + k * 40; J->ps[k].w = 12; J->ps[k].h = p->h - 4;
        J->ps[k].inv = 40; J->ps[k].face = 1;
    }
    J->scroll = -40; J->endT = -1; J->nameT = 110;
    G->nparts = 0;
    ck_play(G, SONG_JET);
}

bool ck_jet_update(ck_game *G) {
    ck_jet *J = &G->jet;
    ck_level *lv = &G->lv;
    J->t += 1.0f / 60;
    if (J->nameT > 0) J->nameT--;
    if (J->endT < 0) lv->timeF++;
    if (J->endT >= 0) { ck_step_parts(G); return ++J->endT > 50; }
    J->scroll += J->speed * (G->prog.little ? 1 : 1.3f);
    for (int k = 0; k < lv->nplayers; k++) {
        int i = lv->players[k].slot;
        __typeof__(J->ps[0]) *p = &J->ps[k];
        bool up = HELD(G, i, CK_UP) || HELD(G, i, CK_A), dn = HELD(G, i, CK_DOWN);
        p->vy += (((dn ? 1 : 0) - (up ? 1 : 0)) * 1.9f - p->vy) * 0.18f;
        p->y = ck_clampf(p->y + p->vy, 14, 224 - p->h);
        int dx = (HELD(G, i, CK_RIGHT) ? 1 : 0) - (HELD(G, i, CK_LEFT) ? 1 : 0);
        if (dx) p->face = 1;
        p->x = ck_clampf(p->x + dx * 1.3f, 16, 220);
        if (p->inv > 0) p->inv--;
        rect wb = { p->x + J->scroll, p->y, p->w, p->h };
        if (p->inv <= 0)
            for (int o = 0; o < J->nobs; o++) {
                const ck_obstacle *ob = &J->obs[o];
                if (!overlap_r(wb, (rect){ ob->x, ob->y, ob->w, ob->h })) continue;
                p->inv = 80; p->vy = p->y > ob->y ? 2.5f : -2.5f;
                ck_sfx(G, SFX_WOBBLE);
                ck_puff(G, p->x + 6 + J->scroll, p->y + 10, 6, 0xFFF8E6, 230);
                int drop = J->bolts < 3 ? J->bolts : 3;
                J->bolts -= drop;
                for (int d = 0; d < drop; d++) ck_sparks(G, p->x + 6 + J->scroll, p->y + 8, 2, 0xFFD23F);
                break;
            }
        for (int it = 0; it < J->nitems; it++) {
            ck_item *b = &J->items[it];
            if (b->dead || !overlap_r(wb, (rect){ b->x, b->y, b->w, b->h })) continue;
            b->dead = true; J->bolts++;
            ck_sfx(G, SFX_BOLT);
            ck_sparks(G, b->x + 5, b->y + 5, 3, 0xFFD23F);
        }
        if (J->fr++ % 3 == 0 && G->nparts < CK_MAXPARTS) {
            ck_part *q = &G->parts[G->nparts++];
            *q = (ck_part){ .x = p->x + J->scroll - 2, .y = p->y + p->h, .vx = -0.6f, .vy = 0.8f, .r = 1.5f, .life = 16, .max = 16, .col = 0xFFC878, .alpha = 230, .kind = PART_PUFF };
        }
    }
    for (int it = 0; it < J->nitems; it++) J->items[it].t += 1.0f / 60;
    if (J->scroll > J->len) { J->endT = 0; ck_sfx(G, SFX_OPEN); }
    ck_step_parts(G);
    return false;
}

void ck_level_come_back(ck_game *G) {
    ck_level *lv = &G->lv;
    const ck_ent *h = &lv->ents[G->jet.hatch];
    lv->bolts += G->jet.bolts;
    ck_play(G, ck_level_song(lv));
    for (int k = 0; k < lv->nplayers; k++) {
        ck_player *p = &lv->players[k];
        p->x = h->outx - p->w / 2 + k * 10; p->y = h->outy - p->h - 2; p->vx = 0; p->vy = -6.5f;
        p->spring = true; p->inv = 60; p->safeX = p->x; p->safeY = h->outy - p->h;
        p->onMover = NULL;
    }
    lv->camX = ck_clampf(h->outx - W / 2, 0, ck_maxf(0, lv->w * TS - W));
    G->nparts = 0;
    ck_puff(G, h->outx, h->outy, 10, 0xFFF8E6, 230);
    ck_sfx(G, SFX_BOING);
}
