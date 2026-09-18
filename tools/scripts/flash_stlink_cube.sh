#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ELF="${ROOT}/build/Debug/BASE_PROJECT.elf"
CUBE="${CUBE_PROGRAMMER:-/mnt/c/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe}"

if [[ ! -f "${ELF}" ]]; then
  echo "ELF not found: ${ELF}" >&2
  exit 1
fi

if [[ ! -x "${CUBE}" && ! -f "${CUBE}" ]]; then
  echo "STM32_Programmer_CLI not found: ${CUBE}" >&2
  exit 1
fi

# Convert WSL path to Windows path for the Windows CLI.
WIN_ELF="$(wslpath -w "${ELF}")"

"${CUBE}" -c port=SWD freq=4000 -w "${WIN_ELF}" -v -rst
