#!/usr/bin/env bash
# Upload a ROM to a SummerCart64 over USB, boot it, and stream its debug log.
#   Usage: tools/deploy-sc64.sh dist/hello.z64 [extra sc64deployer upload args]
#
# Needs sc64deployer on PATH (or SC64DEPLOYER=/path/to/sc64deployer):
#   https://github.com/Polprzewodnikowy/SummerCart64/releases
# Runs on your machine, not in the container, because it needs the USB device.
# Press Ctrl-C to stop the log; the ROM keeps running on the console.
set -euo pipefail

rom="${1:?usage: deploy-sc64.sh <rom.z64> [upload args]}"
shift
[[ -f "$rom" ]] || { echo "deploy-sc64.sh: $rom not found (build it with make first)" >&2; exit 1; }

sc64="${SC64DEPLOYER:-$(command -v sc64deployer 2>/dev/null || true)}"
if [[ -z "$sc64" ]]; then
  echo "deploy-sc64.sh: sc64deployer not found. Download it from" >&2
  echo "  https://github.com/Polprzewodnikowy/SummerCart64/releases" >&2
  echo "and put it on PATH, or set SC64DEPLOYER=/path/to/sc64deployer" >&2
  exit 1
fi

"$sc64" upload "$@" "$rom"
echo "Uploaded $rom. Streaming debug output (Ctrl-C to stop)..."
exec "$sc64" debug
