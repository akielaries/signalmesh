#!/usr/bin/env bash
# copy the MAIN_COMPUTE_BOARD pin constraints from signalmesh_CAD and turn the
# numbered port names into verilog buses (fmc_d7 -> fmc_d[7])
#
#   usage: scripts/sync_mcb_xdc.sh [path/to/signalmesh_CAD]
set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
cad="${1:-$here/../../../../signalmesh_CAD}"
src="$cad/boards/MAIN_COMPUTE_BOARD/v1_r1/fpga/mcb_pins.xdc"
dst="$here/boards/xc7a100t_mcb/mcb_pins.xdc"

if [ ! -f "$src" ]; then
  echo "not found: $src" >&2
  exit 1
fi

buses='ddr_a|ddr_ba|ddr_dm|ddr_dq|fmc_a|fmc_d|fmc_nbl|fpga_btn|fpga_irq|fpga_led|pmoda_io|pmodb_io'

{
  echo "# synced from signalmesh_CAD by scripts/sync_mcb_xdc.sh, do not edit by hand"
  echo "# source: boards/MAIN_COMPUTE_BOARD/v1_r1/fpga/mcb_pins.xdc ($(git -C "$cad" rev-parse --short HEAD 2>/dev/null || echo unknown))"
  echo
  sed -E "s/get_ports ($buses)([0-9]+)\]/get_ports {\1[\2]}]/" "$src"
} > "$dst"

echo "wrote $dst"
