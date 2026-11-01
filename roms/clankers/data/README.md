# Clankers data

> Levels and world maps as plain text. The browser build and (later) the N64 build both read
> these files, so they are the one place to change a level.

<a id="levels"></a>
## Level files

A level is a few `key: value` header lines, a `---` line, then 15 rows of tiles, one
character per 16 px tile. Things that stand on the ground (Glitchies, tyres, the pad) go in
the row just above the ground they stand on.

| Char | Meaning | Char | Meaning |
| --- | --- | --- | --- |
| `.` | empty | `@` | where the players start |
| `#` | ground | `P` | charging pad with the baby bot (the goal) |
| `=` | girder (solid) | `o` | bolt |
| `-` | plank (jump up through it) | `G` | golden gear |
| `X` | crate | `?` | crate with the level's hidden gear |
| `$` | crate with 3 bolts | `1` `2` `3` `4` | crate with Mega Battery, Rainbow Overdrive, Bubble Blaster, Magnet Helmet |
| `D` | door (opened by the lever) | `L` | lever |
| `T` | tyre spring | `v` | "press down" sign |
| `H` | hatch into the side zone named by `hatch:` | `h` | where the hatch drops you back |
| `b` | Bumblebot (walks) | `s` | Boing Sprocket (hops) |
| `g` | Grumble Crate (follows you; stomp it shut, then kick it, or pick it up with B and throw it) | `k` | swinging hook platform (hangs from this tile) |
| `C` | junk compactor: a 2-tile block that slams down from here to the ground | `F` | checkpoint flag (a lost try restarts here) |
| `r` | raft: a 2-tile platform that rides its track | `~` `|` | raft track, sideways or up and down, starting next to the `r` |

Header keys: `name`, `kind` (`level`, `boss` or `bonus`), `song`, `baby` (the baby bot's
colour as hex), `hatch` (side zone id), `par` (seconds to beat for the speedy star), `time`
(seconds on a race clock; running out costs a try), `boss` (boss type).

> [!TIP]
> Each level's three stars are: finish it, collect every bolt and reboot every Glitchy in one go,
> and finish within `par` seconds. Best times are kept per level.

<a id="worlds"></a>
## World files

`world.txt` in each world folder lists the map's nodes, roads and scenery. See the comments in
[w1/world.txt](w1/world.txt).
