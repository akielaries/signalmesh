CC 		= g++
BL_INC		= ../../lib/bootloader/include
FLGS 	= -Wall -Werror -I$(BL_INC)
COMPILE		= $(CC) $(FLGS)

# shared with the firmware - single source of truth for the wire format
BL_SRC		= ../../lib/bootloader/src/crc32.c ../../lib/bootloader/src/frame.c


mkupdate:
	$(COMPILE) mkupdate.cpp -o mkupdate.bin

update:
	$(COMPILE) update.cpp custom_baud.c $(BL_SRC) -o update.bin

clean:
	rm *.bin
