#!/usr/bin/env bash
# Open a ROM in the ares emulator.  Usage: tools/run-ares.sh dist/hello.z64
#
# Finds ares via $ARES, then PATH, then the usual macOS/Windows install spots.
# Get ares from https://ares-emu.net (enable "Homebrew Mode" in its settings
# for stricter, hardware-like behaviour and ISViewer log output).
set -euo pipefail

rom="${1:?usage: run-ares.sh <rom.z64>}"
[[ -f "$rom" ]] || { echo "run-ares.sh: $rom not found (build it with make first)" >&2; exit 1; }

candidates=(
  "${ARES:-}"
  "$(command -v ares 2>/dev/null || true)"
  "/Applications/ares.app/Contents/MacOS/ares"
  "${HOME}/Applications/ares.app/Contents/MacOS/ares"
  "/c/Program Files/ares/ares.exe"
  "/mnt/c/Program Files/ares/ares.exe"
)

for ares in "${candidates[@]}"; do
  if [[ -n "$ares" && -x "$ares" ]]; then
    echo "Launching $rom in $ares"
    exec "$ares" --system "Nintendo 64" "$rom"
  fi
done

echo "run-ares.sh: ares not found. Install it from https://ares-emu.net or set ARES=/path/to/ares" >&2
exit 1
