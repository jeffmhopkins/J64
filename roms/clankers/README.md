# Clankers

> A co-op side-scrolling platformer for little kids: a robot and a robot ostrich rescue the
> baby bots that a Big Glitch storm blew across four worlds.

> [!NOTE]
> Clankers is not a ROM yet (there is no `Makefile` here, so `make` skips it). Today it runs in
> the browser; the N64 build comes next and will share one C core with the browser build.

<a id="layout"></a>
## Layout

```
roms/clankers/
  data/         levels and world maps as plain text: the one place to change a level
  web/          the browser build: game, map themes, drawing kit, editors
    build.py    builds web/dist/clankers.html and the editors
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

<a id="data"></a>
## Changing levels

See [data/README.md](data/README.md) for the level and world file formats.
