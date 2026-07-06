// UART4 firmware-update receiver for the bootloader.

#ifndef BL_UPDATE_H
#define BL_UPDATE_H

#include <stdint.h>

#include "drivers/qspi_memmap.h"

#ifdef __cplusplus
extern "C" {
#endif

// listen on UART4 for a host HELLO for up to window_ms; if a host connects, run
// a framed update session (HELLO_ACK -> MANIFEST -> DATA.. -> DONE), verify the
// transfer crc, write the image to the target QSPI slot, and reply RESULT.
// returns 1 if a session ran, 0 if no host connected in the window.
// qspi must be initialized in indirect mode (as left by qspi_memmap_init).
int bl_update_run(const qspi_memmap_config_t *qspi, uint32_t window_ms);

#ifdef __cplusplus
}
#endif

#endif // BL_UPDATE_H
