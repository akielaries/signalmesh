#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>

static int running = 1;

void handle_sigint(int sig) {
  running = 0;
  //write(STDOUT_FILENO, "\nExiting...\n", 12);
}

speed_t get_baud(int baud) {
  switch (baud) {
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 1000000:
      return B1000000;
    default:
      return B0;
  }
}

int configure_port(int fd, speed_t speed) {
  struct termios tty;

  if (tcgetattr(fd, &tty) != 0) {
    perror("tcgetattr");
    return -1;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag     = 0;
  tty.c_oflag     = 0;
  tty.c_cc[VMIN]  = 1;
  tty.c_cc[VTIME] = 1;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    perror("tcsetattr");
    return -1;
  }

  return 0;
}

void loopback_test(int fd) {
  const char *msg = "loopback test\n";
  write(fd, msg, strlen(msg));

  char buf[256];
  int n = read(fd, buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    printf("Received: %s", buf);
  } else {
    printf("No data received\n");
  }
}

int main(int argc, char *argv[]) {
  const char *device = NULL;
  int baud           = 115200;
  int do_loopback    = 0;

  static struct option long_options[] = {{"device", required_argument, 0, 'd'},
                                         {"baud", required_argument, 0, 'b'},
                                         {"loopback", no_argument, 0, 'l'},
                                         {0, 0, 0, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:b:l", long_options, NULL)) != -1) {
    switch (opt) {
      case 'd':
        device = optarg;
        break;
      case 'b':
        baud = atoi(optarg);
        break;
      case 'l':
        do_loopback = 1;
        break;
      default:
        fprintf(stderr,
                "Usage: %s --device <path> [--baud <rate>] [--loopback]\n",
                argv[0]);
        return 1;
    }
  }

  if (!device) {
    fprintf(stderr, "Error: --device is required.\n");
    return 1;
  }

  speed_t speed = get_baud(baud);
  if (speed == B0) {
    fprintf(stderr, "Unsupported baud rate: %d\n", baud);
    return 1;
  }

  int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  if (configure_port(fd, speed) != 0) {
    close(fd);
    return 1;
  }

  signal(SIGINT, handle_sigint);

  if (do_loopback) {
    printf("Loopbacl on dev: %s\nspeed: %d baud...\n", device, baud);
    loopback_test(fd);
  } else {
    printf("invalid opt\n");
  }

  printf("\nStopping...\n");
  close(fd);
  return 0;
}
