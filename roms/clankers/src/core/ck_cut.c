// The little films between worlds. Mirrors cutScene() and cutCast() in web/game.js: the same
// timings, captions and sounds, so the N64 and the browser show the same film.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ck_internal.h"

const float ck_cut_len[CK_WORLDS + 1] = { 0, 12.6f, 12.6f, 12.6f, 16.5f };

typedef struct { float at; const char *text; } cap;
static const cap CAPS[CK_WORLDS + 1][5] = {
    { { 0, NULL } },
    { { 0, "Junkyard Hills is fixed!" }, { 4, "More baby bots blew into the Gear Factory..." }, { 8, "Let's go and find them!" }, { -1, NULL } },
    { { 0, "The Gear Factory is humming again!" }, { 4, "Next stop: Magnet Mountain!" }, { 8, "Going up, up, up!" }, { -1, NULL } },
    { { 0, "Magnet Mountain is fixed!" }, { 4.5f, "3..." }, { 5.5f, "2..." }, { 6.5f, "1..." }, { 7.5f, "Blast off to the Moon!" } },
    { { 0, "Every baby bot is safe!" }, { 5, "The Big Glitch is fixed for good!" }, { 10, "Home together!  THE END" }, { -1, NULL } },
};

static float smooth(float u) { u = ck_clampf(u, 0, 1); return u * u * (3 - 2 * u); }
static float fade_door(float x) { return ck_clampf(1 - (x - 244) / 24, 0, 1); }
static float fade_pad(float x) { return ck_clampf((CK_CUT_PAD_X - x) / 14, 0, 1); }

static float cage_y(float t) {
    return t < 3.5f ? CK_CUT_GROUND : CK_CUT_GROUND - (CK_CUT_GROUND - CK_CUT_SHAFT_TOP) * smooth((t - 3.5f) / 6);
}

void ck_cut_cast(const ck_game *G, ck_cut_view *v) {
    memset(v, 0, sizeof *v);
    int k = G->cut < 1 ? 1 : G->cut > CK_WORLDS ? CK_WORLDS : G->cut;
    float t = G->t, len = ck_cut_len[k];
    int np = G->prog.joined[1] ? 2 : 1, nb = G->ncutBabies;
    v->nheroes = np;
    v->nbabies = nb;
    const float GR = CK_CUT_GROUND;
    if (k == 1) {
        float lx = t < 0.8f ? -20 : t < 7.5f ? -20 + 30 * (t - 0.8f) : 181 + 34 * (t - 7.5f);
        bool walk = t > 0.8f && t < 10.6f;
        for (int i = 0; i < np; i++) { float x = lx - i * 18; v->heroes[i] = (ck_cut_actor){ x, GR, fade_door(x), walk }; }
        for (int j = 0; j < nb; j++) { float x = lx - np * 18 - 6 - j * 14; v->babies[j] = (ck_cut_actor){ x, GR, fade_door(x), walk }; }
        v->gate = ck_clampf((t - 5.5f) / 1.5f, 0, 1) - ck_clampf((t - 10.4f) / 1, 0, 1);
    } else if (k == 2) {
        float lx = t < 0.5f ? -20 : -20 + 90 * (t - 0.5f), cy = cage_y(t);
        float out = t < 9.8f ? 0 : ck_minf(102, 60 * (t - 9.8f));
        for (int i = 0; i < np; i++) {
            float x = ck_minf(lx - i * 16, 176 - i * 14);
            v->heroes[i] = (ck_cut_actor){ x + out, cy, 1, (x < 176 - i * 14 && t > 0.5f) || (out > 0 && out < 102) };
        }
        for (int j = 0; j < nb; j++) {
            float x = ck_minf(lx - np * 16 - 4 - j * 12, 152 - j * 7);
            v->babies[j] = (ck_cut_actor){ x + out, cy, 1, x < 152 - j * 7 && t > 0.5f };
        }
        v->cageY = cy;
    } else if (k == 3) {
        float lx = t < 0.5f ? -20 : -20 + 60 * (t - 0.5f), X = CK_CUT_PAD_X;
        for (int i = 0; i < np; i++) { float x = ck_minf(lx - i * 16, X); v->heroes[i] = (ck_cut_actor){ x, GR, fade_pad(x), x < X }; }
        for (int j = 0; j < nb; j++) { float x = ck_minf(lx - np * 16 - 6 - j * 12, X); v->babies[j] = (ck_cut_actor){ x, GR, fade_pad(x), x < X }; }
        v->rocketY = t < 7.5f ? GR : GR - 45 * (t - 7.5f) * (t - 7.5f);
        v->shake = t > 4.5f && t < 7.5f ? sinf(t * 50) : 0;
        v->flame = t > 6.8f;
        v->bigFlame = t >= 7.5f;
    } else {
        float u = 1 - ck_clampf(t / 5, 0, 1), fx = 200 + 220 * u * u;
        for (int i = 0; i < np; i++) v->heroes[i] = (ck_cut_actor){ fx - 12 * (np - 1) + i * 24, 180, 1, false };
        for (int j = 0; j < nb; j++) v->babies[j] = (ck_cut_actor){ fx - 60 + j * 8, 180, 1, false };
        v->floatX = fx;
    }
    int ci = 0;
    for (int i = 0; i < 5 && CAPS[k][i].text; i++) if (t >= CAPS[k][i].at) ci = i;
    v->caption = CAPS[k][ci].text;
    v->captionA = ck_clampf((t - CAPS[k][ci].at) * 3, 0, 1);
    v->gold = k == 4 && ci == 2;
    v->black = t < 0.6f ? 1 - t / 0.6f : 0;
    v->white = t > len - 0.8f ? ck_clampf((t - (len - 0.8f)) / 0.8f, 0, 1) : 0;
}

// Which baby bots ride along: the world's own four, or (the parade) all sixteen.
void ck_cut_start(ck_game *G, int k) {
    G->cut = k;
    G->ncutBabies = 0;
    if (k < CK_WORLDS || !G->load) {
        memcpy(G->cutBabies, G->world.babies, sizeof G->world.babies);
        G->ncutBabies = 4;
    } else {
        static ck_world w;   // too big for a small stack
        for (int n = 1; n <= CK_WORLDS; n++) {
            char path[32];
            snprintf(path, sizeof path, "w%d/world.txt", n);
            char *text = G->load(path, G->loadUser);
            if (text && ck_parse_world(text, &w)) memcpy(G->cutBabies + G->ncutBabies, w.babies, sizeof w.babies);
            else memcpy(G->cutBabies + G->ncutBabies, G->world.babies, sizeof G->world.babies);
            G->ncutBabies += 4;
            free(text);
        }
    }
    G->scene = SC_CUT;
    G->t = 0;
    G->nparts = 0;
    ck_play(G, k == CK_WORLDS ? SONG_PARADE : SONG_TRAVEL);
}

void ck_game_film(ck_game *G, int k) {
    if (k < 1 || k > CK_WORLDS) return;
    ck_cut_start(G, k);
}

// One tick of the film. True when it is over (or skipped).
bool ck_cut_update(ck_game *G) {
    int k = G->cut;
    float t = G->t, was = t - 1.0f / 60;
#define AT(w) (was < (w) && t >= (w))
    if (k == 1 && (AT(5.5f) || AT(10.4f))) ck_sfx(G, SFX_DOOR);
    if (k == 2 && (AT(3.5f) || AT(9.5f))) ck_sfx(G, SFX_CLANK);
    if (k == 3) {
        if (AT(4.5f) || AT(5.5f) || AT(6.5f)) ck_sfx(G, SFX_TICK);
        if (AT(7.5f)) { ck_sfx(G, SFX_BOOST); ck_sfx(G, SFX_WHOOSH); }
        if (t > 6 && t < 9 && (int)(t * 60) % 4 == 0) ck_puff(G, CK_CUT_PAD_X + (ck_rand(G) - 0.5f) * 30, CK_CUT_GROUND - 4, 2, 0xF0F0FA, 230);
    }
    if (k == 4 && (int)(t * 60) % 24 == 0) ck_confetti(G, 160, 0, 10);
#undef AT
    ck_step_parts(G);
    return t >= ck_cut_len[k] || (t > 1 && (ANYHIT(G, CK_START) || ANYHIT(G, CK_A)));
}
