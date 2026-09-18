#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ELF="${ROOT}/build/Debug/BASE_PROJECT.elf"

if [[ ! -f "${ELF}" ]]; then
  echo "ELF not found: ${ELF}" >&2
  exit 1
fi

openocd \
  -f "${ROOT}/openocd.cfg" \
  -c "program ${ELF} verify reset exit"
