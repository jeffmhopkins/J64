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
    CK_C = 1 << 7,   // C-up, C-down or C-right: switch between the action and the bubble gun
    CK_L = 1 << 8, CK_R = 1 << 9,   // the shoulder buttons (the paint studio's colours)
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
    SFX_HOP2, SFX_HOVER, SFX_BUY,
    SFX_DRUM0,   // the drum kit's hits, in ck_drums order
    SFX_COUNT = SFX_DRUM0 + 9
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
// Gravity is 80% of what it first was (0.3), and every upward kick a hero gets is scaled by CK_JK = sqrt(0.8),
// so heroes hang in the air longer but jump exactly as high. Running speed climbs from a creep over CK_RAMP frames.
#define CK_GRAV 0.24f
#define CK_JK   0.8944272f
#define CK_RAMP 30
#define CK_MAXNOTES  160   // 16 bars of eighth notes, with room to spare
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
#define CK_TRAIL 40

typedef struct {
    char name[40];
    int kind;
    int song;
    uint32_t baby;            // baby bot colour, 0xRRGGBB
    char hatch[16];           // side zone id
    int par, time;            // seconds; 0 = none
    char boss[16];
    int bossType;             // BOSS_*
    float grav;               // CK_GRAV, or two thirds of it on the Moon ("gravity: low")
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

// Accessories from the parts shop, the same list as K.ACC in web/lib/draw.js. Bought once with
// bolts for the whole family; each player picks what to wear in each slot.
enum {
    ACC_PARTY, ACC_FLOWER, ACC_BEANIE, ACC_BASEBALL, ACC_CHEF, ACC_PROPELLER, ACC_COWBOY, ACC_PIRATE, ACC_TOPHAT, ACC_WIZARD, ACC_CROWN, ACC_RAINBOWCAP,
    ACC_GLASSES, ACC_SHADES, ACC_HEARTS, ACC_STARS, ACC_GOGGLES, ACC_MUSTACHE, ACC_MONOCLE,
    ACC_BOWTIE, ACC_SCARF, ACC_NECKTIE, ACC_BANDANA, ACC_MEDAL,
    ACC_SWEATER, ACC_OVERALLS, ACC_JERSEY, ACC_TUX, ACC_SUPER, ACC_DISCO,
    ACC_SNEAKERS, ACC_BOOTS, ACC_RAINBOOTS, ACC_FLIPPERS, ACC_SKATES, ACC_LIGHTUP,
    ACC_COUNT
};
enum { SLOT_HEAD, SLOT_FACE, SLOT_NECK, SLOT_BODY, SLOT_FEET, SLOT_COUNT };
typedef struct { const char *name; uint8_t slot; uint8_t rainbow; uint16_t cost; } ck_accdef;   // rainbow: cycles colour
extern const ck_accdef ck_acc[ACC_COUNT];
#define CK_OWNS(c, a) (((c)->own[(a) >> 3] >> ((a) & 7)) & 1)
#define CK_CLOSET_MAGIC 0xC2
typedef struct {
    uint8_t magic;                   // CK_CLOSET_MAGIC, or the closet starts empty (older saves)
    uint8_t own[(ACC_COUNT + 7) / 8];  // bit per accessory
    uint8_t wear[2][SLOT_COUNT];       // per player and slot: 0 nothing, else accessory + 1
    // added later: paint pots and gear, valid when magic2 is CK_CLOSET_MAGIC2 (else they start fresh)
    uint8_t magic2;
    uint8_t paints;                    // bit per hero colour owned (the first two are free)
    uint8_t gear;                      // CK_GEAR_* bits
    uint8_t rainbow;                   // bit per player wearing the rainbow paint (their colour cycles)
} ck_closet;
#define CK_CLOSET_MAGIC2 0xC3
#define CK_FREE_PAINTS 0x03
enum { CK_GEAR_GUN = 1, CK_GEAR_JET = 2, CK_GEAR_RAINBOW = 4 };   // the bubble gun; the super jetpack (near-level glide); rainbow paint
// The shop sells the accessories, then (on its last row) the two heroes (free to swap), a paint pot
// per colour, the rainbow paint, the bubble gun and the super jetpack.
enum { SHOP_ROBOT = ACC_COUNT, SHOP_OSTRICH, SHOP_PAINT0, SHOP_RAINBOW = SHOP_PAINT0 + CK_NCOLS, SHOP_GUN, SHOP_JET, SHOP_COUNT };
#define CK_PAINT_COST 30
#define CK_RAINBOW_COST 150
#define CK_RAINBOW_COL 7   // a rainbow hero is drawn from the Snow sheet, tinted
#define CK_GUN_COST 100
#define CK_JET_COST 300
#define CK_GLIDE_VY 0.6f       // how fast a glide sinks
#define CK_SUPERGLIDE_VY 0.12f // with the super jetpack: almost level
int ck_shop_cost(int item);              // 0 for the heroes
const char *ck_shop_name(int item, char *buf, int n);


typedef struct {
    uint32_t magic;
    uint8_t world;                   // 1..CK_WORLDS
    uint8_t pad[3];
    ck_prog w[CK_WORLDS];
    ck_closet closet;                // added after the worlds, so older saves still line up
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
    int16_t runT; int8_t runDir;   // frames the same direction has been held: speed ramps up over CK_RAMP
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
    // where the hero has walked, newest first, a point every 2 px: the babies follow it like a train
    float trail[CK_TRAIL][2];
    int ntrail;
} ck_map;

// ---------------------------------------------------------------------------- the house
// The shop node on every map is the heroes' house: the yard, then rooms in a row along one floor
// (web/lib/houseart.js has the same layout). Down at a room's thing uses it; some open a little
// game of their own (SC_PAINT, SC_AQUA, SC_CARS, SC_PIANO). Rescued baby bots wander about.
#define CK_HOUSE_W 1440
#define CK_HOUSE_FLOOR 200
#define CK_HOUSE_CEIL 20
#define CK_HOUSE_PILLAR 8
#define CK_HOUSE_DOOR_TOP 120
#define CK_HOUSE_ROOMS 7
#define CK_HOUSE_SPOTS 10
#define CK_HOUSE_DECOR 6
#define CK_HOUSE_BABIES 16
enum { SPOT_SNACK, SPOT_PAINT, SPOT_PIANO, SPOT_AQUA, SPOT_BED, SPOT_CARS, SPOT_DRUMS, SPOT_MAIL, SPOT_TREE, SPOT_BACK };
// furniture frames, in the order tools/bake_sprites.mjs bakes them (HOUSEART.NAMES)
enum { FURN_TREE, FURN_MAILBOX, FURN_COUNTER, FURN_FRIDGE, FURN_STOVE, FURN_TABLE, FURN_EASEL, FURN_PIANO,
       FURN_TANK, FURN_BED, FURN_CAR, FURN_PLANT, FURN_DRUM, FURN_LAMP, FURN_TOOLS, FURN_SIGN, FURN_COUNT };
typedef struct { int x0, x1; const char *name; uint32_t wall, stripe, floor; } ck_room;
typedef struct { int x, kind, furn; const char *label; } ck_spot;
typedef struct { int x, furn; } ck_decor;
extern const ck_room ck_rooms[CK_HOUSE_ROOMS];
extern const ck_spot ck_spots[CK_HOUSE_SPOTS];
extern const ck_decor ck_decors[CK_HOUSE_DECOR];
// the paint studio's wall: 80 x 48 cells of 4 px from y 16; 0 is bare, else palette index + 1
#define CK_MURAL_W 80
#define CK_MURAL_H 48
#define CK_MURAL_CELL 4
#define CK_MURAL_Y 16
#define CK_MURAL_FLOOR 220
#define CK_MURAL_PAL 10
#define CK_MURAL_ERASER CK_MURAL_PAL   // the last choice rubs paint out
extern const uint32_t ck_mural_pal[CK_MURAL_PAL];
// the aquarium: water on the left, a beach on the right
// The aquarium is three screens wide: water, then a beach on the right (its last screen is the
// baked aqua_bg, the two before it the aqua_water tile).
#define CK_AQUA_W 960
#define CK_AQUA_TOP 44
#define CK_AQUA_SEABED 222
#define CK_AQUA_BEACH_X 876       // world x (236 on the last screen)
#define CK_AQUA_BEACH_Y 182
#define CK_AQUA_FISH 14
#define CK_AQUA_SHRIMP 8
#define CK_AQUA_CRABS 3
#define CK_AQUA_FOOD 12           // fish food flakes sinking at once
// the garage's race track: an oval ring seen from above
// The garage race: side on, each player in their own lane (two lanes stacked when two play).
// The cars drive themselves; A jumps the cones and crates, and a bump slows you right down.
#define CK_RACE_LEN 2400
#define CK_RACE_OBST 12
#define CK_RACE_CAR_X 80          // where the car sits on the screen
enum { OBST_CONE, OBST_CRATE };
void ck_race_obst(int i, float *x, int *kind, float *w, float *h);
// the piano: two octaves from middle C, 14 white keys
#define CK_KEYS_X 20
#define CK_KEYS_Y 150
#define CK_KEYS_W 20
#define CK_KEYS_H 80
#define CK_KEYS_BW 12
#define CK_KEYS_BH 48
#define CK_KEYS_N 24
void ck_piano_key(int i, float *x, bool *black);
// The music room's drum kit (SC_PIANO with house.drums set): each button hits its own piece.
#define CK_DRUMS 9
enum { DRUM_DRUM, DRUM_CYMBAL, DRUM_HANDS };
typedef struct { const char *name, *button; uint16_t btn; int kind; float x, y, r; uint32_t col; } ck_drum;
extern const ck_drum ck_drums[CK_DRUMS];   // semitone 0..23: the key's middle and colour
// One hero in the house or one of its games (not every game uses every field).
typedef struct {
    int slot;                  // player slot 0 or 1
    float x, y, vx, vy, anim;
    int face;
    bool ground, land, jet, brush;
    int eat, bubT, col;        // house: snack time; aqua: next bubble; paint: palette choice
    float bx, by;              // paint: where the brush was last frame
    float a, s, lapT, best;    // cars: (unused), speed, (unused), finish time
    int laps, key, rep, bounce;
    bool half;
} ck_hhero;
typedef struct { uint32_t hex; float x, x0, x1, vx, y, vy; int wait; } ck_hbaby;
typedef struct { int kind; float x, y, vx, ph; int scare; int happy; } ck_fish;   // happy: frames of a happy wiggle
typedef struct { float x, y; int life; } ck_food;
typedef struct { float x, y, vx, vy; int face, rest; } ck_shrimp;
typedef struct { float x, vx; int snap, wait; } ck_crab;
typedef struct { float x, y; int life; uint32_t col; } ck_note_fx;
enum { HM_WALK, HM_SLOTS, HM_SLEEP };
typedef struct {
    ck_hhero ps[2];            // in the house
    ck_hhero gs[2];            // in the game you went into
    int np;
    ck_hbaby babies[CK_HOUSE_BABIES];
    int nbabies;
    float cam;
    int mode, pick;
    float sleepT;
    char msg[32];
    int msgT;
    uint8_t mural[CK_MURAL_W * CK_MURAL_H];
    ck_fish fish[CK_AQUA_FISH];
    ck_shrimp shrimp[CK_AQUA_SHRIMP];
    ck_crab crabs[CK_AQUA_CRABS];
    ck_food food[CK_AQUA_FOOD];
    int nfood;
    float gcam;                // the camera in a game that scrolls (the aquarium)
    float count, endT, clock;  // cars: the countdown, the time since the winner got home, the race clock
    int win;                   // cars: the winner's index, or -1
    uint8_t lit[CK_KEYS_N];    // piano: frames a key stays lit
    ck_note_fx notes[16];
    int nnotes;
    int letter;                // the next letter in the mailbox
    bool drums;                // SC_PIANO shows the drum kit instead of the piano
} ck_house;
#define CK_SLOTS 3
typedef struct { bool used; int world, bolts; } ck_slot_info;

// ---------------------------------------------------------------------------- the whole game
enum { SC_TITLE, SC_SELECT, SC_MAP, SC_SHOP, SC_LEVEL, SC_JET, SC_PAUSE, SC_INTRO, SC_CUT, SC_MENU,
       SC_HOUSE, SC_PAINT, SC_AQUA, SC_CARS, SC_PIANO, SC_SLOTS };
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

    // START on the map: Keep playing, Change heroes, New game (asks once more first)
    int menuPick;
    bool menuAsk;
    // the parts shop: a cursor per player, and a short message
    int shopCur[2];
    bool gunOn[2];                   // per player: B fires the bubble gun instead of the kick or grab
    char shopMsg[32];
    int shopMsgT;

    ck_house house;
    // save slots: the frontend hands in all three at boot; G->slot is the one being played
    ck_saves slots[CK_SLOTS];
    int slot;
    int slotPick;
    bool saveNow;              // the frontend should write the save now (the bed)
    bool shopFromHouse;        // the parts shop goes back into the house
    float notes[4];            // piano notes to play this tick (Hz)
    int nnotes;

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
// Wipe every world, bolt, star and accessory and start again in World 1 (the heroes stay).
void ck_game_new_game(ck_game *G);
// What player slot s wears in each slot: an accessory id, or -1.
int ck_wearing(const ck_game *G, int s, int slot);
bool ck_shop_owns(const ck_game *G, int item);     // any shop item, SHOP_* included
bool ck_rainbow_skin(const ck_game *G, int slot);   // that player wears the rainbow paint
bool ck_gun_on(const ck_game *G, int slot);        // that player's B fires the bubble gun
void ck_map_baby_pos(const ck_game *G, int i, float *x, float *y);
enum { MENU_KEEP, MENU_HEROES, MENU_DROP, MENU_NEW };
int ck_menu_items(const ck_game *G, int *out);    // the map menu's rows (up to 4), MENU_*
// Save slots: give the core all three saves (after ck_game_boot, which plays slot 0). The
// frontend writes ck_game_saves() into slot G->slot.
void ck_game_slots(ck_game *G, const ck_saves *slots);
ck_slot_info ck_slot_summary(const ck_game *G, int k);
// Which spot (an index into ck_spots) a hero in the house stands at, or -1.
int ck_house_near(const ck_game *G, const ck_hhero *p);
// The yard's signpost (SPOT_BACK) is only there from World 2 on: it goes back a world.
bool ck_spot_here(const ck_game *G, int k);
// One 60 Hz tick. Clears and refills G->sfx.
void ck_game_step(ck_game *G, uint16_t pad1, uint16_t pad2);

// Helpers frontends use while drawing.
float ck_rand(ck_game *G);                 // [0, 1)
bool ck_has(const ck_player *p, int power);
// Put p on firm floor beside buddy b (who must stand on tiles); false if there is no clear spot.
bool ck_set_down(const ck_level *lv, ck_player *p, const ck_player *b);
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
