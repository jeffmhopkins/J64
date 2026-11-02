# Testing

> Three levels, from fastest to most real: host unit tests, the ares emulator, and the
> console itself through the SummerCart64.

| Level | Command | Runs in CI | Catches |
| --- | --- | --- | --- |
| Host unit tests | `make test` | Yes | Logic bugs in `common/` |
| Emulator | `make run ROM=<name>` | No | Most rendering, input and gameplay bugs |
| Hardware | `make deploy ROM=<name>` | No | Timing, cache and accuracy bugs emulators hide |

<a id="host"></a>
## Host unit tests

Code in `common/` must not include `libdragon.h`, so it compiles with an ordinary C
compiler. [tests/host/](../tests/host/) builds it together with the tests and runs them:

```sh
make test
```

To add tests, create `tests/host/test_<thing>.c` with a `void test_<thing>(void)` function
that uses `EXPECT_EQ_INT`, then call it from `tests/host/test_main.c`.

> [!TIP]
> Keep game rules, maths, state machines and parsers in `common/`, and keep each ROM's
> `src/` a thin layer that reads input, calls into `common/`, and draws. The more lives in
> `common/`, the more CI can test.

> [!NOTE]
> The host tests build with address and undefined-behaviour sanitizers, so an out-of-bounds
> read or an overflow fails `make test` instead of crashing the console later. They also mash
> random buttons through every screen and level, and load damaged saves. `SANITIZE=0` turns the
> sanitizers off.

<a id="ares"></a>
## ares

```sh
make run ROM=hello
```

This builds the ROM and opens it with `tools/run-ares.sh`, which finds ares through `$ARES`,
your `PATH`, or the usual install folders on macOS and Windows.

> [!NOTE]
> In ares, turn on **Settings › Options › Homebrew Mode**. It makes ares stricter about
> things real hardware rejects, and shows `debugf()` output from the ROM in its log.

<a id="selftest"></a>
## Self-playing test builds

`make clankers` also builds `dist/clankers-test.z64`. It's the same game, driven by a scripted
tour (`roms/clankers/src/core/ck_autotest.c`). The tour watches the intro, goes through the title,
player select and the map, pauses, and plays 1-1 with a bot. At each checkpoint it prints a
`CHECK` line and holds still for a second. It ends with `PASS` or `FAIL`, and the same text
shows on a strip at the top of the screen.

The host tests run the same tour, so a tour that can't pass fails `make test` before any
emulator is involved. The test ROM starts from a fresh save and never writes one.

On Windows, `tools/win-test.ps1` runs the test ROM in ares, saves its log and a screenshot of
the ares window at each checkpoint, and exits 0 on `PASS`:

```powershell
powershell -ExecutionPolicy Bypass -File tools\win-test.ps1                       # CI's build of this commit
powershell -ExecutionPolicy Bypass -File tools\win-test.ps1 -Rom dist\clankers-test.z64
```

The results land in `test-out\` (ignored by git).

> [!NOTE]
> Releases leave the `-test` ROMs out.

<a id="hardware"></a>
## SummerCart64

Connect the SummerCart64's USB port to your computer, power on the console, then:

```sh
make deploy ROM=hello
```

`tools/deploy-sc64.sh` runs `sc64deployer upload`, which loads the ROM and boots it, then
`sc64deployer debug`, which streams the ROM's `debugf()` output to your terminal. Press
Ctrl-C to stop the log; the ROM keeps running.

> [!IMPORTANT]
> Logging over USB needs `debug_init_usblog()` near the start of `main()`. The `hello` ROM
> shows how. Extra arguments are passed on to `sc64deployer upload` (such as a save type;
> see `sc64deployer upload --help`): `tools/deploy-sc64.sh dist/hello.z64 <args>`.

For a ROM that doesn't need USB, copy it from `dist/` (or a GitHub Release) to the
SummerCart64's microSD card and start it from the menu.
