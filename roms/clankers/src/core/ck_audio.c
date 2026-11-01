// Sound effects and music as data: the tone() and noise() calls from SND in web/game.js, and
// a sequencer for data/songs.txt. Frontends turn the ck_tone values into sound.
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ck_internal.h"

#define T(f, d, w, v, when, sl) { WAVE_##w, 0, f, d, v, when, sl, false }
#define N(d, v, when, f, filt) { WAVE_NOISE, FILT_##filt, f, d, v, when, 0, false }

typedef struct { int n; ck_tone t[8]; } fx;

static const fx FX[SFX_COUNT] = {
    [SFX_JUMP] = { 1, { T(330, 0.16f, SQUARE, 0.18f, 0, 2.2f) } },
    [SFX_BOOST] = { 2, { N(0.3f, 0.25f, 0, 1800, BAND), T(220, 0.22f, SAW, 0.1f, 0, 2.6f) } },
    [SFX_FLAP] = { 1, { T(520, 0.06f, TRIANGLE, 0.14f, 0, 1.5f) } },
    [SFX_BOLT] = { 2, { T(988, 0.05f, SQUARE, 0.12f, 0, 0), T(1319, 0.09f, SQUARE, 0.12f, 0.05f, 0) } },
    [SFX_GEAR] = { 5, { T(523, 0.14f, SQUARE, 0.16f, 0, 0), T(659, 0.14f, SQUARE, 0.16f, 0.07f, 0), T(784, 0.14f, SQUARE, 0.16f, 0.14f, 0), T(1047, 0.14f, SQUARE, 0.16f, 0.21f, 0), T(1319, 0.14f, SQUARE, 0.16f, 0.28f, 0) } },
    [SFX_REBOOT] = { 3, { T(180, 0.08f, SQUARE, 0.18f, 0, 3), T(880, 0.1f, TRIANGLE, 0.22f, 0.08f, 0), T(1175, 0.14f, TRIANGLE, 0.22f, 0.15f, 0) } },
    [SFX_WOBBLE] = { 2, { T(420, 0.35f, TRIANGLE, 0.25f, 0, 0.35f), T(300, 0.3f, SQUARE, 0.06f, 0.05f, 0.5f) } },
    [SFX_SPRING] = { 1, { T(140, 0.4f, SINE, 0.4f, 0, 4.5f) } },
    [SFX_CRATE] = { 2, { N(0.2f, 0.35f, 0, 500, BAND), T(110, 0.12f, SQUARE, 0.12f, 0, 0.5f) } },
    [SFX_LEVER] = { 2, { T(140, 0.07f, SQUARE, 0.2f, 0, 0), T(210, 0.1f, SQUARE, 0.2f, 0.07f, 0) } },
    [SFX_DOOR] = { 2, { N(0.6f, 0.18f, 0, 220, BAND), T(90, 0.5f, SAW, 0.06f, 0, 0.6f) } },
    [SFX_GRAB] = { 1, { T(700, 0.12f, SAW, 0.08f, 0, 0.45f) } },
    [SFX_KICK] = { 2, { N(0.08f, 0.35f, 0, 1200, BAND), T(170, 0.1f, SQUARE, 0.18f, 0, 0.5f) } },
    [SFX_BOING] = { 1, { T(200, 0.45f, SINE, 0.35f, 0, 3.2f) } },
    [SFX_BUTTON] = { 2, { T(440, 0.09f, SQUARE, 0.2f, 0, 0), T(330, 0.16f, SQUARE, 0.2f, 0.09f, 0) } },
    [SFX_CLANK] = { 2, { N(0.35f, 0.4f, 0, 300, BAND), T(80, 0.3f, SQUARE, 0.15f, 0, 0.7f) } },
    [SFX_SELECT] = { 1, { T(660, 0.05f, SQUARE, 0.13f, 0, 0) } },
    [SFX_CONFIRM] = { 2, { T(523, 0.07f, SQUARE, 0.16f, 0, 0), T(784, 0.12f, SQUARE, 0.16f, 0.07f, 0) } },
    [SFX_OPEN] = { 4, { T(392, 0.14f, TRIANGLE, 0.22f, 0, 0), T(523, 0.14f, TRIANGLE, 0.22f, 0.09f, 0), T(659, 0.14f, TRIANGLE, 0.22f, 0.18f, 0), T(784, 0.14f, TRIANGLE, 0.22f, 0.27f, 0) } },
    [SFX_STEP] = { 1, { T(260, 0.03f, SQUARE, 0.05f, 0, 0) } },
    [SFX_HOP] = { 1, { T(500, 0.08f, SQUARE, 0.12f, 0, 1.8f) } },
    [SFX_NOPE] = { 1, { T(160, 0.15f, SQUARE, 0.12f, 0, 0) } },
    [SFX_SPROUT] = { 5, { T(262, 0.08f, TRIANGLE, 0.18f, 0, 0), T(330, 0.08f, TRIANGLE, 0.18f, 0.05f, 0), T(392, 0.08f, TRIANGLE, 0.18f, 0.1f, 0), T(523, 0.08f, TRIANGLE, 0.18f, 0.15f, 0), T(659, 0.08f, TRIANGLE, 0.18f, 0.2f, 0) } },
    [SFX_POWER] = { 7, { T(523, 0.1f, SQUARE, 0.15f, 0, 0), T(659, 0.1f, SQUARE, 0.15f, 0.06f, 0), T(784, 0.1f, SQUARE, 0.15f, 0.12f, 0), T(1047, 0.1f, SQUARE, 0.15f, 0.18f, 0), T(784, 0.1f, SQUARE, 0.15f, 0.24f, 0), T(1047, 0.1f, SQUARE, 0.15f, 0.3f, 0), T(1319, 0.1f, SQUARE, 0.15f, 0.36f, 0) } },
    [SFX_SHRINK] = { 4, { T(784, 0.1f, SQUARE, 0.15f, 0, 0), T(659, 0.1f, SQUARE, 0.15f, 0.07f, 0), T(523, 0.1f, SQUARE, 0.15f, 0.14f, 0), T(392, 0.1f, SQUARE, 0.15f, 0.21f, 0) } },
    [SFX_BLOW] = { 2, { T(500, 0.15f, SINE, 0.2f, 0, 1.8f), N(0.1f, 0.08f, 0, 2500, BAND) } },
    [SFX_STOMP] = { 2, { N(0.3f, 0.45f, 0, 180, LOW), T(70, 0.25f, SQUARE, 0.15f, 0, 0.6f) } },
    [SFX_HATCH] = { 5, { T(784, 0.09f, SQUARE, 0.15f, 0, 0), T(659, 0.09f, SQUARE, 0.15f, 0.05f, 0), T(523, 0.09f, SQUARE, 0.15f, 0.1f, 0), T(392, 0.09f, SQUARE, 0.15f, 0.15f, 0), T(262, 0.09f, SQUARE, 0.15f, 0.2f, 0) } },
    [SFX_OUCH] = { 5, { T(523, 0.16f, SQUARE, 0.16f, 0, 0), T(440, 0.16f, SQUARE, 0.16f, 0.11f, 0), T(349, 0.16f, SQUARE, 0.16f, 0.22f, 0), T(262, 0.16f, SQUARE, 0.16f, 0.33f, 0), T(196, 0.16f, SQUARE, 0.16f, 0.44f, 0) } },
    [SFX_POP] = { 2, { T(900, 0.06f, SINE, 0.25f, 0, 0.5f), N(0.05f, 0.15f, 0, 3000, BAND) } },
    [SFX_TICK] = { 1, { T(1200, 0.04f, SQUARE, 0.1f, 0, 0) } },
    [SFX_STAR0] = { 2, { T(784, 0.18f, TRIANGLE, 0.26f, 0, 0), T(1568, 0.12f, SQUARE, 0.06f, 0.04f, 0) } },
    [SFX_STAR1] = { 2, { T(988, 0.18f, TRIANGLE, 0.26f, 0, 0), T(1976, 0.12f, SQUARE, 0.06f, 0.04f, 0) } },
    [SFX_STAR2] = { 2, { T(1175, 0.18f, TRIANGLE, 0.26f, 0, 0), T(2349, 0.12f, SQUARE, 0.06f, 0.04f, 0) } },
    [SFX_RECORD] = { 7, { T(523, 0.11f, SQUARE, 0.13f, 0, 0), T(659, 0.11f, SQUARE, 0.13f, 0.05f, 0), T(784, 0.11f, SQUARE, 0.13f, 0.1f, 0), T(1047, 0.11f, SQUARE, 0.13f, 0.15f, 0), T(1319, 0.11f, SQUARE, 0.13f, 0.2f, 0), T(1568, 0.11f, SQUARE, 0.13f, 0.25f, 0), T(2093, 0.11f, SQUARE, 0.13f, 0.3f, 0) } },
    [SFX_FLAG] = { 5, { T(392, 0.12f, TRIANGLE, 0.22f, 0, 0), T(523, 0.12f, TRIANGLE, 0.22f, 0.06f, 0), T(659, 0.12f, TRIANGLE, 0.22f, 0.12f, 0), T(784, 0.12f, TRIANGLE, 0.22f, 0.18f, 0), T(1047, 0.12f, TRIANGLE, 0.22f, 0.24f, 0) } },
    [SFX_SLIDE] = { 2, { N(0.35f, 0.2f, 0, 900, LOW), T(300, 0.25f, TRIANGLE, 0.1f, 0, 0.6f) } },
    [SFX_THUNDER] = { 2, { N(1.1f, 0.5f, 0, 160, LOW), N(0.2f, 0.3f, 0, 2000, BAND) } },
    [SFX_WHOOSH] = { 2, { N(1.4f, 0.35f, 0, 600, BAND), T(200, 1.2f, SINE, 0.08f, 0, 3) } },
};

int ck_sfx_tones(int id, ck_tone *out, int max) {
    if (id < 0 || id >= SFX_COUNT) return 0;
    int n = FX[id].n < max ? FX[id].n : max;
    memcpy(out, FX[id].t, sizeof(ck_tone) * (size_t)n);
    return n;
}

// ---------------------------------------------------------------------------- songs

static float hz(const char *s, size_t n) {
    static const int NOTE[7] = { 9, 11, 0, 2, 4, 5, 7 };   // A..G
    if (n < 2 || s[0] < 'A' || s[0] > 'G') return 0;
    int semi = NOTE[s[0] - 'A'];
    size_t i = 1;
    if (s[i] == '#') { semi++; i++; }
    if (i >= n || !isdigit((unsigned char)s[i])) return 0;
    int oct = s[i] - '0';
    int midi = semi + (oct + 1) * 12;
    return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
}

static int wave_by_name(const char *s, size_t n, bool *drum) {
    *drum = false;
    if (n == 6 && !strncmp(s, "square", 6)) return WAVE_SQUARE;
    if (n == 8 && !strncmp(s, "triangle", 8)) return WAVE_TRIANGLE;
    if (n == 8 && !strncmp(s, "sawtooth", 8)) return WAVE_SAW;
    if (n == 4 && !strncmp(s, "sine", 4)) return WAVE_SINE;
    if (n == 4 && !strncmp(s, "drum", 4)) { *drum = true; return WAVE_NOISE; }
    return -1;
}

int ck_parse_songs(const char *text, ck_song *songs) {
    memset(songs, 0, sizeof(ck_song) * SONG_COUNT);
    ck_song *cur = NULL;
    int count = 0;
    const char *p = text;
    while (*p) {
        const char *e = p;
        while (*e && *e != '\n') e++;
        const char *next = *e ? e + 1 : e;
        // words of this line
        const char *w[CK_MAXNOTES * 2 + 4];
        size_t wl[CK_MAXNOTES * 2 + 4];
        int nw = 0;
        for (const char *q = p; q < e && nw < (int)(sizeof w / sizeof *w);) {
            while (q < e && isspace((unsigned char)*q)) q++;
            if (q >= e) break;
            const char *s = q;
            while (q < e && !isspace((unsigned char)*q)) q++;
            if (!(q - s == 1 && *s == '|')) { w[nw] = s; wl[nw] = (size_t)(q - s); nw++; }
        }
        p = next;
        if (!nw || w[0][0] == '#') continue;
        if (wl[0] == 4 && !strncmp(w[0], "song", 4) && nw >= 2) {
            char name[16];
            size_t n = wl[1] < 15 ? wl[1] : 15;
            memcpy(name, w[1], n); name[n] = 0;
            int id = ck_song_by_name(name);
            cur = id ? &songs[id] : NULL;
            if (!cur) continue;
            memset(cur, 0, sizeof *cur);
            cur->valid = true; cur->bpm = 120;
            count++;
            for (int k = 2; k < nw; k++) {
                if (wl[k] > 4 && !strncmp(w[k], "bpm=", 4)) cur->bpm = (float)atof(w[k] + 4);
                else if (wl[k] == 4 && !strncmp(w[k], "loop", 4)) cur->loop = true;
            }
            continue;
        }
        bool drum;
        int wave = wave_by_name(w[0], wl[0], &drum);
        if (!cur || wave < 0 || nw < 2 || cur->ntracks >= CK_MAXTRACKS) continue;
        ck_track *t = &cur->tracks[cur->ntracks++];
        t->wave = (uint8_t)wave; t->drum = drum; t->vol = (float)atof(w[1]);
        for (int k = 2; k < nw && t->n < CK_MAXNOTES; k++) {
            const char *s = w[k];
            size_t n = wl[k], dashes = 0;
            while (n && s[n - 1] == '-') { n--; dashes++; }
            float f = 0;
            if (drum) f = n && s[0] == 'k' ? 1 : n && s[0] == 's' ? 2 : n && s[0] == 'h' ? 3 : 0;
            else if (!(n == 1 && s[0] == '.')) f = hz(s, n);
            t->freq[t->n] = f;
            t->len[t->n] = (uint8_t)(1 + dashes);
            t->n++;
        }
    }
    return count;
}

void ck_music_start(ck_music *m, const ck_song *songs, int id) {
    memset(m, 0, sizeof *m);
    m->id = id;
    m->song = id > 0 && id < SONG_COUNT && songs[id].valid ? &songs[id] : NULL;
    for (int k = 0; k < CK_MAXTRACKS; k++) m->next[k] = 0.06f;
    m->done = !m->song;
}

void ck_music_advance(ck_music *m, float dt, void (*emit)(const ck_tone *t, void *u), void *u) {
    if (m->done || !m->song) return;
    const ck_song *s = m->song;
    float spb = 60.0f / s->bpm / 2;   // seconds per eighth
    bool allDone = true;
    for (int k = 0; k < s->ntracks; k++) {
        const ck_track *tr = &s->tracks[k];
        while (m->next[k] < dt) {
            if (m->idx[k] >= tr->n) { if (!s->loop || !tr->n) break; m->idx[k] = 0; }
            int i = m->idx[k]++;
            float f = tr->freq[i], dur = tr->len[i] * spb, when = m->next[k];
            ck_tone t;
            memset(&t, 0, sizeof t);
            t.when = when; t.music = true;
            if (tr->drum) {
                if (f == 1) { t.wave = WAVE_SINE; t.freq = 120; t.dur = 0.12f; t.vol = tr->vol * 2.2f; t.slide = 0.35f; emit(&t, u); }
                else if (f == 2) { t.wave = WAVE_NOISE; t.filt = FILT_BAND; t.freq = 1800; t.dur = 0.1f; t.vol = tr->vol * 1.2f; emit(&t, u); }
                else if (f == 3) { t.wave = WAVE_NOISE; t.filt = FILT_HIGH; t.freq = 7000; t.dur = 0.04f; t.vol = tr->vol * 0.6f; emit(&t, u); }
            } else if (f > 0) {
                t.wave = tr->wave; t.freq = f; t.dur = dur * 0.92f; t.vol = tr->vol;
                emit(&t, u);
            }
            m->next[k] += dur;
        }
        m->next[k] -= dt;
        if (m->idx[k] < tr->n) allDone = false;
    }
    if (!s->loop && allDone) m->done = true;
}
