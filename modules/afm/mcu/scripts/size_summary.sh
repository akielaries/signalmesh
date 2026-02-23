#!/usr/bin/env bash

FILE="$1"
SIZE_FILE="${FILE}.size"
SIZE_PREV="${FILE}.size.prev"

# GOWIN M1 memory layout from linker script
# FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 16k
# RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16k
TOTAL_FLASH_BYTES=64000
TOTAL_RAM_BYTES=64000

arm-none-eabi-size "$FILE" > "$SIZE_FILE"

echo ""
echo "=== size summary for $FILE ==="

if [[ -f "$SIZE_PREV" ]]; then
    prev_line=$(tail -n 1 "$SIZE_PREV")
    curr_line=$(tail -n 1 "$SIZE_FILE")

    # arm-none-eabi-size outputs: text data bss dec hex filename
    set -- $prev_line; p_text=$1; p_data=$2; p_bss=$3;
    set -- $curr_line; c_text=$1; c_data=$2; c_bss=$3;

    echo ""
    echo "=== size diff ==="
    printf "  text: %+d bytes\n" $((c_text - p_text))
    printf "  data: %+d bytes\n" $((c_data - p_data))
    printf "   bss: %+d bytes\n" $((c_bss - p_bss))
    printf " total: %+d bytes\n" $(( (c_text+c_data+c_bss) - (p_text+p_data+p_bss) ))
fi

# Show current memory usage with percentage
curr_line=$(tail -n 1 "$SIZE_FILE")
set -- $curr_line; c_text=$1; c_data=$2; c_bss=$3;

USED_FLASH=$((c_text + c_data))
USED_RAM=$((c_data + c_bss))

FLASH_PERC=$(echo "scale=2; $USED_FLASH*100/$TOTAL_FLASH_BYTES" | bc -l 2>/dev/null || echo "N/A")
RAM_PERC=$(echo "scale=2; $USED_RAM*100/$TOTAL_RAM_BYTES" | bc -l 2>/dev/null || echo "N/A")

echo ""
echo "=== current memory usage ==="
printf "  FLASH: %d / %d bytes (%.2f%%)\n" $USED_FLASH $TOTAL_FLASH_BYTES $FLASH_PERC
printf "    RAM: %d / %d bytes (%.2f%%)\n" $USED_RAM $TOTAL_RAM_BYTES $RAM_PERC

cp "$SIZE_FILE" "$SIZE_PREV"
