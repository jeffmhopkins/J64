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

The ROM saves to a 16K EEPROM with three save slots, one per 512-byte block. On a SummerCart64
the save type comes from the ROM header, so there is nothing to set. Each world keeps its own
progress, and a save from before Worlds 2 to 4 carries on as World 1.

> [!IMPORTANT]
> A save from the old 4K build is 512 bytes. Pad it with zeros to 2048 bytes before loading it
> with this build; it becomes slot 1.

| Button | In a level | On the map |
| --- | --- | --- |
| Stick or D-pad | Run, and crouch or slide (down) | Walk the roads |
| A | Jump (hold to jump higher), again in the air for a double jump; hold to glide (jetpack or flapping) | Enter a level, the parts shop or the house; on the exit, go to the next world |
| B or C-left | Action: grab, throw, kick (bubbles with the Bubble Blaster power-up) | |
| Z or L | With the bubble gun bought: shoot bubbles | |
| Start | Pause | Menu: keep playing, change heroes, let player 2 leave, or start a new game |
| L (or Z), R | | In the paint room: the previous or next colour |

After the title you pick one of the three save slots.

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

<a id="house"></a>
## The house

Each world's map starts at the heroes' house: a four-bedroom family house with a garage and a
garden all round, seen from above. You come in at the front gate and walk every way with the
stick; your hero turns to face the way you walk (sideways, toward you, or away with their back to
you, outfits and all). The camera follows you, and indoors it keeps the room you are in on screen. A uses the thing
you stand at, and A anywhere else hops. START leaves any game or screen. Walk back out of the front
gate, or press START, to go back to the map.

> [!NOTE]
> The plan is `web/lib/housetop.js` and the furniture `web/lib/furn34.js`. After changing either,
> re-run `tools/bake_sprites.mjs`: it bakes the N64 furniture and the house picture, and writes the
> same plan into `src/core/ck_house_map.c`.

| Room | What A does there |
| --- | --- |
| Gardens | Out the back, pick an apple off the tree or go fishing at the pond (hold A and let go to cast, up and down move the hook, A when a fish bites, then hold A to reel and let go when the line goes red). Beside the pond is the sandbox, a falling-sand toy: each player moves a cursor and holds A to pour sand (falls and piles up), water (falls and lies flat), acid (eats sand, clay and water) or clay (stays just where you put it, so you can build shelves). L and R pick, C changes the brush size, B rubs out; Sway turns gravity's slow lean left and right (every ten seconds) off and on, and ALL wipes the tank. Out the front, open the mailbox for a big letter; from World 2 on, the signpost by the gate goes back a world |
| Main bedroom | Sleep in the big bed, and save the game to a slot |
| Kitchen | The fridge: pick one of nine foods. The stove: two shelves of twelve ingredients (the stick picks, A adds up to four; a fifth swaps out the last), stir with B till the bar fills, and take the dish. What went in names one of 38 dishes, each with its own look, and everyone playing gets one |
| Bedroom 3 | The easel: fly on the jetpack with no gravity (up rises, down sinks, let go to hover) and paint the wall (hold A); L and R change colour, C changes the brush size (one, two or three cells), and the last colour choice, ALL, wipes the wall with A. B swaps the brush for a sticker: eight of them (C picks), stamped with A in shades of your colour with an outline. Turning round keeps the brush where it was |
| Living room and bedroom 2 | The piano in the living room (the stick picks a key, A plays it, B jumps an octave) or the drum kit in bedroom 2 (every button is a different drum) |
| Living room | The fish tank: dive into a tank six screens wide with a bubble helmet: the Kelp Forest, the Coral Reef, a Shipwreck, the dark Deep Blue and the Rocky Caves, each with its own plants and its own fish (clownfish, angelfish, blue tangs, puffers, jellyfish, eels and rays among them), then a beach. B sprinkles food (a fed fish follows you for a while), opens a treasure chest (four of them, 5 bolts each, once a visit), pokes a crab on the beach, and picks up a shrimp (B again throws it); swimming into a fish boops it |
| Garage | The go-kart: a race seen from behind the car, with a horizon and bends that push you wide; half the screen each for two players, on four tracks (Green Hills, Desert Dash, Snowy Pass, Moon Run) with their own obstacles, boost pads and jump ramps. Pick a track with left and right. Steer across three lanes; A or up goes faster, B or down brakes |
| Family room | The game corner: pinball (B or L flips the left flipper, A or R the right; A fires the ball up the lane) and Bug Blaster (the stick moves your cannon, A or B fires; clear the bugs before they reach the shields). Two more cabinets stand beside them: Bolt Ball and Jet Run |

Food you hold (from the tree, the fridge or the stove) goes in your hand: A eats it and fills your
batteries, B throws it the way you last walked, and a baby that gets to it first munches it.

The babies you have rescued wander about the house and the back garden. Two players share the
screen and can play every room together; they can't walk more than a screen apart. The dining
room, laundry, bathrooms, ensuite, hallway, foyer and nursery are there to explore.

<a id="shop"></a>
## The parts shop

The shop on each map is the parts shop. Bolts buy 36 accessories in five slots: hats, face
(glasses, goggles, a mustache), neck, outfits (with sleeves) and shoes. Each player has a cursor;
A buys an item and puts it on, and on an owned item A puts it on or takes it off. Items in
different slots stack, one per slot. The rainbow cap, disco suit and light-up shoes cycle through
colours.

The bottom row has the rest:

| Item | Cost | What it does |
| --- | --- | --- |
| Robot, Robot ostrich | free | Swap your hero |
| Paint pots | 30 each (the first two are free) | Paint your hero; the select screen offers only paints you own |
| Rainbow paint | 150 | Your hero's whole body keeps cycling through the colours |
| Bubble gun | 100 | Z or L shoots bubbles; B stays grab or kick |
| Super jetpack | 300 | Glides almost level, so you can glide much further |

What you own is shared by both players and saved with the game; a new game from the map menu
empties it.

> [!NOTE]
> On the N64 each accessory is a small sprite drawn over the hero sprite, placed per frame from
> `ACC_AT` in `src/n64/sprites.h`, so it can sit a pixel off where the browser draws it.

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
