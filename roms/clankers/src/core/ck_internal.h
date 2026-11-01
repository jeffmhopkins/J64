// Shared between the core's .c files; not part of the frontend API.
#ifndef CK_INTERNAL_H
#define CK_INTERNAL_H

#include "ck.h"

#define CK_PI 3.14159265f

#define HELD(G, i, b) (((G)->in.held[i] & (b)) != 0)
#define HIT(G, i, b) ((((G)->in.held[i] & (b)) != 0) && !(((G)->in.prev[i] & (b)) != 0))
#define ANYHIT(G, b) (HIT(G, 0, b) || HIT(G, 1, b))

static inline float ck_clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static inline float ck_minf(float a, float b) { return a < b ? a : b; }
static inline float ck_maxf(float a, float b) { return a > b ? a : b; }
static inline float ck_absf(float a) { return a < 0 ? -a : a; }

void ck_sfx(ck_game *G, int id);
void ck_play(ck_game *G, int song);
void ck_puff(ck_game *G, float x, float y, int n, uint32_t col, uint8_t alpha);
void ck_sparks(ck_game *G, float x, float y, int n, uint32_t col);
void ck_confetti(ck_game *G, float x, float y, int n);
void ck_debris(ck_game *G, float x, float y);
void ck_step_parts(ck_game *G);
void ck_save(ck_game *G);

// level.c
enum { LV_PLAYING, LV_FINISH, LV_RETRY, LV_GAMEOVER, LV_PAUSE, LV_HATCH };
void ck_level_start(ck_game *G, int node, bool retry, bool useSnap);
int ck_level_update(ck_game *G);
void ck_level_finish(ck_game *G);          // record results into prog
int ck_level_song(const ck_level *lv);
void ck_jet_start(ck_game *G, int hatch);
bool ck_jet_update(ck_game *G);           // true when the zone is over
void ck_level_come_back(ck_game *G);

// map.c
void ck_map_reset(ck_game *G);
void ck_map_update(ck_game *G, float now);
bool ck_map_step(ck_game *G, int dir);    // CK_LEFT...; true if it started walking
void ck_map_beat(ck_game *G, int bolts, int gears);
void ck_map_say(ck_game *G, const char *text);


#endif
