// Reading the text formats in roms/clankers/data (see data/README.md).
// Mirrors web/lib/data.js, so the browser and the N64 read the files the same way.
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ck.h"

static const char KNOWN[] = ".#=-X?$1234DLoGbsgTvrk~|CHh@FP";

static void copy(char *dst, size_t n, const char *src, size_t len) {
    if (len >= n) len = n - 1;
    memcpy(dst, src, len);
    dst[len] = 0;
}

// Next line of text: sets *len (without \r\n) and returns the start of the following line.
static const char *line(const char *s, size_t *len) {
    const char *e = s;
    while (*e && *e != '\n') e++;
    size_t n = (size_t)(e - s);
    while (n && (s[n - 1] == '\r')) n--;
    *len = n;
    return *e ? e + 1 : e;
}

static bool is_word(char c) { return isalnum((unsigned char)c) || c == '_' || c == '-'; }

// "key: value" -> key and value (trimmed). False if the line is not one.
static bool keyval(const char *s, size_t n, char *key, size_t kn, char *val, size_t vn) {
    size_t i = 0;
    while (i < n && isspace((unsigned char)s[i])) i++;
    size_t k0 = i;
    while (i < n && is_word(s[i])) i++;
    if (i == k0) return false;
    size_t k1 = i;
    while (i < n && isspace((unsigned char)s[i])) i++;
    if (i >= n || s[i] != ':') return false;
    i++;
    while (i < n && isspace((unsigned char)s[i])) i++;
    size_t v1 = n;
    while (v1 > i && isspace((unsigned char)s[v1 - 1])) v1--;
    copy(key, kn, s + k0, k1 - k0);
    copy(val, vn, s + i, v1 - i);
    return true;
}

static const char *const SONG_NAMES[SONG_COUNT] = {
    "", "title", "select", "map", "level", "level2", "level3", "boss", "bonus", "rainbow",
    "jet", "rescue", "clear", "lullaby", "storm",
};

int ck_song_by_name(const char *name) {
    for (int i = 1; i < SONG_COUNT; i++)
        if (!strcmp(name, SONG_NAMES[i])) return i;
    return SONG_NONE;
}

bool ck_parse_level(const char *text, ck_leveldef *L) {
    memset(L, 0, sizeof *L);
    L->baby = 0xFF8FC7;
    const char *s = text;
    size_t n;
    char key[16], val[64];
    // header, up to the "---" line
    bool sawSep = false;
    const char *p = s;
    while (*p) {
        const char *next = line(p, &n);
        size_t a = 0, b = n;
        while (a < b && isspace((unsigned char)p[a])) a++;
        while (b > a && isspace((unsigned char)p[b - 1])) b--;
        if (b - a == 3 && !strncmp(p + a, "---", 3)) { p = next; sawSep = true; break; }
        if (keyval(p, n, key, sizeof key, val, sizeof val)) {
            if (!strcmp(key, "name")) copy(L->name, sizeof L->name, val, strlen(val));
            else if (!strcmp(key, "kind")) L->kind = !strcmp(val, "boss") ? CK_KIND_BOSS : !strcmp(val, "bonus") ? CK_KIND_BONUS : CK_KIND_LEVEL;
            else if (!strcmp(key, "song")) L->song = ck_song_by_name(val);
            else if (!strcmp(key, "baby")) L->baby = (uint32_t)strtoul(val, NULL, 16);
            else if (!strcmp(key, "hatch")) copy(L->hatch, sizeof L->hatch, val, strlen(val));
            else if (!strcmp(key, "par")) L->par = atoi(val);
            else if (!strcmp(key, "time")) L->time = atoi(val);
            else if (!strcmp(key, "boss")) copy(L->boss, sizeof L->boss, val, strlen(val));
        }
        p = next;
    }
    if (!sawSep) p = s;   // no header at all: the whole file is rows, like data.js
    // rows: non-empty lines, the first 15
    const char *rows[CK_ROWS];
    size_t lens[CK_ROWS];
    int nr = 0;
    size_t w = 20;
    while (*p && nr < CK_ROWS) {
        const char *next = line(p, &n);
        if (n) { rows[nr] = p; lens[nr] = n; if (n > w) w = n; nr++; }
        p = next;
    }
    if (!nr) return false;
    if (w > CK_MAXW) w = CK_MAXW;
    L->w = (int)w;
    int pad = CK_ROWS - nr;   // short maps sit at the bottom
    for (int y = 0; y < CK_ROWS; y++) {
        for (size_t x = 0; x < w; x++) {
            char c = '.';
            int r = y - pad;
            if (r >= 0 && x < lens[r]) c = rows[r][x];
            if (!c || !strchr(KNOWN, c)) c = '.';
            L->rows[y][x] = c;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------- world files

typedef struct { const char *s; size_t n; bool quoted; } tok;

static int tokens(const char *s, size_t n, tok *out, int max) {
    int k = 0;
    size_t i = 0;
    while (i < n && k < max) {
        while (i < n && isspace((unsigned char)s[i])) i++;
        if (i >= n) break;
        if (s[i] == '"') {
            size_t j = i + 1;
            while (j < n && s[j] != '"') j++;
            out[k++] = (tok){ s + i + 1, j - i - 1, true };
            i = j + 1;
        } else {
            size_t j = i;
            while (j < n && !isspace((unsigned char)s[j])) j++;
            out[k++] = (tok){ s + i, j - i, false };
            i = j;
        }
    }
    return k;
}

static bool tok_is(tok t, const char *w) { return t.n == strlen(w) && !strncmp(t.s, w, t.n); }
static int tok_int(tok t) { char b[16]; copy(b, sizeof b, t.s, t.n); return atoi(b); }
// key=value token: returns the value part or NULL
static const char *opt(tok t, const char *key, size_t *vn) {
    size_t kl = strlen(key);
    if (t.quoted || t.n <= kl || strncmp(t.s, key, kl) || t.s[kl] != '=') return NULL;
    *vn = t.n - kl - 1;
    return t.s + kl + 1;
}

int ck_world_node(const ck_world *w, const char *id) {
    for (int i = 0; i < w->nnodes; i++)
        if (!strcmp(w->nodes[i].id, id)) return i;
    return -1;
}

static int node_kind(tok t) {
    if (tok_is(t, "home")) return CK_NODE_HOME;
    if (tok_is(t, "boss")) return CK_NODE_BOSS;
    if (tok_is(t, "shop")) return CK_NODE_SHOP;
    if (tok_is(t, "bonus")) return CK_NODE_BONUS;
    if (tok_is(t, "exit")) return CK_NODE_EXIT;
    return CK_NODE_LEVEL;
}

bool ck_parse_world(const char *text, ck_world *W) {
    memset(W, 0, sizeof *W);
    W->num = 1;
    W->gears = 9;
    const uint32_t babies[4] = { 0xFF8FC7, 0x49F08A, 0xFFB347, 0x8FD3FF };
    memcpy(W->babies, babies, sizeof babies);
    strcpy(W->name, "New World");
    // roads name nodes by id; resolve after every node is read
    struct { char a[12], b[12]; } pend[CK_MAXROADS];
    const char *p = text;
    size_t n;
    while (*p) {
        const char *next = line(p, &n);
        // drop "  # comment" (a # after whitespace) and whole-line comments
        size_t e = n;
        for (size_t i = 1; i < n; i++)
            if (p[i] == '#' && isspace((unsigned char)p[i - 1])) { e = i; break; }
        size_t a = 0;
        while (a < e && isspace((unsigned char)p[a])) a++;
        while (e > a && isspace((unsigned char)p[e - 1])) e--;
        const char *s = p + a;
        size_t len = e - a;
        p = next;
        if (!len || s[0] == '#') continue;
        char key[16], val[64];
        if (keyval(s, len, key, sizeof key, val, sizeof val)) {
            if (!strcmp(key, "name")) copy(W->name, sizeof W->name, val, strlen(val));
            else if (!strcmp(key, "num")) W->num = atoi(val);
            else if (!strcmp(key, "theme")) copy(W->theme, sizeof W->theme, val, strlen(val));
            else if (!strcmp(key, "gears")) W->gears = atoi(val);
            else if (!strcmp(key, "boss-msg")) copy(W->bossmsg, sizeof W->bossmsg, val, strlen(val));
            else if (!strcmp(key, "babies")) {
                char *q = val;
                for (int k = 0; k < 4 && *q; k++) {
                    char *end;
                    W->babies[k] = (uint32_t)strtoul(q, &end, 16);
                    if (end == q) break;
                    q = end;
                }
            }
            continue;
        }
        tok t[16];
        int nt = tokens(s, len, t, 16);
        if (!nt) continue;
        if (tok_is(t[0], "node") && W->nnodes < CK_MAXNODES) {
            ck_node *N = &W->nodes[W->nnodes];
            memset(N, 0, sizeof *N);
            int plain = 0;
            for (int k = 1; k < nt; k++) {
                size_t vn;
                const char *v;
                if ((v = opt(t[k], "level", &vn))) copy(N->level, sizeof N->level, v, vn);
                else if ((v = opt(t[k], "gate", &vn))) { char b[8]; copy(b, sizeof b, v, vn); N->gate = atoi(b); }
                else {
                    if (plain == 0) copy(N->id, sizeof N->id, t[k].s, t[k].n);
                    else if (plain == 1) N->col = tok_int(t[k]);
                    else if (plain == 2) N->row = tok_int(t[k]);
                    else if (plain == 3) N->kind = node_kind(t[k]);
                    else if (plain == 4) copy(N->label, sizeof N->label, t[k].s, t[k].n);
                    plain++;
                }
            }
            if (!N->label[0]) strcpy(N->label, N->id);
            N->x = N->col * CK_TS + 8;
            N->y = N->row * CK_TS + 8;
            W->nnodes++;
        } else if (tok_is(t[0], "road") && W->nroads < CK_MAXROADS) {
            ck_road *R = &W->roads[W->nroads];
            memset(R, 0, sizeof *R);
            int plain = 0, nv = 0;
            for (int k = 1; k < nt; k++) {
                size_t vn;
                const char *v;
                if ((v = opt(t[k], "via", &vn))) {
                    char b[16]; copy(b, sizeof b, v, vn);
                    char *c = strchr(b, ',');
                    if (c && nv < CK_MAXVIA) {
                        *c = 0;
                        R->pts[1 + nv][0] = (float)(atoi(b) * CK_TS + 8);
                        R->pts[1 + nv][1] = (float)(atoi(c + 1) * CK_TS + 8);
                        nv++;
                    }
                } else if ((v = opt(t[k], "gate", &vn))) { char b[8]; copy(b, sizeof b, v, vn); R->gate = atoi(b); }
                else {
                    if (plain == 0) copy(pend[W->nroads].a, 12, t[k].s, t[k].n);
                    else if (plain == 1) copy(pend[W->nroads].b, 12, t[k].s, t[k].n);
                    plain++;
                }
            }
            R->npts = nv + 2;
            W->nroads++;
        }
        // feature and prop lines are scenery: the baked map picture already has them
    }
    // count level numbers, resolve roads, measure them
    int num = 0;
    for (int i = 0; i < W->nnodes; i++)
        if (W->nodes[i].kind == CK_NODE_LEVEL) W->nodes[i].num = ++num;
    int out = 0;
    for (int i = 0; i < W->nroads; i++) {
        ck_road R = W->roads[i];
        R.a = ck_world_node(W, pend[i].a);
        R.b = ck_world_node(W, pend[i].b);
        if (R.a < 0 || R.b < 0) continue;
        R.pts[0][0] = (float)W->nodes[R.a].x; R.pts[0][1] = (float)W->nodes[R.a].y;
        R.pts[R.npts - 1][0] = (float)W->nodes[R.b].x; R.pts[R.npts - 1][1] = (float)W->nodes[R.b].y;
        R.lens[0] = 0;
        for (int k = 1; k < R.npts; k++)
            R.lens[k] = R.lens[k - 1] + hypotf(R.pts[k][0] - R.pts[k - 1][0], R.pts[k][1] - R.pts[k - 1][1]);
        R.total = R.lens[R.npts - 1];
        W->roads[out++] = R;
    }
    W->nroads = out;
    return W->nnodes > 0;
}

void ck_road_point(const ck_road *r, float u, float *x, float *y) {
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    float d = u * r->total;
    int i = 1;
    while (i < r->npts - 1 && r->lens[i] < d) i++;
    float seg = r->lens[i] - r->lens[i - 1];
    float k = (d - r->lens[i - 1]) / (seg ? seg : 1);
    *x = r->pts[i - 1][0] + (r->pts[i][0] - r->pts[i - 1][0]) * k;
    *y = r->pts[i - 1][1] + (r->pts[i][1] - r->pts[i - 1][1]) * k;
}
