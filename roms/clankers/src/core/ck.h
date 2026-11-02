// Clankers game core: the rules of the game, in plain C.
//
// Everything here is pure C (no libdragon, no browser), so the same code runs on the N64
// (roms/clankers/src/n64/), on the host for unit tests (tests/host/), and later in the
// browser through WebAssembly. A frontend feeds in pad input once per 60 Hz tick, calls
// ck_game_step(), then draws what the state says and plays the sounds it queued.
//
// It is a port of roms/clankers/web/game.js. Levels and the world map come from the text
// files in roms/clankers/data (see data/README.md).
#ifndef CLANKERS_CK_H
#define CLANKERS_CK_H

#include <stdbool.h>
#include <stdint.h>

#define CK_W        320
#define CK_H        240
#define CK_TS       16
#define CK_ROWS     15
#define CK_MAXW     256    // widest level, in tiles
#define CK_MAXENTS  640
#define CK_MAXPARTS 200
#define CK_MAXSFX   24
#define CK_MAXNODES 16
#define CK_MAXROADS 16
#define CK_MAXVIA   6
#define CK_NCOLS    8      // hero colours

// ---------------------------------------------------------------------------- input
enum {
    CK_LEFT = 1 << 0, CK_RIGHT = 1 << 1, CK_UP = 1 << 2, CK_DOWN = 1 << 3,
    CK_A = 1 << 4, CK_B = 1 << 5, CK_START = 1 << 6,
};
typedef struct { uint16_t held[2], prev[2]; } ck_input;

// ---------------------------------------------------------------------------- sound
// Sound effects and songs the core asks for. Each frontend decides how they sound.
enum {
    SFX_JUMP, SFX_BOOST, SFX_FLAP, SFX_BOLT, SFX_GEAR, SFX_REBOOT, SFX_WOBBLE, SFX_SPRING,
    SFX_CRATE, SFX_LEVER, SFX_DOOR, SFX_GRAB, SFX_KICK, SFX_BOING, SFX_BUTTON, SFX_CLANK,
    SFX_SELECT, SFX_CONFIRM, SFX_OPEN, SFX_STEP, SFX_HOP, SFX_NOPE, SFX_SPROUT, SFX_POWER,
    SFX_SHRINK, SFX_BLOW, SFX_STOMP, SFX_HATCH, SFX_OUCH, SFX_POP, SFX_TICK, SFX_STAR0,
    SFX_STAR1, SFX_STAR2, SFX_RECORD, SFX_FLAG, SFX_SLIDE, SFX_THUNDER, SFX_WHOOSH,
    SFX_COUNT
};
enum {
    SONG_NONE, SONG_TITLE, SONG_SELECT, SONG_MAP, SONG_LEVEL, SONG_LEVEL2, SONG_LEVEL3,
    SONG_BOSS, SONG_BONUS, SONG_RAINBOW, SONG_JET, SONG_RESCUE, SONG_CLEAR, SONG_LULLABY, SONG_STORM,
    SONG_MAP2, SONG_MAP3, SONG_MAP4, SONG_FACTORY, SONG_MOUNTAIN, SONG_MOON, SONG_TRAVEL, SONG_PARADE,
    SONG_COUNT
};
int ck_song_by_name(const char *name);   // "level2" -> SONG_LEVEL2, unknown -> SONG_NONE

// A sound to make: what one Web Audio tone() or noise() call makes in the browser build.
enum { WAVE_SQUARE, WAVE_TRIANGLE, WAVE_SAW, WAVE_SINE, WAVE_NOISE };
enum { FILT_BAND, FILT_HIGH, FILT_LOW };
typedef struct {
    uint8_t wave, filt;
    float freq;      // Hz (noise: filter frequency)
    float dur;       // seconds
    float vol;       // peak gain
    float when;      // seconds from now
    float slide;     // end frequency = freq * slide (0 = none)
    bool music;      // goes through the music volume rather than the effects volume
} ck_tone;
// The tones a sound effect is made of. Returns how many were written.
int ck_sfx_tones(int id, ck_tone *out, int max);

// Music from data/songs.txt.
#define CK_MAXTRACKS 4
#define CK_MAXNOTES  96
typedef struct {
    uint8_t wave;                 // WAVE_*, or WAVE_NOISE for a drum track
    bool drum;
    float vol;
    int n;
    float freq[CK_MAXNOTES];      // 0 = rest (drums: 1 kick, 2 snare, 3 hi-hat)
    uint8_t len[CK_MAXNOTES];     // in eighth notes
} ck_track;
typedef struct { bool valid, loop; float bpm; int ntracks; ck_track tracks[CK_MAXTRACKS]; } ck_song;
typedef struct {
    const ck_song *song;
    int id;
    int idx[CK_MAXTRACKS];
    float next[CK_MAXTRACKS];     // seconds until each track's next note
    bool done;
} ck_music;
// Parse songs.txt into songs[SONG_COUNT] (indexed by SONG_*). Returns how many it read.
int ck_parse_songs(const char *text, ck_song *songs);
void ck_music_start(ck_music *m, const ck_song *songs, int id);
// Advance by dt seconds; calls emit() for each note that starts, with how far into dt it starts.
void ck_music_advance(ck_music *m, float dt, void (*emit)(const ck_tone *t, void *u), void *u);

// ---------------------------------------------------------------------------- data files
enum { CK_KIND_LEVEL, CK_KIND_BOSS, CK_KIND_BONUS };
// The Big Glitch of each world (a boss level's "boss:"), and each world's look ("theme:").
enum { BOSS_CRANE, BOSS_STAMPER, BOSS_BOULDER, BOSS_HAULER, CK_BOSS_COUNT };
enum { THEME_JUNKYARD, THEME_FACTORY, THEME_MOUNTAIN, THEME_MOON, CK_THEME_COUNT };
#define CK_WORLDS 4

typedef struct {
    char name[40];
    int kind;
    int song;
    uint32_t baby;            // baby bot colour, 0xRRGGBB
    char hatch[16];           // side zone id
    int par, time;            // seconds; 0 = none
    char boss[16];
    int bossType;             // BOSS_*
    float grav;               // 0.3, or 0.2 on the Moon ("gravity: low")
    int w;                    // width in tiles
    char rows[CK_ROWS][CK_MAXW];
} ck_leveldef;

// Parse a level file. Returns false if the text has no map rows.
bool ck_parse_level(const char *text, ck_leveldef *out);

enum { CK_NODE_HOME, CK_NODE_LEVEL, CK_NODE_BOSS, CK_NODE_SHOP, CK_NODE_BONUS, CK_NODE_EXIT };

typedef struct {
    char id[12];
    int col, row, x, y;       // x, y: pixel centre on the map
    int kind;
    char label[40];
    char level[16];           // level file stem, "" if none
    int gate;                 // gears needed
    int num;                  // 1-based number among level nodes
} ck_node;

typedef struct {
    int a, b;                 // node indices
    int npts;                 // a, via..., b
    float pts[CK_MAXVIA + 2][2];
    float lens[CK_MAXVIA + 2];
    float total;
    int gate;
} ck_road;

typedef struct {
    char name[32];
    int num;
    char theme[16];
    int themeId;              // THEME_*
    int song;                 // the map's song
    int gears;
    uint32_t babies[4];
    char bossmsg[48];
    ck_node nodes[CK_MAXNODES];
    int nnodes;
    ck_road roads[CK_MAXROADS];
    int nroads;
} ck_world;

bool ck_parse_world(const char *text, ck_world *out);
int ck_world_node(const ck_world *w, const char *id);   // index or -1

// ---------------------------------------------------------------------------- progress (saved)
#define CK_SAVE_MAGIC 0x434B3031u   // "CK01"
typedef struct {
    uint32_t magic;
    uint16_t beaten;                 // bit per node
    uint8_t at;                      // node index
    uint8_t gears;
    uint16_t bolts;
    uint8_t lives;
    uint8_t little;                  // gentler speed for little kids (default on)
    uint8_t got[CK_MAXNODES];        // golden gears found, 3 bits
    uint8_t stars[CK_MAXNODES];      // stars earned, 3 bits
    uint32_t best[CK_MAXNODES];      // best time in frames, 0 = none
    uint8_t hero[2], col[2], joined[2];
} ck_prog;

void ck_prog_fresh(ck_prog *p);

// Everything the EEPROM holds: which world you are in, and each world's progress. A world you
// have not been to yet has magic 0. An older save (one ck_prog with magic "CK01" at the start)
// becomes World 1's progress.
#define CK_SAVES_MAGIC 0x434B3032u  // "CK02"
typedef struct {
    uint32_t magic;
    uint8_t world;                   // 1..CK_WORLDS
    uint8_t pad[3];
    ck_prog w[CK_WORLDS];
} ck_saves;
// Read a save from raw EEPROM bytes (at least sizeof(ck_saves)); anything unknown starts fresh.
void ck_saves_read(ck_saves *s, const void *raw);

// ---------------------------------------------------------------------------- particles
enum { PART_PUFF, PART_STAR, PART_CONF, PART_CHUNK };
typedef struct {
    float x, y, vx, vy, r, g, rot;
    int life, max;
    uint32_t col;       // 0xRRGGBB
    uint8_t alpha;      // base alpha 0..255
    uint8_t kind;
} ck_part;

// ---------------------------------------------------------------------------- level
// A moving body: what moveBody() in the browser game works on.
#define CK_BODY_FIELDS                                                     \
    float x, y, w, h, vx, vy;                                              \
    bool ground, bumped;                                                   \
    int8_t hitWall;                                                        \
    int16_t bumpTx, bumpTy
typedef struct { CK_BODY_FIELDS; } ck_body;

enum {
    E_NONE, E_BOLT, E_GEAR, E_ENEMY, E_TYRE, E_LEVER, E_HATCH, E_HATCHOUT, E_MOVER,
    E_SIGN, E_FLAG, E_PAD, E_POWER, E_SHOT, E_BUTTON,
};
enum { EN_BUMBLE, EN_SPROCKET, EN_GRUMBLE };
enum { ES_GLITCHY, ES_HAPPY, ES_BUBBLED, ES_SHELL, ES_KICKED, ES_CARRIED };
enum { MV_RAFT, MV_CRUSHER, MV_HOOK };
enum { CR_UP, CR_WARN, CR_DROP, CR_DOWN, CR_RISE };
enum { PW_NONE, PW_MEGA, PW_RAINBOW, PW_BUBBLE, PW_MAGNET };

typedef struct ck_ent {
    union { ck_body body; struct { CK_BODY_FIELDS; }; };
    uint8_t type, kind, state;
    bool dead, fly, orig, on, rain;
    int8_t face, dir;
    int16_t uid, id, owner, by;
    float t;
    float dx, dy;                         // how far a mover moved this frame
    int timer, delay, hopT, happyT, shellT, kickT, bounces, bubT, life, rise, pop;
    float spd, sq;
    float ax, ay, bx, by2, period;        // raft track
    float home, low;                      // crusher
    float px, py, len, ang;               // hook
    float outx, outy;                     // hatch: where you come back
} ck_ent;

enum { HERO_ROBOT, HERO_OSTRICH };

typedef struct {
    union { ck_body body; struct { CK_BODY_FIELDS; }; };
    int slot, i, hero, col;
    uint32_t hex;
    uint8_t stack[2]; int nstack;          // kept power-ups (mega, bubble), newest last
    int rainbowT, magnetT;
    int face, coyote, jetT, flapT, actT, inv, wob, bubble, slideT;
    bool boost, flapping, cheer, dead, low, spring, shoot;
    float safeX, safeY, anim;
    ck_ent *carry, *onMover;
} ck_player;

typedef struct {
    int time;
    bool sweep, fast, record, first, hasStars;
    bool stars[3], had[3];
} ck_result;

// What a checkpoint flag remembers: a lost try restarts from here with what was done still done.
typedef struct {
    bool valid;
    float x, y;
    int flag, timeF, bolts, boltGot, doorT;
    bool gotNow[3];
    char tiles[CK_ROWS][CK_MAXW];
    uint8_t gone[CK_MAXENTS / 8];
} ck_snap;

typedef struct {
    const ck_leveldef *def;
    int node;
    int w, h;
    char tiles[CK_ROWS][CK_MAXW];
    char contents[CK_ROWS][CK_MAXW];      // what a crate holds: 'o' 'G' 'M' 'R' 'B' 'N', or 0
    ck_ent ents[CK_MAXENTS];
    int nents, entCount;
    float startX, startY;
    int doorT, bolts, boltTotal, boltGot, foeTotal;
    bool got[3], gotNow[3];
    ck_player players[2];
    int nplayers;
    float t, camX;
    int frame, clearT, dyingT, nameT, timeF, shake, toastT;
    bool retry, timeUp;
    char toast[24];
    // The Big Glitch: bounce on its three red buttons while dodging it.
    //   crane: swings its magnet (ang, rope, mx, my)
    //   stamper: slides along its rail (x), shakes, slams down to the floor (y is its top)
    //   boulder: rolls along the floor (x, y centre, rot), sits dizzy after bumping a wall
    //   hauler: flies to and fro (x, y centre), stops, then shines a beam straight down
    bool hasBoss;
    struct {
        int type, hits, shake, ouch, state, timer, dir;
        bool fixed, padShown;
        float ang, rope, mx, my;
        float x, y, vy, low, rot, ft;
    } boss;
    bool hasShower;
    int showerLeft, showerDone;
    bool hasResult;
    ck_result result;
} ck_level;

// The jetpack side zone a hatch drops you into.
typedef struct { float x, y, w, h; uint8_t kind; } ck_obstacle;   // 0 pipe top, 1 pipe bottom, 2 pillar
typedef struct { float x, y, w, h, t; bool dead; } ck_item;
typedef struct {
    char name[24];
    float len, speed;
    ck_obstacle obs[32]; int nobs;
    ck_item items[48]; int nitems;
    struct { float x, y, w, h, vy; int inv, face; } ps[2];
    float t, scroll;
    int bolts, endT, nameT, fr;
    int hatch;                 // the hatch entity in the level
} ck_jet;

// ---------------------------------------------------------------------------- map
typedef struct {
    uint16_t beaten;
    int at, gears, bolts, babies;
    bool open[CK_MAXROADS];
    struct { int road; bool rev; } legs[CK_MAXROADS];
    int nlegs, leg, dest;
    bool walking;
    float walkT0;
    struct { int road; float t0, dur; } reveal[CK_MAXROADS];
    int nreveal;
    char msg[48];
    float msgT0;
    bool hasMsg;
    // hero position this frame (for drawing)
    float hx, hy;
    int hdir;
    float now;
} ck_map;

// ---------------------------------------------------------------------------- the whole game
enum { SC_TITLE, SC_SELECT, SC_MAP, SC_SHOP, SC_LEVEL, SC_JET, SC_PAUSE, SC_INTRO, SC_CUT };
enum { BS_MOVE, BS_WARN, BS_DROP, BS_DOWN, BS_RISE, BS_WAIT, BS_ROLL, BS_DIZZY, BS_FLY, BS_BEAM };

// The little film between worlds (G->t is its clock): 1 into the Gear Factory, 2 up the mine
// lift, 3 the rocket to the Moon, 4 the parade home (then THE END and the title). Where everyone
// is comes from ck_cut_cast(), so every frontend shows the same film.
extern const float ck_cut_len[CK_WORLDS + 1];
#define CK_CUT_GROUND 196       // where everyone stands
#define CK_CUT_SHAFT_X 160      // film 2: the lift shaft's middle, and the top the cage rides to
#define CK_CUT_SHAFT_TOP 64
#define CK_CUT_PAD_X 214        // film 3: the rocket's middle
#define CK_CUT_GATE_X 236       // film 1: the factory's roller door
#define CK_CUT_GATE_Y 136
#define CK_CUT_GATE_W 56
#define CK_CUT_GATE_H 60
typedef struct { float x, y, a; bool walk; } ck_cut_actor;   // a: how visible, 0..1
typedef struct {
    ck_cut_actor heroes[2], babies[16];
    int nheroes, nbabies;
    float gate;          // film 1: how far the door is open, 0..1
    float cageY;         // film 2: the lift cage's floor
    float rocketY;       // film 3: the rocket's bottom
    float shake;         // film 3: the rocket shaking before lift-off
    bool flame, bigFlame;
    float floatX;        // film 4: the parade float's left end
    const char *caption;
    float captionA;      // fades in
    bool gold;           // the last caption of the last film is gold
    float black, white;  // fade from black at the start, to white at the end, 0..1
} ck_cut_view;
struct ck_game;
void ck_cut_cast(const struct ck_game *G, ck_cut_view *v);

// The story before the title (G->t is its clock): the night, the Big Glitch storm, the babies
// blown away, then dawn. Any button skips it.
#define CK_INTRO_BOLT 12
typedef struct {
    float flash;               // white flash after thunder, 1 -> 0
    int boltLife;              // frames the lightning bolt still shows (0: none)
    float boltX;               // where it starts
    float boltDx[CK_INTRO_BOLT]; // its zigzag, one x step per 14 px down (new every frame)
} ck_intro;

// Reads a data file ("w2/world.txt", "w2/2-1.txt", ...) for the core. Returns malloc'd text
// (the core frees it), or NULL if there is no such file.
typedef char *(*ck_loader)(const char *path, void *user);

typedef struct ck_game {
    ck_world world;
    int worldNum;              // 1..CK_WORLDS
    ck_saves saves;            // prog is copied in here by ck_game_saves()
    ck_loader load;
    void *loadUser;
    int cut;                   // which film SC_CUT is showing
    uint32_t cutBabies[16];
    int ncutBabies;
    ck_leveldef defs[CK_MAXNODES];
    bool hasDef[CK_MAXNODES];
    ck_prog prog;
    bool progDirty;            // the frontend should save prog

    int scene;
    float t;                   // scene time, seconds
    bool ready[2];             // player select
    ck_map map;
    bool keepMap;
    ck_level lv;
    ck_jet jet;
    ck_snap snap;
    ck_intro intro;

    ck_input in;
    ck_part parts[CK_MAXPARTS];
    int nparts;
    uint8_t sfx[CK_MAXSFX];
    int nsfx;
    int song;                  // what should be playing now
    uint32_t rng;
} ck_game;

// Start the game: reads the save's world (and the others as you travel) through `load`.
bool ck_game_boot(ck_game *G, ck_loader load, void *user, const ck_saves *saved);
// Go to world n (1..CK_WORLDS), standing on its first node of kind `at` (CK_NODE_HOME or
// CK_NODE_EXIT). False if its files are missing.
bool ck_game_enter_world(ck_game *G, int n, int at);
// Show the film after world k (1..CK_WORLDS); when it ends you are in world k+1 (or at the title).
void ck_game_film(ck_game *G, int k);
// The save with the current world's progress copied in, for the frontend to write.
const ck_saves *ck_game_saves(ck_game *G);
// Lower level, for tests: set up World 1 from world.txt; then add each level with ck_game_add_level().
bool ck_game_init(ck_game *G, const char *world_text, const ck_prog *saved);
// Level text for the node whose level= is `stem`. Returns false if no node uses it.
bool ck_game_add_level(ck_game *G, const char *stem, const char *text);
// One 60 Hz tick. Clears and refills G->sfx.
void ck_game_step(ck_game *G, uint16_t pad1, uint16_t pad2);

// Helpers frontends use while drawing.
float ck_rand(ck_game *G);                 // [0, 1)
bool ck_has(const ck_player *p, int power);
bool ck_road_open(const ck_game *G, int road);
bool ck_node_reachable(const ck_game *G, int node);
bool ck_node_beaten(const ck_game *G, int node);
void ck_road_point(const ck_road *r, float u, float *x, float *y);
int ck_foes_left(const ck_level *lv);
int ck_map_here(const ck_game *G);        // node index, or -1 while walking
extern const uint32_t ck_cols[CK_NCOLS];
extern const char *const ck_col_names[CK_NCOLS];
extern const char *const ck_power_names[5];

#endif
