# J64

[![Build](https://github.com/jeffmhopkins/J64/actions/workflows/build.yml/badge.svg)](.github/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/jeffmhopkins/J64?include_prereleases&sort=semver)](https://github.com/jeffmhopkins/J64/releases)
[![Platform: Nintendo 64](https://img.shields.io/badge/platform-Nintendo%2064-red)](https://en.wikipedia.org/wiki/Nintendo_64)
[![libdragon: preview](https://img.shields.io/badge/libdragon-preview-orange)](https://github.com/DragonMinded/libdragon/tree/preview)
[![Licence: MIT OR Apache-2.0](https://img.shields.io/badge/licence-MIT%20OR%20Apache--2.0-blue)](LICENSE-MIT)

> Homebrew for the Nintendo 64, built with [libdragon](https://libdragon.dev), tested in
> [ares](https://ares-emu.net) and on real hardware through a
> [SummerCart64](https://summercart64.dev).

> [!TIP]
> **New here?** Read [docs/setup.md](docs/setup.md) first. All you need on your machine is
> Docker and git; the N64 toolchain lives in a container.

> [!IMPORTANT]
> Clone with submodules, because libdragon is pinned as one:
> `git clone --recurse-submodules https://github.com/jeffmhopkins/J64`

<a id="quickstart"></a>
## Quick start

```sh
make                  # build every ROM into dist/
make test             # host unit tests for shared code
make run ROM=hello    # open dist/hello.z64 in ares
make deploy ROM=hello # upload to the SummerCart64 over USB and stream its log
make shell            # a shell inside the dev container
```

The first `make` builds the dev image, which takes a few minutes. After that, builds take
seconds.

<a id="layout"></a>
## Repo layout

```
roms/<name>/          one folder per ROM: Makefile, src/, assets/
common/               C code shared by every ROM (no libdragon, so it can be unit tested)
tests/host/           unit tests for common/, built with the native compiler
libdragon/            git submodule, pinned to a commit on the preview branch
docker/Dockerfile     prebuilt N64 toolchain + the pinned libdragon built in
.devcontainer/        VS Code / Codespaces inside the same image
mk/rom.mk             shared build rules every ROM's Makefile includes
tools/                dev.sh (run in container), run-ares.sh, deploy-sc64.sh
.github/workflows/    build.yml (every push and PR), release.yml (on a v* tag)
docs/                 setup, testing and release guides
```

<a id="docs"></a>
## Docs

| Guide | What it covers |
| --- | --- |
| [Setup](docs/setup.md) | Docker, the devcontainer, editors, and updating libdragon |
| [Testing](docs/testing.md) | Host tests, ares, and running on the SummerCart64 |
| [ROMs](docs/roms.md) | Adding a ROM, assets, and shared code |
| [Releasing](docs/releasing.md) | Tagging a version and what CI publishes |

<a id="roms"></a>
## ROMs

| ROM | What it is |
| --- | --- |
| [hello](roms/hello/) | Starter ROM: prints the build version, moves a box, logs over USB |
| [clankers](roms/clankers/) | Co-op platformer for little kids; the ROM plays World 1, the browser build plays all four |

<a id="licence"></a>
## Licence

Licensed under either of [Apache License, Version 2.0](LICENSE-APACHE) or
[MIT license](LICENSE-MIT), at your option. libdragon is released into the public domain
under the [Unlicense](https://github.com/DragonMinded/libdragon/blob/trunk/LICENSE.md).
