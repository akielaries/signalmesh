# bare-metal demo app for the bootloader stage-to-flash path (linked for the
# 0x08100000 exec region). produces demo_app.bin -> wrap with mkupdate ->
# upload with update.bin --file.

CROSS  = arm-none-eabi-
CFLAGS = -mcpu=cortex-m7 -mthumb -Os -ffreestanding -fno-builtin \
         -Wall -Wextra -ffunction-sections -fdata-sections
LDFLAGS = -nostdlib -Wl,--gc-sections -T demo_app.ld

all: demo_app.bin

demo_app.elf: demo_app.c demo_app.ld
	$(CROSS)gcc $(CFLAGS) $(LDFLAGS) -o $@ demo_app.c

demo_app.bin: demo_app.elf
	$(CROSS)objcopy -O binary $< $@
	$(CROSS)size $<

clean:
	rm -f demo_app.elf demo_app.bin
