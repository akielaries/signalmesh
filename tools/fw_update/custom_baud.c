// arbitrary-baud serial setup via termios2 / BOTHER. this file includes the
// kernel header <asm/termbits.h> (which conflicts with glibc's <termios.h>), so
// it is kept separate from update.cpp and does the full port config itself.

#include "custom_baud.h"

#include <asm/termbits.h>
#include <sys/ioctl.h>

int serial_setup(int fd, unsigned baud) {
  struct termios2 tio;
  if (ioctl(fd, TCGETS2, &tio) != 0) {
    return -1;
  }

  // raw mode, 8N1, no flow control (matches the firmware cr3=0)
  tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL |
                   IXON | IXOFF | IXANY);
  tio.c_oflag &= ~OPOST;
  tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS | CBAUD);
  tio.c_cflag |= (CS8 | CLOCAL | CREAD | BOTHER); // BOTHER: use c_ispeed/c_ospeed
  tio.c_ispeed = baud;
  tio.c_ospeed = baud;
  tio.c_cc[VMIN] = 0;
  tio.c_cc[VTIME] = 0;

  return ioctl(fd, TCSETS2, &tio);
}
