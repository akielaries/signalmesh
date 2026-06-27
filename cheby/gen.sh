#!/usr/bin/env bash
# regenerate register interfaces from the cheby sources in this directory.
#   C headers -> modules/apm/bsp            (STM32 firmware, on the include path)
#   verilog   -> modules/acm/acm_fpga/rtl   (FPGA, shared across board targets)
#
# the .yaml files here are the single source of truth; the generated .h/.v are
# build artifacts placed in their consuming module. run: ./gen.sh  (needs cheby)

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
hdr_dst="$root/modules/apm/cheby"
hdl_dst="$root/modules/acm/acm_fpga/rtl"

shopt -s nullglob
for yaml in "$here"/*.yaml; do
  name="$(basename "$yaml" .yaml)"
  printf 'cheby  %-14s -> apm/bsp/%s.h , acm rtl/%s.v\n' "$name" "$name" "$name"
  cheby -i "$yaml" --gen-c                "$hdr_dst/$name.h"
  cheby -i "$yaml" --hdl verilog --gen-hdl "$hdl_dst/$name.v"
done

echo "done."
echo "  headers: $hdr_dst"
echo "  verilog: $hdl_dst"
