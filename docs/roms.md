# ROMs

> Every folder under `roms/` with a `Makefile` is a ROM. `make` finds and builds all of them,
> and CI builds and releases all of them.

<a id="add"></a>
## Adding a ROM

```sh
cp -r roms/hello roms/mygame
```

Then edit `roms/mygame/Makefile`:

```make
ROMNAME       := mygame
N64_ROM_TITLE := "My Game"

include ../../mk/rom.mk
```

`make mygame` builds it, and `make run ROM=mygame` runs it. Add a row to the ROMs table in
the [README](../README.md#roms).

> [!NOTE]
> `ROMNAME` must match the folder name, because the top-level `Makefile` copies
> `roms/<name>/<name>.z64` into `dist/`.

<a id="layout"></a>
## What a ROM folder holds

```
roms/<name>/
  Makefile    ROMNAME, N64_ROM_TITLE, and the include of mk/rom.mk
  src/        every .c file here is compiled in
  assets/     optional; PNGs become sprites in the ROM's filesystem
              (assets/rgba16/*.png are packed as RGBA16, for big pictures)
```

Set `ROM_DATA := <folder>` before the include to copy a folder into the ROM as it is (read it
as `rom:/<folder>/...`), and `ASSET_EXTRA` to list more filesystem outputs that the ROM's own
Makefile has rules for. [roms/clankers/Makefile](../roms/clankers/Makefile) does both.

[mk/rom.mk](../mk/rom.mk) holds the shared rules: it compiles `src/` and `common/src/`,
turns `assets/*.png` into sprites under `rom:/`, and bakes in `J64_VERSION`, the
`git describe` of the build (or the tag, for releases).

> [!TIP]
> Load an asset at run time with `dfs_init(DFS_DEFAULT_LOCATION)` and then
> `sprite_load("rom:/<file>.sprite")`. For other kinds of asset (fonts, audio, models), add
> a rule to the ROM's own Makefile using the libdragon tools (`mkfont`, `audioconv64`,
> `mkmodel`). [libdragon's examples](https://github.com/DragonMinded/libdragon/tree/preview/examples)
> show each one.

<a id="common"></a>
## Shared code

Put code that more than one ROM needs in `common/src/`, with its header in
`common/include/j64/`, and include it as `#include "j64/<name>.h"`. Every ROM links all of
`common/`, and the linker drops what a ROM doesn't use.

> [!IMPORTANT]
> `common/` must not include `libdragon.h`, so that the host tests can build it. Code that
> touches hardware belongs in a ROM's `src/`.
