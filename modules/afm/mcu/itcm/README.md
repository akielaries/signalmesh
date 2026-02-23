# Initialized ITCM
This is just the initialized ITCM (instruction tightly coupled memory)
when the Cortex M1 softcore is configured in the IP menu. In this case it just prints the uptime
in seconds and blinks some LEDs via the "RTOS" this project uses...


# Build
This initialized ITCM code is built with the same cmake invocation as the top level application


# Generate ITCM files
I'm sure this could be reverse engineered into a proper linux program but to run the Gowin
make hex util I wrap up the following in a shell script called `gowin_make_hex`:


```
#!/bin/bash

# make_hex.exe executable
MAKE_HEX_EXE="Gowin_EMPU_M1_V2.3/tool/make_hex/bin/make_hex.exe"

wine "$MAKE_HEX_EXE" "$@"
```

```
gowin_make_hex ../../build/bootloader/bin/bootloader.bin
```


