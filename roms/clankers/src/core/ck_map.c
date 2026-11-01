// The world map: nodes joined by roads that open as you beat levels and find golden gears.
// A port of OW.core() in web/lib/ow.js (the rules; the picture is drawn by each frontend).
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ck_internal.h"

static bool is_play(int kind) { return kind == CK_NODE_LEVEL || kind == CK_NODE_BOSS || kind == CK_NODE_BONUS; }

bool ck_node_beaten(const ck_game *G, int node) { return (G->map.beaten >> node) & 1; }
bool ck_road_open(const ck_game *G, int road) { return G->map.open[road]; }

static void open_from(ck_game *G, int id, bool instant) {
    ck_map *M = &G->map;
    const ck_world *W = &G->world;
    for (int r = 0; r < W->nroads; r++) {
        const ck_road *e = &W->roads[r];
        if (e->a != id || M->open[r]) continue;
        if (e->gate && M->gears < e->gate) continue;
        int k = W->nodes[id].kind;
        if (k != CK_NODE_HOME && k != CK_NODE_SHOP && !ck_node_beaten(G, id)) continue;
        M->open[r] = true;
        if (!instant && M->nreveal < CK_MAXROADS) {
            float dur = e->total / 70;
            M->reveal[M->nreveal].road = r;
            M->reveal[M->nreveal].t0 = M->now;
            M->reveal[M->nreveal].dur = dur < 0.5f ? 0.5f : dur;
            M->nreveal++;
        }
    }
}

void ck_map_reset(ck_game *G) {
    ck_map *M = &G->map;
    float now = M->now;
    memset(M, 0, sizeof *M);
    M->now = now;
    M->beaten = G->prog.beaten;
    M->at = G->prog.at < G->world.nnodes ? G->prog.at : 0;
    M->gears = G->prog.gears;
    M->bolts = G->prog.bolts;
    for (int i = 0; i < G->world.nnodes; i++)
        if (ck_node_beaten(G, i) && G->world.nodes[i].kind == CK_NODE_LEVEL) M->babies++;
    for (int i = 0; i < G->world.nnodes; i++) if (ck_node_beaten(G, i)) open_from(G, i, true);
    open_from(G, 0, true);
    M->hx = (float)G->world.nodes[M->at].x;
    M->hy = (float)G->world.nodes[M->at].y - 6;
    M->hdir = 1;
}

// Breadth-first route over open roads. Fills legs; returns the number of legs, or -1.
static int route(const ck_game *G, int from, int to, int legs[][2]) {
    const ck_world *W = &G->world;
    int prev[CK_MAXNODES], prevRoad[CK_MAXNODES], prevRev[CK_MAXNODES], q[CK_MAXNODES], qh = 0, qt = 0;
    for (int i = 0; i < CK_MAXNODES; i++) prev[i] = -2;
    prev[from] = -1;
    q[qt++] = from;
    while (qh < qt) {
        int c = q[qh++];
        if (c == to) break;
        for (int r = 0; r < W->nroads; r++) {
            if (!G->map.open[r]) continue;
            const ck_road *e = &W->roads[r];
            int n = -1, rev = 0;
            if (e->a == c) n = e->b;
            else if (e->b == c) { n = e->a; rev = 1; }
            if (n < 0 || prev[n] != -2) continue;
            prev[n] = c; prevRoad[n] = r; prevRev[n] = rev;
            q[qt++] = n;
        }
    }
    if (prev[to] == -2) return -1;
    int n = 0, c = to;
    while (prev[c] >= 0) { n++; c = prev[c]; }
    c = to;
    for (int k = n - 1; k >= 0; k--) { legs[k][0] = prevRoad[c]; legs[k][1] = prevRev[c]; c = prev[c]; }
    return n;
}

bool ck_node_reachable(const ck_game *G, int node) {
    int legs[CK_MAXROADS][2];
    return node == G->map.at || route(G, G->map.at, node, legs) >= 0;
}

void ck_map_say(ck_game *G, const char *text) {
    snprintf(G->map.msg, sizeof G->map.msg, "%s", text);
    G->map.msgT0 = G->map.now;
    G->map.hasMsg = true;
}

static void walk_to(ck_game *G, int id) {
    ck_map *M = &G->map;
    if (M->walking || id == M->at) return;
    int legs[CK_MAXROADS][2];
    int n = route(G, M->at, id, legs);
    if (n < 0) { ck_map_say(G, "Not open yet"); return; }
    for (int k = 0; k < n; k++) { M->legs[k].road = legs[k][0]; M->legs[k].rev = legs[k][1]; }
    M->nlegs = n; M->leg = 0; M->walkT0 = M->now; M->dest = id; M->walking = n > 0;
}

bool ck_map_step(ck_game *G, int dir) {
    // the stick: go to the open neighbour that best matches the pushed direction
    ck_map *M = &G->map;
    const ck_world *W = &G->world;
    float vx = dir == CK_LEFT ? -1 : dir == CK_RIGHT ? 1 : 0, vy = dir == CK_UP ? -1 : dir == CK_DOWN ? 1 : 0;
    if (M->walking || (!vx && !vy)) return false;
    const ck_node *here = &W->nodes[M->at];
    int best = -1;
    float bs = 0.3f;
    for (int r = 0; r < W->nroads; r++) {
        if (!M->open[r]) continue;
        const ck_road *e = &W->roads[r];
        int id = e->a == M->at ? e->b : e->b == M->at ? e->a : -1;
        if (id < 0) continue;
        float dx = (float)(W->nodes[id].x - here->x), dy = (float)(W->nodes[id].y - here->y), l = hypotf(dx, dy);
        if (!l) l = 1;
        float sc = (dx * vx + dy * vy) / l;
        if (sc > bs) { bs = sc; best = id; }
    }
    if (best >= 0) { walk_to(G, best); return true; }
    return false;
}

int ck_map_here(const ck_game *G) { return G->map.walking ? -1 : G->map.at; }

void ck_map_update(ck_game *G, float now) {
    ck_map *M = &G->map;
    const ck_world *W = &G->world;
    M->now = now;
    if (M->walking) {
        const ck_road *e = &W->roads[M->legs[M->leg].road];
        bool rev = M->legs[M->leg].rev;
        float dur = e->total / 75;
        if (dur < 0.3f) dur = 0.3f;
        float u = (now - M->walkT0) / dur;
        if (u > 1) u = 1;
        float uu = rev ? 1 - u : u, x, y, x2, y2;
        ck_road_point(e, uu, &x, &y);
        ck_road_point(e, rev ? (uu - 0.05f < 0 ? 0 : uu - 0.05f) : (uu + 0.05f > 1 ? 1 : uu + 0.05f), &x2, &y2);
        M->hdir = x2 >= x ? 1 : -1;
        M->hx = x; M->hy = y;
        if (u >= 1) {
            M->at = rev ? e->a : e->b;
            M->leg++;
            M->walkT0 = now;
            if (M->leg >= M->nlegs) M->walking = false;
        }
    } else {
        M->hx = (float)W->nodes[M->at].x;
        M->hy = (float)W->nodes[M->at].y - 6;
    }
    int keep = 0;
    for (int k = 0; k < M->nreveal; k++)
        if (now - M->reveal[k].t0 < M->reveal[k].dur) M->reveal[keep++] = M->reveal[k];
    M->nreveal = keep;
    if (M->hasMsg && now - M->msgT0 >= 1.8f) M->hasMsg = false;
}

void ck_map_beat(ck_game *G, int bolts, int gears) {
    ck_map *M = &G->map;
    const ck_world *W = &G->world;
    const ck_node *n = &W->nodes[M->at];
    if (M->walking || !is_play(n->kind)) { ck_map_say(G, "Walk to a level first"); return; }
    M->bolts += bolts;
    M->gears += gears;
    if (M->gears > W->gears) M->gears = W->gears;
    char msg[48];
    if (!ck_node_beaten(G, M->at)) {
        M->beaten |= (uint16_t)(1u << M->at);
        if (n->kind == CK_NODE_LEVEL || n->kind == CK_NODE_BOSS) M->babies++;
        if (n->kind == CK_NODE_BOSS) {
            if (W->bossmsg[0]) snprintf(msg, sizeof msg, "%s", W->bossmsg);
            else snprintf(msg, sizeof msg, "Fixed! Path to World %d open", W->num + 1);
        } else if (gears) snprintf(msg, sizeof msg, "+%d golden gear%s", gears, gears > 1 ? "s" : "");
        else snprintf(msg, sizeof msg, "Level clear!");
        ck_map_say(G, msg);
        ck_sfx(G, SFX_OPEN);
    } else if (gears) {
        snprintf(msg, sizeof msg, "+%d golden gear%s", gears, gears > 1 ? "s" : "");
        ck_map_say(G, msg);
    }
    open_from(G, M->at, false);
    for (int i = 0; i < W->nnodes; i++) if (ck_node_beaten(G, i)) open_from(G, i, false);   // gear gates may now open
    G->prog.beaten = M->beaten;
    G->prog.gears = (uint8_t)M->gears;
    G->prog.bolts = (uint16_t)(M->bolts > 65535 ? 65535 : M->bolts);
    G->prog.at = (uint8_t)M->at;
}
