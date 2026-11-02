# Clankers

> A co-op side-scrolling platformer for little kids: a robot and a robot ostrich rescue the
> baby bots that a Big Glitch storm blew across four worlds.

> [!NOTE]
> Both the N64 ROM and the browser build play all four worlds: Junkyard Hills, Gear Factory,
> Magnet Mountain and Moon Base, with a short film between worlds. Both read the same level files in `data/`, and the ROM's rules live in one C core (`src/core/`) that a
> later WASM build can share.

<a id="layout"></a>
## Layout

```
roms/clankers/
  data/         levels, world maps and songs as plain text: the one place to change a level
  web/          the browser build: game, map themes, drawing kit, editors
    build.py    builds web/dist/clankers.html and the editors
  src/core/     the game's rules in plain C (no libdragon), unit tested in tests/host/
  src/n64/      the N64 frontend: drawing with rdpq, the synth, input, EEPROM saves
  assets/       sprite strips baked from web/lib (do not edit by hand), Titan One font
  tools/        bake_sprites.mjs, which redraws assets/ from the browser drawing code
```

<a id="build"></a>
## Building the browser version

```sh
python3 roms/clankers/web/build.py
```

Open `roms/clankers/web/dist/clankers.html` in a browser. The level and world editors are
built next to it.

> [!NOTE]
> Every push to `main` also publishes the game to GitHub Pages (`.github/workflows/pages.yml`):
> the game at `https://jeffmhopkins.github.io/J64/`, with `level-editor.html` and
> `world-editor.html` beside it. Pages must be set to deploy from GitHub Actions in the repo's
> settings.

<a id="n64"></a>
## Building and playing the ROM

```sh
make clankers                  # builds dist/clankers.z64
make deploy ROM=clankers       # sends it to a SummerCart64
```

The ROM saves to a 4K EEPROM. On a SummerCart64 the save type comes from the ROM header, so
there is nothing to set. Each world keeps its own progress, and a save from before Worlds 2 to 4
carries on as World 1.

| Button | In a level | On the map |
| --- | --- | --- |
| Stick or D-pad | Run, and crouch or slide (down) | Walk the roads |
| A | Jump (hold to jump higher, again to boost or flap) | Enter a level; on the exit, go to the next world; at home, go back a world |
| B or C-left | Action: grab, throw, shoot | |
| Start | Pause | Enter a level |

<a id="worlds"></a>
## The four worlds

| World | New here | Big Glitch |
| --- | --- | --- |
| 1 Junkyard Hills | crates, levers, tyres, rafts, the jetpack zone | Clanky Crane swings its magnet |
| 2 Gear Factory | conveyor belts that carry you along | The Stamper slams down where you stand |
| 3 Magnet Mountain | low caves to slide through, mine carts over the pits | Rumbleboulder rolls wall to wall |
| 4 Moon Base | low gravity: big floaty jumps | The Sky Hauler shines a beam down |

Every boss is beaten the same way: bounce on its three red buttons. A world's exit plays a
short film (into the factory, up the mine lift, the rocket to the Moon), and Moon Base's exit
plays the parade home and THE END. A or START skips a film after the first second.

In the pause menu, Start or A goes back to the level, B goes back to the map, and Z turns the
CRT scanlines on or off. A second controller plays the ostrich.

<a id="sprites"></a>
## Redrawing the sprites

The ROM's sprites are drawn by the same code as the browser build (`web/lib/`), so a change
there shows up in both. After changing a drawing, bake the strips again and commit them:

```sh
node roms/clankers/tools/bake_sprites.mjs
```

It needs Node and Playwright with Chromium. It writes `assets/*.png`, `assets/rgba16/*.png`
(full-screen pictures, kept at 16 bits to save memory) and `src/n64/sprites.h`.

> [!IMPORTANT]
> The baked strips are checked in, so a ROM build does not need a browser. Bake again whenever
> `web/lib/` changes, or the ROM will draw the old pictures.

<a id="hardware"></a>
## Where the N64 differs from the browser

> [!NOTE]
> The rules are the same: the C core is a line-by-line port of the browser game, and the host
> tests check it. These are the places the N64 draws or sounds a little differently.

- Rainbow power-ups tint the whole hero sheet instead of redrawing it in shifting colours.
- Glows, sparks and puffs are tinted dots, not soft gradients.
- The story before the title cross-fades three baked pictures (night, storm, dawn) instead of
  blending colours, so the sun fades in rather than rising, and the robot in it is always blue.
- The map's river, chimney smoke and factory belts stand still.
- In the films, the babies and heroes fade at the door or the rocket instead of being hidden by it.
- Music and sound effects come from a small software synth playing `data/songs.txt` and the
  browser's sound recipes, so they sound close to, but not exactly like, the browser.
- The game always runs 60 ticks a second, so PAL consoles play at the same speed and drop a
  frame now and then.
- The CRT scanlines (pause menu, Z) are for LCD TVs; on a real CRT, leave them off.

<a id="data"></a>
## Changing levels

See [data/README.md](data/README.md) for the level, world and song file formats.

The font is [Titan One](https://fonts.google.com/specimen/Titan+One) by Rodrigo Fuenzalida,
under the SIL Open Font License ([assets/OFL.txt](assets/OFL.txt)).
