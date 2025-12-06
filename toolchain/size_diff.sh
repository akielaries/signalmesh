#!/usr/bin/env bash

FILE="$1"
SIZE_FILE="${FILE}.size"
SIZE_PREV="${FILE}.size.prev"

arm-none-eabi-size "$FILE" > "$SIZE_FILE"

echo ""
echo "=== Size Change ==="

if [[ -f "$SIZE_PREV" ]]; then
    prev_line=$(tail -n 1 "$SIZE_PREV")
    curr_line=$(tail -n 1 "$SIZE_FILE")

    set -- $prev_line; p_text=$1; p_data=$2; p_bss=$3; p_dec=$4
    set -- $curr_line; c_text=$1; c_data=$2; c_bss=$3; c_dec=$4

    printf "  text: %+d bytes\n" $((c_text - p_text))
    printf "  data: %+d bytes\n" $((c_data - p_data))
    printf "   bss: %+d bytes\n" $((c_bss - p_bss))
    printf "  total: %+d bytes\n" $((c_dec - p_dec))
else
    echo "First build. No previous size."
fi

cp "$SIZE_FILE" "$SIZE_PREV"
