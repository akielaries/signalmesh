// arbitrary-baud serial setup via Linux termios2. isolated in its own file
// because <asm/termbits.h> conflicts with <termios.h>.

#ifndef CUSTOM_BAUD_H
#define CUSTOM_BAUD_H

#ifdef __cplusplus
extern "C" {
#endif

// configure fd as raw 8N1, no flow control, at an exact baud (any integer, not
// limited to the fixed Bxxxxxx constants). returns 0 on success, -1 on error
int serial_setup(int fd, unsigned baud);

#ifdef __cplusplus
}
#endif

#endif // CUSTOM_BAUD_H
