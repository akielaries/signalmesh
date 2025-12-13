#!/usr/bin/env bash

FILE="$1"
SIZE_FILE="${FILE}.size"
SIZE_PREV="${FILE}.size.prev"

# STM32H755xI_M7 memory layout from linker script
# RAM regions (all in bytes):
# ram0: 512KB (0x24000000) - AXI SRAM
# ram1: 256KB (0x30000000) - AHB SRAM1+SRAM2  
# ram2: 32KB (0x30040000) - AHB SRAM3
# ram3: 64KB (0x38000000) - AHB SRAM4
# ram4: 128KB (0x20000000) - DTCM-RAM
# ram5: 64KB (0x00000000) - ITCM-RAM
# ram6: 4KB (0x38800000) - BCKP SRAM

# Total usable RAM for data+bss
TOTAL_RAM_KB=$((512 + 256 + 32 + 64 + 128 + 64 + 4))
TOTAL_RAM_BYTES=$((TOTAL_RAM_KB * 1024))

# Flash regions:
# flash0: 2MB (0x08000000) - Main flash (bank1+bank2)
# flash1: 1MB (0x08100000) - Secondary flash

arm-none-eabi-size "$FILE" > "$SIZE_FILE"

echo ""
echo "=== size diff ==="

if [[ -f "$SIZE_PREV" ]]; then
    prev_line=$(tail -n 1 "$SIZE_PREV")
    curr_line=$(tail -n 1 "$SIZE_FILE")

    # arm-none-eabi-size outputs: text data bss dec hex filename
    set -- $prev_line; p_text=$1; p_data=$2; p_bss=$3; p_dec=$4; p_hex=$5; p_file=$6
    set -- $curr_line; c_text=$1; c_data=$2; c_bss=$3; c_dec=$4; c_hex=$5; c_file=$6

    printf "  text: %+d bytes\n" $((c_text - p_text))
    printf "  data: %+d bytes\n" $((c_data - p_data))
    printf "   bss: %+d bytes\n" $((c_bss - p_bss))
    printf "  total: %+d bytes\n" $((c_dec - p_dec))
    
    # Show current memory usage with percentage
    echo ""
    echo "=== current memory usage ==="
    printf "  text: %d bytes (%.1f KB) [%.1f%% of flash]\n" $c_text $(echo "scale=1; $c_text/1024" | bc -l 2>/dev/null || echo "N/A") $(echo "scale=1; $c_text*100/1048576" | bc -l 2>/dev/null || echo "N/A")
    printf "  data: %d bytes (%.1f KB)\n" $c_data $(echo "scale=1; $c_data/1024" | bc -l 2>/dev/null || echo "N/A")
    printf "   bss: %d bytes (%.1f KB)\n" $c_bss $(echo "scale=1; $c_bss/1024" | bc -l 2>/dev/null || echo "N/A")
    printf "  total: %d bytes (%.1f KB) [%.1f%% of %dKB RAM]\n" $c_dec $(echo "scale=1; $c_dec/1024" | bc -l 2>/dev/null || echo "N/A") $(echo "scale=1; $c_dec*100/$TOTAL_RAM_BYTES" | bc -l 2>/dev/null || echo "N/A") $TOTAL_RAM_KB
    
    # Memory remaining
    remaining_bytes=$((TOTAL_RAM_BYTES - c_dec))
    remaining_kb=$((remaining_bytes / 1024))
    printf "  free: %d bytes (%.1f KB) available\n" $remaining_bytes $(echo "scale=1; $remaining_bytes/1024" | bc -l 2>/dev/null || echo "N/A")
else
    echo "First build. No previous size."
    
    # Show current memory usage on first build
    curr_line=$(tail -n 1 "$SIZE_FILE")
    set -- $curr_line; c_text=$1; c_data=$2; c_bss=$3; c_dec=$4; c_hex=$5; c_file=$6
    
    echo ""
    echo "=== current memory usage ==="
    printf "  text: %d bytes (%.1f KB) [%.1f%% of flash]\n" $c_text $(echo "scale=1; $c_text/1024" | bc -l 2>/dev/null || echo "N/A") $(echo "scale=1; $c_text*100/1048576" | bc -l 2>/dev/null || echo "N/A")
    printf "  data: %d bytes (%.1f KB)\n" $c_data $(echo "scale=1; $c_data/1024" | bc -l 2>/dev/null || echo "N/A")
    printf "   bss: %d bytes (%.1f KB)\n" $c_bss $(echo "scale=1; $c_bss/1024" | bc -l 2>/dev/null || echo "N/A")
    printf "  total: %d bytes (%.1f KB) [%.1f%% of %dKB RAM]\n" $c_dec $(echo "scale=1; $c_dec/1024" | bc -l 2>/dev/null || echo "N/A") $(echo "scale=1; $c_dec*100/$TOTAL_RAM_BYTES" | bc -l 2>/dev/null || echo "N/A") $TOTAL_RAM_KB
    
    # Memory remaining
    remaining_bytes=$((TOTAL_RAM_BYTES - c_dec))
    remaining_kb=$((remaining_bytes / 1024))
    printf "  free: %d bytes (%.1f KB) available\n" $remaining_bytes $(echo "scale=1; $remaining_bytes/1024" | bc -l 2>/dev/null || echo "N/A")
fi

cp "$SIZE_FILE" "$SIZE_PREV"