# CLAUDE.md

> Notes for agents working in J64, an N64 homebrew repo built on libdragon.

- Build and test only through the top-level `Makefile` (`make`, `make test`); it routes
  every step through `tools/dev.sh` and the dev container. Never install the toolchain on
  the host.
- Each ROM lives in `roms/<name>/` and its Makefile includes `mk/rom.mk`. `ROMNAME` must
  match the folder name.
- `common/` holds shared code and must not include `libdragon.h`, so `tests/host/` can unit
  test it with the native compiler. Add a host test for any logic added there.
- libdragon is a submodule pinned on the `preview` branch. Bump it in its own PR.
- `make run` and `make deploy` need ares and a SummerCart64 on the maintainer's machine;
  they can't run in CI or in a cloud session.
- Docs use GitHub alert boxes (`> [!NOTE]`, `> [!TIP]`, `> [!IMPORTANT]`, `> [!WARNING]`,
  `> [!CAUTION]`), a one-line `>` summary under the title, and `<a id>` anchors before
  `##` headings, matching the maintainer's template repo.
