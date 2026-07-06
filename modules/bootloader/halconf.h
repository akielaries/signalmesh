// bootloader HAL config. found before apm/bsp/halconf.h via the -I ordering, so
// hal.h picks this up. it disables the USB host stack (APM enables it for a MIDI
// keyboard; the bootloader must not, or it needs usbh_midi.c), then reuses all
// of APM's HAL settings.
//
// the trick: APM's halconf.h does `#include "halconf_community.h"` (a quoted
// include that resolves next to APM's own file, so our -I override can't win).
// we pre-set USBH off AND pre-define that file's include guard, so when APM's
// halconf.h reaches it the body is skipped and our values stand.

#ifndef HALCONF_H

// usb host stack off for the bootloader
#define HAL_USE_USBH                             FALSE
#define HAL_USBH_USE_MSD                         FALSE
#define HAL_USBH_USE_FTDI                        FALSE
#define HAL_USBH_USE_AOA                         FALSE
#define HAL_USBH_USE_UVC                         FALSE
#define HAL_USBH_USE_HID                         FALSE
#define HAL_USBH_USE_HUB                         FALSE
#define HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS    FALSE

// neutralize apm's community-config include (its body would re-enable USBH)
#define HALCONF_COMMUNITY_H

// pull in apm's full HAL config for everything else (relative to this file)
#include "../apm/bsp/halconf.h"

#endif // HALCONF_H
