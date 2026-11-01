// A tiny software synth for the N64: square, triangle, saw and sine voices plus filtered noise,
// with the same envelope and pitch slide as the browser's Web Audio tones. It plays the
// ck_tone values the core hands out (sound effects and the songs in data/songs.txt) through
// one libdragon mixer channel.
#include <libdragon.h>
#include <math.h>
#include <string.h>

#include "ck.h"
#include "synth.h"

#define RATE    22050
#define VOICES  24
#define CHUNK   64

typedef struct {
    bool on, music;
    uint8_t wave, filt;
    float phase, freq, fmul;
    int delay, len, t, att;
    float vol, env, dec;
    float a1, a2, y1, y2;       // noise filters (one-pole low-passes)
} voice_t;

static voice_t V[VOICES];
static ck_song songs[SONG_COUNT];
static ck_music music;
static int cur_song = -1;
static uint32_t lfsr = 0x12345u;
static float sine[256];
static waveform_t wave;
static bool muted;

static float onepole(float fc) {
    if (fc > RATE * 0.45f) fc = RATE * 0.45f;
    return 1.0f - expf(-2.0f * 3.14159265f * fc / RATE);
}

static void start_tone(const ck_tone *t, int extra_delay) {
    if (t->dur <= 0 || t->vol <= 0) return;
    voice_t *v = NULL;
    for (int i = 0; i < VOICES; i++) if (!V[i].on) { v = &V[i]; break; }
    if (!v) {   // steal the quietest
        v = &V[0];
        for (int i = 1; i < VOICES; i++) if (V[i].env < v->env) v = &V[i];
    }
    memset(v, 0, sizeof *v);
    v->on = true;
    v->music = t->music;
    v->wave = t->wave;
    v->filt = t->filt;
    v->freq = t->freq;
    v->delay = extra_delay + (int)(t->when * RATE);
    v->len = (int)(t->dur * RATE);
    if (v->len < 8) v->len = 8;
    v->att = (int)(0.008f * RATE);
    if (v->att >= v->len) v->att = v->len / 2;
    v->vol = t->vol;
    v->dec = powf(0.0001f / t->vol, 1.0f / (float)(v->len - v->att));
    v->fmul = t->slide > 0 ? powf(t->slide < 0.03f ? 0.03f : t->slide, 1.0f / (float)v->len) : 1.0f;
    if (t->wave == WAVE_NOISE) {
        if (t->filt == FILT_BAND) { v->a1 = onepole(t->freq * 1.5f); v->a2 = onepole(t->freq / 1.5f); }
        else v->a1 = onepole(t->freq);
    }
}

static void emit_music(const ck_tone *t, void *u) { start_tone(t, *(int *)u); }

static void render(int16_t *out, int n) {
    for (int base = 0; base < n; base += CHUNK) {
        int len = n - base < CHUNK ? n - base : CHUNK;
        int at = 0;
        ck_music_advance(&music, (float)len / RATE, emit_music, &at);
        for (int s = 0; s < len; s++) {
            float mus = 0, sfx = 0;
            for (int i = 0; i < VOICES; i++) {
                voice_t *v = &V[i];
                if (!v->on) continue;
                if (v->delay > 0) { v->delay--; continue; }
                float x;
                switch (v->wave) {
                case WAVE_SQUARE: x = v->phase < 0.5f ? 1.0f : -1.0f; break;
                case WAVE_TRIANGLE: x = 4.0f * fabsf(v->phase - 0.5f) - 1.0f; break;
                case WAVE_SAW: x = 2.0f * v->phase - 1.0f; break;
                case WAVE_SINE: x = sine[(int)(v->phase * 256) & 255]; break;
                default: {
                    lfsr ^= lfsr << 13; lfsr ^= lfsr >> 17; lfsr ^= lfsr << 5;
                    float w = (float)(int32_t)lfsr / 2147483648.0f;
                    v->y1 += v->a1 * (w - v->y1);
                    if (v->filt == FILT_BAND) { v->y2 += v->a2 * (w - v->y2); x = (v->y1 - v->y2) * 2.0f; }
                    else if (v->filt == FILT_HIGH) x = w - v->y1;
                    else x = v->y1 * 1.5f;
                }
                }
                if (v->wave != WAVE_NOISE) {
                    v->phase += v->freq / RATE;
                    if (v->phase >= 1.0f) v->phase -= (float)(int)v->phase;
                    v->freq *= v->fmul;
                }
                if (v->t < v->att) v->env = v->vol * (float)v->t / (float)v->att;
                else v->env = (v->t == v->att ? v->vol : v->env * v->dec);
                if (v->music) mus += x * v->env; else sfx += x * v->env;
                if (++v->t >= v->len) v->on = false;
            }
            float y = (mus * 0.32f + sfx * 0.7f) * 0.55f;
            if (muted) y = 0;
            int q = (int)(y * 26000.0f);
            out[base + s] = (int16_t)(q > 32767 ? 32767 : q < -32768 ? -32768 : q);
        }
    }
}

static void wave_read(void *ctx, samplebuffer_t *sbuf, int wpos, int wlen, bool seeking) {
    (void)ctx; (void)wpos; (void)seeking;
    int16_t *out = samplebuffer_append(sbuf, wlen);
    render(out, wlen);
}

void synth_init(const char *songs_text) {
    for (int i = 0; i < 256; i++) sine[i] = sinf(i * 2.0f * 3.14159265f / 256.0f);
    if (songs_text) ck_parse_songs(songs_text, songs);
    audio_init(RATE, 4);
    mixer_init(1);
    wave = (waveform_t){
        .name = "synth", .bits = 16, .channels = 1, .frequency = RATE,
        .len = WAVEFORM_UNKNOWN_LEN, .loop_len = 0, .read = wave_read, .ctx = NULL,
    };
    mixer_ch_play(0, &wave);
}

void synth_sfx(int id) {
    ck_tone t[8];
    int n = ck_sfx_tones(id, t, 8);
    for (int i = 0; i < n; i++) start_tone(&t[i], 0);
}

void synth_song(int id) {
    if (id == cur_song) return;
    cur_song = id;
    for (int i = 0; i < VOICES; i++) if (V[i].music) V[i].on = false;
    ck_music_start(&music, songs, id);
}

void synth_poll(void) { mixer_try_play(); }

void synth_mute(bool m) { muted = m; }
