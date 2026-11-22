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
    CK_C = 1 << 7,   // C-up, C-down or C-right
    CK_L = 1 << 8, CK_R = 1 << 9,   // L or Z: the bubble gun; L and R: the paint studio's colours
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
    SONG_PINBALL, SONG_BLASTER, SONG_RACE, SONG_VICTORY, SONG_RACE2, SONG_RACE3, SONG_RACE4, SONG_BOLTBALL, SONG_JETRUN,
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
    // added later, so older saves keep their bits: costume pieces, then dresses, suits and shoes
    ACC_MIRROR, ACC_POLICECAP, ACC_MOHAWK, ACC_FIREHAT, ACC_SPACEHELM, ACC_HARDHAT, ACC_TIARA, ACC_NINJABAND,
    ACC_CLOWNWIG, ACC_KNIGHTHELM, ACC_EYEPATCH, ACC_REDNOSE, ACC_MASK, ACC_STETHO, ACC_PEARLS, ACC_CHAIN,
    ACC_CHEFCOAT, ACC_LABCOAT, ACC_POLICE, ACC_LEATHER, ACC_FIRECOAT, ACC_SPACESUIT, ACC_VEST, ACC_PIRATESHIRT,
    ACC_NINJASUIT, ACC_CLOWNSUIT, ACC_ARMOR, ACC_ROBE, ACC_COWVEST, ACC_SUNDRESS, ACC_PARTYDRESS, ACC_POLKADRESS,
    ACC_TUTU, ACC_BALLGOWN, ACC_SPARKLEDRESS, ACC_BLUESUIT, ACC_PINSTRIPE, ACC_WHITESUIT, ACC_PLAIDSUIT, ACC_VELVETSUIT,
    ACC_GOLDSUIT, ACC_DRESSSHOES, ACC_ROCKBOOTS, ACC_MOONBOOTS, ACC_CLOWNSHOES, ACC_SLIPPERS, ACC_HEELS, ACC_FLATS,
    ACC_SANDALS, ACC_HIGHTOPS, ACC_COWBOOTS,
    ACC_COUNT
};
enum { SLOT_HEAD, SLOT_FACE, SLOT_NECK, SLOT_BODY, SLOT_FEET, SLOT_COUNT };
typedef struct { const char *name; uint8_t slot; uint8_t rainbow; uint16_t cost; } ck_accdef;   // rainbow: cycles colour
extern const ck_accdef ck_acc[ACC_COUNT];
#define CK_CLOSET_MAGIC 0xC2
#define CK_OWN1_BITS 40               // accessories 0..39 own a bit in own[], the rest in own2[]
typedef struct {
    uint8_t magic;                   // CK_CLOSET_MAGIC, or the closet starts empty (older saves)
    uint8_t own[CK_OWN1_BITS / 8];   // bit per accessory
    uint8_t wear[2][SLOT_COUNT];       // per player and slot: 0 nothing, else accessory + 1
    // added later: paint pots and gear, valid when magic2 is CK_CLOSET_MAGIC2 (else they start fresh)
    uint8_t magic2;
    uint8_t paints;                    // bit per hero colour owned (the first two are free)
    uint8_t gear;                      // CK_GEAR_* bits
    uint8_t rainbow;                   // bit per player wearing the rainbow paint (their colour cycles)
    // added later: fish caught in the pond, valid when magic3 is CK_CLOSET_MAGIC3 (else 0)
    uint8_t magic3, pad3;
    uint16_t fished;
    // added later: the costume pieces and fashion line, valid when magic4 is CK_CLOSET_MAGIC4
    uint8_t magic4;
    uint8_t own2[7];                 // bit per accessory from CK_OWN1_BITS on
} ck_closet;
#define CK_CLOSET_MAGIC4 0xC5
_Static_assert(ACC_COUNT <= CK_OWN1_BITS + 7 * 8, "own2 has room for every accessory");
bool ck_owns_acc(const ck_closet *c, int a);
void ck_own_acc(ck_closet *c, int a);
#define CK_OWNS(c, a) ck_owns_acc(c, a)
// Whole looks: one buy gets every piece still missing, one press puts them all on (and takes the
// other slots off). The same list as K.COSTUMES in web/lib/draw.js.
#define CK_COSTUMES 16
typedef struct { const char *name; int8_t piece[SLOT_COUNT]; } ck_costume;   // -1: nothing in that slot
extern const ck_costume ck_costumes[CK_COSTUMES];
#define CK_CLOSET_MAGIC2 0xC3
#define CK_CLOSET_MAGIC3 0xC4
#define CK_FREE_PAINTS 0x03
enum { CK_GEAR_GUN = 1, CK_GEAR_JET = 2, CK_GEAR_RAINBOW = 4 };   // the bubble gun; the super jetpack (near-level glide); rainbow paint
// The shop sells the accessories, then (on its last row) the two heroes (free to swap), a paint pot
// per colour, the rainbow paint, the bubble gun and the super jetpack.
enum { SHOP_ROBOT = ACC_COUNT, SHOP_OSTRICH, SHOP_PAINT0, SHOP_RAINBOW = SHOP_PAINT0 + CK_NCOLS, SHOP_GUN, SHOP_JET, SHOP_COSTUME0, SHOP_COUNT = SHOP_COSTUME0 + CK_COSTUMES };
// The shop shows its items in sections (K.SHOP_SECS), each starting a new row of cols tiles: 12
// across for one player, 6 for each half when two play. CK_SHOP_ROWS rows show at once.
#define CK_SHOP_SECS 9
#define CK_SHOP_ROWS 4
int ck_shop_rows(int cols);                              // how many rows in all
int ck_shop_row_of(int item, int cols, int *col);        // the row (and column) an item sits in
int ck_shop_row_len(int row, int cols);
int ck_shop_item_at(int row, int col, int cols);         // -1 past the row's end
const char *ck_shop_sec_name(int item);
#define CK_PAINT_COST 30
#define CK_RAINBOW_COST 150
#define CK_RAINBOW_COL 7   // a rainbow hero is drawn from the Snow sheet, tinted
#define CK_GUN_COST 100
#define CK_JET_COST 300
#define CK_GLIDE_VY 0.6f       // how fast a glide sinks
#define CK_SUPERGLIDE_VY 0.12f // with the super jetpack: almost level
int ck_shop_cost(int item);              // 0 for the heroes; a costume: all its pieces
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
#define CK_HOUSE_W 1900
#define CK_HOUSE_FLOOR 200
#define CK_HOUSE_CEIL 20
#define CK_HOUSE_PILLAR 8
#define CK_HOUSE_DOOR_TOP 120
#define CK_HOUSE_ROOMS 8
#define CK_HOUSE_SPOTS 16
#define CK_HOUSE_DECOR 5
#define CK_HOUSE_BABIES 16
enum { SPOT_SNACK, SPOT_PAINT, SPOT_PIANO, SPOT_AQUA, SPOT_BED, SPOT_CARS, SPOT_DRUMS, SPOT_MAIL, SPOT_TREE, SPOT_BACK, SPOT_STOVE,
       SPOT_PINBALL, SPOT_BLAST, SPOT_POND, SPOT_BOLT, SPOT_JETRUN };
// furniture frames, in the order tools/bake_sprites.mjs bakes them (HOUSEART.NAMES)
enum { FURN_TREE, FURN_MAILBOX, FURN_COUNTER, FURN_FRIDGE, FURN_STOVE, FURN_TABLE, FURN_EASEL, FURN_PIANO,
       FURN_TANK, FURN_BED, FURN_CAR, FURN_PLANT, FURN_DRUM, FURN_LAMP, FURN_TOOLS, FURN_SIGN,
       FURN_PINBALL, FURN_ARCADE, FURN_POND, FURN_PADDLE, FURN_JETRUN, FURN_COUNT };
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
#define CK_MURAL_ERASER CK_MURAL_PAL   // after the colours: rub paint out
#define CK_MURAL_CLEAR (CK_MURAL_PAL + 1)   // then: B wipes the whole wall
#define CK_MURAL_CHOICES (CK_MURAL_PAL + 2)
extern const uint32_t ck_mural_pal[CK_MURAL_PAL];
// A mural cell holds 0 (bare), 1..10 a palette colour, 11..20 its dark shade, 21..30 its light shade.
uint32_t ck_mural_hex(int v);
// Stickers: B swaps the brush for one, A stamps it, C picks which of the 8. Each is drawn in five
// roles (main, dark, light, white, ink) that take their shades from the colour you have picked,
// with an ink outline round it, on a grid CK_STICKER_N cells across.
#define CK_STICKERS 8
typedef struct { uint8_t kind; float x, y, w, h; uint32_t col; } ck_fshape;   // kind 0: a circle (w is its radius), 1: a box
#define CK_STICKER_N 13
enum { ROLE_MAIN, ROLE_DARK, ROLE_LIGHT, ROLE_WHITE, ROLE_INK };
typedef struct { const char *name; int n; ck_fshape s[12]; } ck_stickerart;   // col holds a ROLE_*; x, y, w, h in cells, boxes inclusive
extern const ck_stickerart ck_stickers[CK_STICKERS];
// the cells of sticker `kind` stamped in colour c (CK_MURAL_ERASER rubs out its shape): out[y][x] is
// the mural value, or -1 where the sticker leaves the wall alone
void ck_sticker_cells(int kind, int c, int out[CK_STICKER_N][CK_STICKER_N]);
// the aquarium: water on the left, a beach on the right
// The aquarium is six screens wide: five of water, each its own place with its own fish (the
// sections below), then a beach on the right (its last screen is the baked aqua_bg, the ones
// before it the aqua_water tile with each section's plants and things drawn on top).
#define CK_AQUA_W 1920
#define CK_AQUA_SECTIONS 5
#define CK_AQUA_SEC_W 320
enum { SEC_KELP, SEC_REEF, SEC_WRECK, SEC_DEEP, SEC_CAVES };
extern const char *const ck_aqua_sections[CK_AQUA_SECTIONS];
// fish kinds: the first three are sprites, the rest are drawn from shapes
enum { AFISH_ORANGE, AFISH_YELLOW, AFISH_PINK, AFISH_ANGEL, AFISH_PUFFER, AFISH_JELLY, AFISH_EEL, AFISH_CLOWN, AFISH_TANG, AFISH_RAY, AFISH_KINDS };
#define CK_AQUA_PER_SEC 6
extern const int ck_aqua_pop[CK_AQUA_SECTIONS][CK_AQUA_PER_SEC];   // the kinds of fish living in each section
#define CK_AQUA_TOP 44
#define CK_AQUA_SEABED 222
#define CK_AQUA_BEACH_X 1836      // world x (236 on the last screen)
#define CK_AQUA_BEACH_Y 182
#define CK_AQUA_FISH (CK_AQUA_SECTIONS * CK_AQUA_PER_SEC)
#define CK_AQUA_SHRIMP 8
#define CK_AQUA_CRABS 3
#define CK_AQUA_FOOD 12           // fish food flakes sinking at once
// The garage race, seen from behind the car: the road runs off to a horizon and bends left and
// right, the stick steers across three lanes to dodge cones, crates, oil and slow cars, and a bend
// pushes the car to its outside. Up or A goes faster, down or B brakes. Boost pads speed you up and
// ramps jump you over whatever is just past them. Two players get a half of the screen each.
#define CK_RACE_LEN 9600          // about a minute at cruising speed
int ck_race_song(int track);
#define CK_RACE_CARS 4            // always four on the grid: the players, then computer cars
extern const uint32_t ck_race_bot_col[CK_RACE_CARS - 1];
#define CK_RACE_OBST 36           // sparse at the start, closer together further on
#define CK_RACE_LANE 40           // the lanes are at -40, 0 and 40 from the road's middle
#define CK_RACE_ROAD 64           // the road's half width; past it is the kerb, then grass that slows you
#define CK_RACE_EDGE 84           // how far from the middle a car can go
#define CK_RACE_PADS 14           // boost pads and ramps along the road
enum { PAD_BOOST, PAD_RAMP };
// the view: the horizon's height, the depth scale, how far ahead of the camera your car sits, and
// px per unit across at the bottom of a 320 px wide view. The view is squashed toward the horizon
// (ck_race_row): the far road holds more distance than a true camera would show, so things come
// into sight sooner and drift slowly down the top of the screen before they rush past your car.
#define CK_RACE_HZ 96
#define CK_RACE_ZC 2880.0f
#define CK_RACE_CAR_Z 26.0f
#define CK_RACE_PX0 2.66f
#define CK_RACE_SQUASH 0.6f       // 1 is a true camera; less holds more road near the horizon
#define CK_RACE_FAR 3000.0f       // how far ahead things are drawn
float ck_race_row(float zr);      // px below the horizon of something zr ahead of the camera
float ck_race_depth(float row);   // and back: how far ahead a row below the horizon is
#define CK_RACE_BEND 0.0026f      // how far a bend carries the road sideways in the distance
// The kinds work the same on every track and only look different: a small thing, a big block, a
// slippery patch, and something that moves (cars and moon rovers drive on, tumbleweeds and
// penguins wander across the road).
enum { OBST_CONE, OBST_CRATE, OBST_OIL, OBST_CAR };
#define CK_RACE_TRACKS 4
enum { TRACK_HILLS, TRACK_DESERT, TRACK_SNOW, TRACK_MOON };
typedef struct { const char *name; uint32_t ground, deco, road, kerbA, kerbB, line, sky, hills; } ck_racetrack;
extern const ck_racetrack ck_tracks[CK_RACE_TRACKS];
// obstacle i on a track: how far along the road (at race time `clock`: movers move), across, its kind and size
void ck_race_obst(int track, int i, float clock, float *d, float *x, int *kind, float *w, float *h);
// how hard the road bends at distance d (-1 hard left .. 1 hard right, 0 straight)
float ck_race_curve(int track, float d);
// boost pad or ramp i: how far along, across, and which (pads are 28 across and 30 long)
void ck_race_pad(int track, int i, float *d, float *x, int *kind);
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
// ---- the arcade room's pinball table (SC_PINBALL): one tall table, both players share the flippers.
// B or L flips the left flippers, A or R the right ones; A also fires a ball waiting in the launch lane.
// The table is CK_PIN_W x CK_PIN_H in its own units, drawn at CK_PIN_ZOOM and scrolled to follow the
// ball. On it: three rollover lanes at the top, three gear bumpers and two tyres, a spinner up the
// left orbit, a bank of four drop targets, a saucer that locks balls for multiball, slingshots over
// the inlanes, and a third flipper up the right side.
#define CK_PIN_W 240
#define CK_PIN_H 452
#define CK_PIN_ZOOM 0.8f
#define CK_PIN_VIEW 300           // table units that fit on the screen
#define CK_PIN_OX 64              // the screen x of the table's left edge
#define CK_PIN_R 4                // the ball's radius
#define CK_PIN_WALLS 19
#define CK_PIN_BUMPS 5
#define CK_PIN_LIGHTS 3
#define CK_PIN_DROPS 4
#define CK_PIN_FLIPS 3            // left, right, and the upper one on the right
#define CK_PIN_SLINGS 2
#define CK_PIN_BALLS 3
#define CK_PIN_MULTI 3            // balls on the table at once in multiball
#define CK_PIN_DRAIN 452
typedef struct { float x0, y0, x1, y1; } ck_seg;
typedef struct { float x, y, r; int score; } ck_bump;
typedef struct { float x, y, len; int side; } ck_flipdef;   // side 0 points right, 1 left
enum { PINMSG_NONE, PINMSG_BONUS, PINMSG_LOCK, PINMSG_MULTI, PINMSG_DOUBLE, PINMSG_SPIN };
extern const ck_seg ck_pin_walls[CK_PIN_WALLS];
extern const ck_bump ck_pin_bumps[CK_PIN_BUMPS];
extern const ck_seg ck_pin_gate;   // over the launch lane's top: the ball only goes out through it
extern const ck_seg ck_pin_spinner;   // across the left orbit
extern const ck_seg ck_pin_slings[CK_PIN_SLINGS];   // the kicking faces over the inlanes
extern const ck_seg ck_pin_drops[CK_PIN_DROPS];
extern const float ck_pin_lights[CK_PIN_LIGHTS][2];
extern const float ck_pin_saucer[2];
extern const ck_flipdef ck_pin_flips[CK_PIN_FLIPS];
// flipper k at angle a: from its pivot to its tip
void ck_pin_flipper(int k, float a, float *x0, float *y0, float *x1, float *y1);
typedef struct { float x, y, vx, vy; bool on; int held; } ck_pball;   // held: frames left in the saucer
bool ck_pin_in_lane(const ck_pball *b);   // a ball waiting in the launch lane
typedef struct {
    ck_pball ball[CK_PIN_MULTI];
    float flip[CK_PIN_FLIPS], flipW[CK_PIN_FLIPS];   // each flipper's angle and how far it turned this frame
    int balls, score, best;
    int flash[CK_PIN_BUMPS];   // frames a bumper stays lit after a hit
    int sling[CK_PIN_SLINGS];
    int lit, litT;             // the top lights that are lit (bits); litT: the all-lit flash
    int drops, dropT;          // the drop targets that are down (bits); dropT: the bank's reset flash
    float spin, spinA;         // the spinner's speed and angle
    int locks, x2T;            // balls locked in the saucer; frames of double scoring left
    int msg, msgT;             // the banner on show
    float cam;                 // the table's scroll, in table units
    bool over;
    int overT;
} ck_pinball;
// ---- the arcade cabinet (SC_BLAST): Bug Blaster. Rows of glitch bugs march side to side and step
// down; each player has a cannon on the bottom (the stick moves it, A or B fires). Three shields.
#define CK_BUG_COLS 8
#define CK_BUG_ROWS 4
#define CK_BUG_DX 24
#define CK_BUG_DY 18
#define CK_BLAST_Y 216            // the cannons' row
#define CK_BLAST_BOMBS 6
#define CK_SHIELDS 3
#define CK_SHIELD_W 6             // cells across
#define CK_SHIELD_H 3
#define CK_SHIELD_CELL 6
#define CK_SHIELD_Y 180
extern const float ck_shield_x[CK_SHIELDS];   // each shield's left edge
typedef struct { float x, y; bool on; } ck_shot;
typedef struct {
    uint32_t bugs;             // one bit a bug, row by row
    float bx, by;              // the top left bug's middle
    int dir, stepT, frame;
    float gun[2];
    int inv[2];
    ck_shot shot[2], bomb[CK_BLAST_BOMBS];
    uint8_t shield[CK_SHIELDS][CK_SHIELD_W * CK_SHIELD_H];
    int lives, score, best, wave, waveT;
    bool over;
    int overT;
} ck_blaster;
void ck_bug_at(const ck_blaster *B, int k, float *x, float *y);
// ---- the arcade's fourth cabinet (SC_JETRUN): Jet Run, the long version of the jetpack zones.
// Fly with the jetpack as in a zone (up or A rises, down sinks, letting go hovers, left and right
// move across) through gaps in pipes that go on and on. Each player presses A to join; once one has,
// the others have a short countdown to join too (as in Bolt Ball); every CK_RUN_STAGE_LEN the scenery changes and it all gets quicker and
// tighter. A bump costs one of the team's lives; bolts and gates passed score.
#define CK_RUN_COLS 6
#define CK_RUN_BOLTS 18
#define CK_RUN_FLOOR 214          // the ground's top
#define CK_RUN_X 64               // where the heroes start, across the screen
#define CK_RUN_WAIT 180           // frames the others get to join once one player is ready
#define CK_RUN_HW 12              // a hero's box
#define CK_RUN_HH 20
#define CK_RUN_W 28               // a pipe's width
#define CK_RUN_THEMES 9
#define CK_RUN_STAGE_LEN 2000.0f
#define CK_RUN_LIVES 3
#define CK_RUN_BOLT_LIFE 300      // bolts for an extra life (never more than CK_RUN_LIVES)
// a gap between two pipes, a pillar from the ground, a pipe from the top, a spinning gear in the
// air, two pipes that crush together and part again, a glitch drone that bobs and flies at you
enum { RUNCOL_PIPES, RUNCOL_PILLAR, RUNCOL_HANG, RUNCOL_GEAR, RUNCOL_CRUSH, RUNCOL_DRONE };
#define CK_RUN_GEAR_R 20
#define CK_RUN_DRONE_R 10
enum { RUN_READY, RUN_PLAY, RUN_OVER };
typedef struct { float x, gy, gap, amp, ph, spd, pulse; int kind; bool passed; } ck_runcol;   // amp, spd: how far and fast it sways; pulse: how far a crusher's gap opens and shuts
typedef struct { float x, y; bool on; } ck_runbolt;
typedef struct {
    int state, stateT;
    float scroll, speed, t;
    ck_runcol col[CK_RUN_COLS];
    ck_runbolt bolt[CK_RUN_BOLTS];
    float nextX, lastGy;      // where the next column goes, and the last one's gap
    struct { float x, y, vy; int inv; bool on; } ps[2];   // on: this player joined in
    int readyT;
    int lives, score, best, bolts, gates, stage, stageT, shake;
    int lifeT;                 // frames the extra life message shows
} ck_jetrun;
extern const char *const ck_run_stage_names[CK_RUN_THEMES];
float ck_run_gap_y(const ck_jetrun *J, const ck_runcol *c);   // a column's gap middle (or a gear's or drone's middle) right now
float ck_run_gap(const ck_jetrun *J, const ck_runcol *c);     // its gap's height right now (crushers open and shut)
float ck_run_mid(const ck_runcol *c);                         // its middle across
float ck_run_safe_y(const ck_jetrun *J, const ck_runcol *c);  // where a hero's middle should be to get by it right now
// ---- the arcade's third cabinet (SC_BOLT): Bolt Ball. A paddle each side, a ball between: miss
// it and the other side scores, first to CK_BOLT_WIN wins. Everyone playing presses A to say
// ready; with two players, if only one says ready before the countdown ends, they play the
// computer. Now and then a bolt floats in the middle: knock the ball through it for a long paddle.
#define CK_BOLT_TOP 26            // the court's top and bottom walls
#define CK_BOLT_BOT 226
#define CK_BOLT_PX 22             // the left paddle's face (the right one mirrors it)
#define CK_BOLT_PW 6
#define CK_BOLT_PH 34
#define CK_BOLT_BIG 52            // a paddle with the bolt's power
#define CK_BOLT_R 4
#define CK_BOLT_WIN 7
#define CK_BOLT_WAIT 180          // frames the ready countdown gives the other player
enum { BOLT_READY, BOLT_SERVE, BOLT_PLAY, BOLT_WON };
typedef struct {
    int state, stateT;
    bool ready[2];             // by player
    int readyT;                // the countdown, once one player is ready
    int who[2];                // each side's player (0 or 1), or -1 for the computer; left first
    float py[2], ph[2];        // each paddle's middle and height
    int big[2];                // frames of long paddle left
    float bx, by, vx, vy;
    int score[2], serveTo, rally, last, win;
    float aim;                 // where on its paddle the computer means to hit
    bool bolt;
    float boltX, boltY;
    int boltT;
} ck_boltball;
// ---- the yard's pond (SC_FISH): fishing. Hold A to swing the rod back, let go to cast; up and
// down raise and sink the hook; when a fish bites, A hooks it; then hold A to reel in, letting go
// when the line gets tight so it does not snap. B reels in an empty line.
#define CK_POND_FISH 6
#define CK_POND_TOP 112           // the water's surface
#define CK_POND_BED 226
#define CK_POND_BANK 70           // the bank's edge: a hooked fish is caught when it gets here
// The pond starts with plain little fish; the more the family catches (saved), the higher the
// pond's level and the bigger, cooler and harder to reel its fish.
enum { CATCH_MINNOW, CATCH_GOLDFISH, CATCH_BASS, CATCH_GOLDEN, CATCH_BOOT,
       CATCH_SARDINE, CATCH_PERCH, CATCH_TROUT, CATCH_CATFISH, CATCH_PIKE, CATCH_KOI, CATCH_ROBO, CATCH_KING, CATCH_COUNT };
typedef struct { const char *name; uint32_t col, col2; float len, speed, pull; int bolts; } ck_catch;
extern const ck_catch ck_catches[CATCH_COUNT];
#define CK_POND_LEVELS 5
extern const int ck_pond_level_at[CK_POND_LEVELS];   // fish caught this visit to reach each level
int ck_pond_level(int fished);
typedef struct { int kind; float x, y, vx; int scare; } ck_pfish;
enum { ROD_IDLE, ROD_CHARGE, ROD_FLY, ROD_WAIT, ROD_BITE, ROD_REEL, ROD_GOT, ROD_SNAP };
typedef struct { int state, fish, t, nibble; float power, x, y, vx, vy, tension, bob; bool pulling; } ck_rod;   // bob: the bobber's x
typedef struct { ck_pfish fish[CK_POND_FISH]; ck_rod rod[2]; int caught, got[CATCH_COUNT], last, level, levelT, fished; } ck_pond;   // fished: this visit, not counting boots
void ck_rod_tip(int i, float *x, float *y);   // player i's rod tip (they stand on the bank)
// One hero in the house or one of its games (not every game uses every field).
typedef struct {
    int slot;                  // player slot 0 or 1
    float x, y, vx, vy, anim;
    int face;
    bool ground, land, jet, brush;
    int eat, bubT, col;        // house: snack time; aqua: next bubble; paint: palette choice
    float bx, by;              // paint: where the brush was last frame
    float a, s, sky, best;     // cars: distance along the road, speed, how far the skyline has turned, finish time
    float jz, jvz, lean;       // cars: height off the road and climb speed after a ramp, how hard it leans into a turn
    int boost;                 // cars: frames of boost pad speed left
    uint32_t pads;             // cars: the boost pads and ramps you have used, one bit each
    int laps, key, rep, bounce;
    int place;                 // cars: where you finished (1 first), 0 still racing
    bool half;
    int hold;                  // house: the food in your hand, FOOD_* + 1 (0: nothing)
    uint64_t garnish;          // a cooked dish's extras on top (bits of ck_cook_ingr)
    uint64_t hit;              // cars: the obstacles you have bumped, one bit each
    int size;                  // paint: the brush size, 0 (one cell) to 2 (three cells across)
    bool stick;                // paint: holding a sticker instead of the brush
    int kind;                  // paint: which sticker
} ck_hhero;
typedef struct { uint32_t hex; float x, x0, x1, vx, y, vy; int wait; } ck_hbaby;
typedef struct { int kind; float x, y, vx, ph; int scare, happy, follow, who, home; } ck_fish;   // happy: a happy wiggle; follow: frames it swims after hero `who`; home: its section
typedef struct { float x, y; int life; } ck_flake;
typedef struct { float x, y, vx, vy; int face, rest, held, spin; } ck_shrimp;   // held: in hero held - 1's hand; spin: frames tumbling after a throw
typedef struct { float x, vx, y, vy; int snap, wait, run; } ck_crab;   // y: a jump when poked; run: frames of running away
typedef struct { float x, y; int life; uint32_t col; } ck_note_fx;
enum { HM_WALK, HM_SLOTS, HM_SLEEP, HM_FRIDGE, HM_LETTER, HM_COOK };
// Food from the fridge (and apples off the tree): held in the hand, A eats it, B throws it.
// Each is drawn from a few circles and boxes, the same in both frontends.
enum { FOOD_APPLE, FOOD_BANANA, FOOD_CHEESE, FOOD_CARROT, FOOD_CAKE, FOOD_ICECREAM, FOOD_MILK, FOOD_PIZZA, FOOD_COOKIE,   // the fridge
       FOOD_EGG, FOOD_TOMATO, FOOD_MUSHROOM, FOOD_BREAD, FOOD_POTATO, FOOD_RICE, FOOD_SAUSAGE, FOOD_PASTA,                 // stove ingredients
       FOOD_STRAWBERRY, FOOD_ORANGE, FOOD_LEMON, FOOD_GRAPES, FOOD_CHERRY, FOOD_PEAR, FOOD_PINEAPPLE, FOOD_BLUEBERRY, FOOD_WATERMELON, FOOD_ONION, FOOD_PEPPER, FOOD_BROCCOLI, FOOD_PEAS, FOOD_CORN, FOOD_LETTUCE, FOOD_CUCUMBER, FOOD_PUMPKIN, FOOD_BEANS, FOOD_CHICKEN, FOOD_FISH, FOOD_SHRIMP, FOOD_BACON, FOOD_HAM, FOOD_TOFU, FOOD_CHOCOLATE, FOOD_HONEY, FOOD_YOGURT, FOOD_CREAM, FOOD_BUTTER, FOOD_JAM, FOOD_FLOUR, FOOD_NOODLES, FOOD_TORTILLA, FOOD_OATS, FOOD_NUTS, FOOD_SPRINKLES,
       FOOD_DISH0 };   // then every dish the stove cooks, in DISH_* order
// what the stove cooks: food FOOD_DISH0 + DISH_*, each its own name and look
enum {
    DISH_OMELETTE, DISH_CHEESY_OMELETTE, DISH_MUSHROOM_OMELETTE, DISH_TOMATO_OMELETTE, DISH_POTATO_OMELETTE,
    DISH_BIG_BREAKFAST, DISH_TOAST, DISH_CHEESE_TOASTIE, DISH_PIZZA_TOASTIE, DISH_MUSHROOM_TOAST,
    DISH_TOMATO_SOUP, DISH_MUSHROOM_SOUP, DISH_CARROT_SOUP, DISH_CHEESE_SOUP, DISH_VEGGIE_STEW,
    DISH_SAUSAGE_STEW, DISH_HOTDOG, DISH_CHEESY_HOTDOG, DISH_KETCHUP_HOTDOG, DISH_SPAGHETTI, DISH_MEATBALLS,
    DISH_MAC_CHEESE, DISH_MUSHROOM_PASTA, DISH_BUTTER_PASTA, DISH_FRIED_RICE, DISH_SAUSAGE_RICE,
    DISH_VEGGIE_RICE, DISH_RICE_PUDDING, DISH_RICE_BOWL, DISH_APPLE_PIE, DISH_FRUIT_PIE, DISH_PANCAKES,
    DISH_CHIPS, DISH_CHEESY_CHIPS, DISH_SAUSAGE_CHIPS, DISH_BANANA_BREAD, DISH_CARROT_CAKE,
    DISH_CHEESE_PLATE,
    DISH_CURRY, DISH_ROAST, DISH_FISH_SUPPER, DISH_SKEWERS, DISH_TACOS, DISH_PORRIDGE, DISH_CHOC_CAKE, DISH_SUNDAE, DISH_SMOOTHIE, DISH_PUMPKIN_PIE, DISH_FRUIT_SALAD, DISH_SALAD, DISH_STIR_FRY, DISH_COOKIES,
    CK_DISHES
};
#define FOOD_COUNT (FOOD_DISH0 + CK_DISHES)
#define CK_FRIDGE_FOODS 9
// The stove: 48 ingredients (ck_cook_ingr) on a shelf with a row for each player, six on show at a
// time; the stick scrolls along your row (up and down jump six). Up to four in the pan, B stirs it
// till the heat bar fills, and out comes a dish named by what went in. Everyone playing gets one.
#define CK_COOK_INGR 48
#define CK_COOK_ROW 6
#define CK_COOK_PAN 4
extern const int ck_cook_ingr[CK_COOK_INGR];
// what a pan of n ingredients cooks into: *dish is FOOD_* + 1, name its name
// The stove's dish: its kind (FOOD_* + 1) from the main ingredients, then a name built from
// everything in the pan ("Double meaty cheesy omelette with carrots"), and *extra: a bit per
// ck_cook_ingr that is in the pan but not in the dish's name already, drawn on top as garnish.
void ck_cook_dish(const int *pan, int n, int *dish, uint64_t *extra, char *name, int size);
typedef struct { const char *name; int n; ck_fshape s[6]; } ck_foodart;
extern const ck_foodart ck_foods[FOOD_COUNT];
typedef struct { float x, y, vx, vy; int kind, life; } ck_thrown;
#define CK_THROWN 6
#define CK_LETTERS 6
typedef struct { const char *from; const char *lines[3]; } ck_letter;
extern const ck_letter ck_letters[CK_LETTERS];
#define CK_AQUA_CHESTS 4
extern const float ck_aqua_chest_x[CK_AQUA_CHESTS];
typedef struct {
    ck_hhero ps[2];            // in the house
    ck_hhero gs[2];            // in the game you went into
    int np;
    int last;                  // the player (0 or 1) who last pressed something: the labels follow them
    ck_hbaby babies[CK_HOUSE_BABIES];
    int nbabies;
    float cam;
    int mode, pick;
    float sleepT;
    char msg[56];
    int msgT;
    uint8_t mural[CK_MURAL_W * CK_MURAL_H];
    ck_fish fish[CK_AQUA_FISH];
    ck_shrimp shrimp[CK_AQUA_SHRIMP];
    ck_crab crabs[CK_AQUA_CRABS];
    ck_flake food[CK_AQUA_FOOD];
    int nfood;
    float gcam;                // the camera in a game that scrolls (the aquarium)
    float count, endT, clock;  // cars: the countdown, the time since the winner got home, the race clock
    int win;                   // cars: the first player home (index in gs), or -1
    ck_hhero bot[CK_RACE_CARS - 1];   // cars: computer drivers fill the grid up to four cars
    int nbots, nfin;           // cars: how many bots; how many cars are home
    int track;                 // cars: which track (TRACK_*)
    bool pickTrack;            // cars: choosing the track before the countdown
    uint8_t lit[CK_KEYS_N];    // piano: frames a key stays lit
    ck_note_fx notes[16];
    int nnotes;
    int letter;                // the next letter in the mailbox (HM_LETTER shows letter - 1)
    int fpick[2];              // HM_FRIDGE: each player's cursor
    int cpick[2];              // HM_COOK: each player's cursor on the shelf
    int pan[CK_COOK_PAN], npan;   // HM_COOK: what is in the pan (FOOD_*)
    float heat;                // HM_COOK: 0..100, the dish is done at 100
    int dish;                  // HM_COOK: the cooked dish, FOOD_* + 1 (0: still cooking)
    char dishName[40];
    uint64_t dishExtra;
    ck_thrown thrown[CK_THROWN];
    int nthrown;
    int chestT[CK_AQUA_CHESTS];   // aquarium: frames a chest stays open
    uint8_t chestGot;          // aquarium: chests whose bolts you have had this visit
    bool drums;                // SC_PIANO shows the drum kit instead of the piano
    ck_pinball pin;
    ck_blaster blast;
    ck_boltball bolt;
    ck_jetrun run;
    ck_pond pond;
} ck_house;
int ck_race_rank(const ck_house *H, const ck_hhero *me);   // a car's place in the race right now (1 is in front)
#define CK_SLOTS 3
typedef struct { bool used; int world, bolts; } ck_slot_info;

// ---------------------------------------------------------------------------- the whole game
enum { SC_TITLE, SC_SELECT, SC_MAP, SC_SHOP, SC_LEVEL, SC_JET, SC_PAUSE, SC_INTRO, SC_CUT, SC_MENU,
       SC_HOUSE, SC_PAINT, SC_AQUA, SC_CARS, SC_PIANO, SC_SLOTS, SC_PINBALL, SC_BLAST, SC_FISH, SC_BOLT, SC_JETRUN };
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
    int shopScroll[2];         // the top row each player's half shows
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
int ck_shop_price(const ck_game *G, int item);     // what buying it costs now (a costume: its missing pieces)
bool ck_costume_worn(const ck_game *G, int s, int item);   // player s wears every piece of that costume
int ck_shop_cols(const ck_game *G);                // 12 for one player, 6 a half for two
bool ck_rainbow_skin(const ck_game *G, int slot);   // that player wears the rainbow paint
bool ck_gun_on(const ck_game *G, int slot);        // that player has the bubble gun: Z or L fires it
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
const char *ck_col_name(int col);   // any colour number: past the eight (the rainbow paint) is "Rainbow"
extern const char *const ck_power_names[5];

#endif
