#!/usr/bin/env bash
# Run a command inside the J64 dev container (libdragon toolchain + pinned libdragon).
#
#   tools/dev.sh make            build every ROM
#   tools/dev.sh bash            interactive shell in the container
#
# If you are already inside the container (devcontainer, CI), the command runs directly.
# Set J64_IMAGE to use a prebuilt image instead of building one locally.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -n "${N64_INST:-}" && -f "${N64_INST}/include/n64.mk" ]]; then
  exec "$@"
fi

if ! command -v docker >/dev/null 2>&1; then
  echo "dev.sh: docker is not installed or not on PATH." >&2
  echo "        Install Docker Desktop (or docker engine), or open the repo in the devcontainer." >&2
  exit 1
fi

if [[ ! -f "${ROOT}/libdragon/Makefile" ]]; then
  echo "dev.sh: the libdragon submodule is missing. Run: git submodule update --init" >&2
  exit 1
fi

if [[ -z "${J64_IMAGE:-}" ]]; then
  # Tag the image by the inputs that define it, so bumping libdragon or the
  # Dockerfile rebuilds it automatically and nothing else does.
  libdragon_rev="$(git -C "${ROOT}/libdragon" rev-parse --short=12 HEAD 2>/dev/null || echo unknown)"
  dockerfile_sum="$( (sha256sum "${ROOT}/docker/Dockerfile" 2>/dev/null || shasum -a 256 "${ROOT}/docker/Dockerfile") | cut -c1-12)"
  J64_IMAGE="j64-dev:${libdragon_rev}-${dockerfile_sum}"
  if ! docker image inspect "${J64_IMAGE}" >/dev/null 2>&1; then
    echo "dev.sh: building ${J64_IMAGE} (first run takes a few minutes)..." >&2
    docker build -f "${ROOT}/docker/Dockerfile" -t "${J64_IMAGE}" "${ROOT}" >&2
  fi
fi

tty_flags=()
if [[ -t 0 && -t 1 ]]; then
  tty_flags=(-it)
fi

exec docker run --rm ${tty_flags[@]+"${tty_flags[@]}"} \
  --user "$(id -u):$(id -g)" \
  --env HOME=/tmp \
  --env J64_VERSION="${J64_VERSION:-}" \
  --volume "${ROOT}:/work" \
  --workdir /work \
  "${J64_IMAGE}" "$@"
