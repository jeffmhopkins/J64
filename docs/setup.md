# Setup

> How to get a working N64 build environment. Docker does the heavy lifting, so the only
> things your machine needs are git, make, Docker and (for testing) ares and sc64deployer.

<a id="requirements"></a>
## What to install

| Tool | Why | Where |
| --- | --- | --- |
| git | Clone the repo and its submodule | <https://git-scm.com> |
| Docker | Runs the libdragon toolchain | [Docker Desktop](https://www.docker.com/products/docker-desktop/) or Docker Engine |
| make | Runs the top-level commands | Included on macOS and Linux; on Windows use WSL |
| ares | N64 emulator for quick testing | <https://ares-emu.net> |
| sc64deployer | Uploads ROMs to the SummerCart64 over USB | [SummerCart64 releases](https://github.com/Polprzewodnikowy/SummerCart64/releases) |

> [!NOTE]
> **On Windows,** run the `make` commands from WSL (Ubuntu) with Docker Desktop's WSL
> integration turned on. ares and sc64deployer can stay as Windows programs:
> `tools/run-ares.sh` looks in `C:\Program Files\ares`, and you can point
> `SC64DEPLOYER` at `sc64deployer.exe`.

<a id="clone"></a>
## Clone

```sh
git clone --recurse-submodules https://github.com/jeffmhopkins/J64
cd J64
make
```

If you cloned without submodules, run `git submodule update --init`.

<a id="container"></a>
## How the container works

`tools/dev.sh <command>` runs a command inside the dev image, with the repo mounted at
`/work` and running as your own user, so the files it creates belong to you. The top-level
`Makefile` sends every build step through it.

The image is [docker/Dockerfile](../docker/Dockerfile): Ubuntu with libdragon's official
prebuilt MIPS toolchain (amd64 or arm64, so Apple Silicon Macs run it natively), and our
pinned `libdragon/` submodule built and installed on top. `dev.sh` tags the image by the
libdragon commit and a hash of the Dockerfile, so changing either one rebuilds it
automatically.

> [!TIP]
> Already inside the container (devcontainer, Codespaces, CI)? `dev.sh` notices and just
> runs the command directly, so `make` works the same everywhere.

<a id="devcontainer"></a>
## VS Code and Codespaces

Open the repo in VS Code and run **Dev Containers: Reopen in Container**, or create a
Codespace from GitHub. Both use [.devcontainer/devcontainer.json](../.devcontainer/devcontainer.json),
which builds the same image, and they come with C/C++ IntelliSense pointed at the N64
compiler.

> [!WARNING]
> `make run` and `make deploy` need ares and the SummerCart64's USB port, so run them from a
> terminal on your own machine, not from inside the container or a Codespace.

<a id="libdragon"></a>
## Updating libdragon

libdragon is pinned to one commit on its `preview` branch, which has the newest APIs
(rdpq, OpenGL, the asset pipeline) and is what most new homebrew uses.

```sh
git -C libdragon fetch origin preview
git -C libdragon checkout origin/preview
make clean && make && make test   # rebuilds the image, then check everything still builds
git add libdragon && git commit -m "build: bump libdragon"
```

> [!CAUTION]
> Bump libdragon in its own PR. Preview APIs sometimes change, and a separate PR keeps any
> fallout easy to find and to revert.
