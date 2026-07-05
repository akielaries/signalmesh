// x86 application to update the signalmesh components over USB->UART
//
// run like:
//   ./update.bin --dev /dev/ttyUSB0 --component [APM | ACM] --file foo.bin
//
// test/benchmark mode streams dummy bytes to exercise the STM32 USART3 RX
// benchmark (test_uart3_rx_bench.c):
//   ./update.bin --dev /dev/ttyUSB0 --test
//   ./update.bin --dev /dev/ttyUSB0 --test --baud 2000000 --size 4M

#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <chrono>

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace {

// fnv-1a 32-bit, must match the firmware receiver
// (Fowler–Noll–Vo hash function)
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
uint32_t fnv1a(const std::vector<uint8_t> &data) {
  uint32_t h = 0x811C9DC5u;
  for (uint8_t b : data) {
    h ^= b;
    h *= 0x01000193u;
  }
  return h;
}

// accept plain bytes or K/M suffix (e.g. 4M, 577178, 800K)
size_t parse_size(const std::string &s) {
  if (s.empty()) {
    return 0;
  }
  char suffix = s.back();
  size_t mult = 1;
  std::string num = s;
  if (suffix == 'K' || suffix == 'k') {
    mult = 1024;
    num = s.substr(0, s.size() - 1);
  } else if (suffix == 'M' || suffix == 'm') {
    mult = 1024 * 1024;
    num = s.substr(0, s.size() - 1);
  }
  return static_cast<size_t>(std::stod(num) * static_cast<double>(mult));
}

// map an integer baud to its termios constant; 0 if unsupported
speed_t baud_const(int baud) {
  switch (baud) {
    case 115200:  return B115200;
    case 230400:  return B230400;
    case 460800:  return B460800;
    case 921600:  return B921600;
    case 1000000: return B1000000;
    case 1500000: return B1500000;
    case 2000000: return B2000000;
    case 3000000: return B3000000;
    default:      return 0;
  }
}

// open the serial device raw: 8N1, no flow control, matching the firmware cr3=0
int open_serial(const std::string &dev, int baud) {
  speed_t bc = baud_const(baud);
  if (bc == 0) {
    std::cerr << "unsupported baud " << baud << "\n";
    return -1;
  }

  int fd = ::open(dev.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd < 0) {
    std::cerr << "open " << dev << ": " << std::strerror(errno) << "\n";
    return -1;
  }

  termios tio{};
  if (tcgetattr(fd, &tio) != 0) {
    std::cerr << "tcgetattr: " << std::strerror(errno) << "\n";
    ::close(fd);
    return -1;
  }

  cfmakeraw(&tio);
  tio.c_cflag |= (CLOCAL | CREAD);      // ignore modem lines, enable rx
  tio.c_cflag &= ~CSIZE;
  tio.c_cflag |= CS8;                   // 8 data bits
  tio.c_cflag &= ~CSTOPB;               // 1 stop bit
  tio.c_cflag &= ~PARENB;               // no parity
  tio.c_cflag &= ~CRTSCTS;              // no hw flow control
  tio.c_iflag &= ~(IXON | IXOFF | IXANY); // no sw flow control
  tio.c_cc[VMIN] = 0;
  tio.c_cc[VTIME] = 0;

  cfsetospeed(&tio, bc);
  cfsetispeed(&tio, bc);

  if (tcsetattr(fd, TCSANOW, &tio) != 0) {
    std::cerr << "tcsetattr: " << std::strerror(errno) << "\n";
    ::close(fd);
    return -1;
  }
  return fd;
}

// blast `size` dummy bytes and report host-side throughput + checksum
int run_test(const std::string &dev, int baud, size_t size) {
  std::vector<uint8_t> data(size);
  std::mt19937 rng(0xACE1u); // fixed seed so runs are reproducible
  for (size_t i = 0; i < size; i++) {
    data[i] = static_cast<uint8_t>(rng());
  }
  uint32_t checksum = fnv1a(data);

  std::cout << "dev " << dev << "  baud " << baud << "  size " << size
            << " bytes\n";
  printf("payload fnv1a: 0x%08X\n", checksum);

  int fd = open_serial(dev, baud);
  if (fd < 0) {
    return 1;
  }

  auto t0 = std::chrono::steady_clock::now();
  size_t off = 0;
  while (off < data.size()) {
    ssize_t w = ::write(fd, data.data() + off, data.size() - off);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "write: " << std::strerror(errno) << "\n";
      ::close(fd);
      return 1;
    }
    off += static_cast<size_t>(w);
  }
  tcdrain(fd); // block until all bytes are physically shifted out
  auto t1 = std::chrono::steady_clock::now();
  ::close(fd);

  double dt = std::chrono::duration<double>(t1 - t0).count();
  if (dt <= 0.0) {
    dt = 1e-6;
  }
  double kbps = (static_cast<double>(size) / 1024.0) / dt;
  double mbits = (static_cast<double>(size) * 8.0) / (dt * 1e6);
  double util = (static_cast<double>(size) / dt) / (baud / 10.0) * 100.0;

  printf("sent in  : %.1f ms\n", dt * 1000.0);
  printf("host rate: %.1f KB/s  (%.2f Mbit/s)\n", kbps, mbits);
  printf("line util: %.1f %%\n", util);
  std::cout << "compare payload fnv1a with the firmware console output\n";
  return 0;
}

void usage(const char *prog) {
  std::cout
      << "usage: " << prog << " --dev <path> [options]\n"
      << "  --dev <path>        serial device (e.g. /dev/ttyUSB0)\n"
      << "  --test              send dummy bytes to benchmark STM32 RX\n"
      << "  --baud <n>          baud rate (default 2000000)\n"
      << "  --size <n[K|M]>     test payload size (default 1M)\n"
      << "  --component <name>  APM | ACM   (update mode, not yet implemented)\n"
      << "  --file <path>       image to send (update mode, not yet implemented)\n"
      << "  --help              this message\n";
}

} // namespace

int main(int argc, char **argv) {
  std::string dev;
  std::string component;
  std::string file;
  int baud = 2000000;
  size_t size = 1024 * 1024;
  bool test = false;

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&](const char *name) -> std::string {
      if (i + 1 >= argc) {
        std::cerr << name << " needs an argument\n";
        std::exit(2);
      }
      return argv[++i];
    };

    if (a == "--dev") {
      dev = next("--dev");
    } else if (a == "--test") {
      test = true;
    } else if (a == "--baud") {
      baud = std::stoi(next("--baud"));
    } else if (a == "--size") {
      size = parse_size(next("--size"));
    } else if (a == "--component") {
      component = next("--component");
    } else if (a == "--file") {
      file = next("--file");
    } else if (a == "--help" || a == "-h") {
      usage(argv[0]);
      return 0;
    } else {
      std::cerr << "unknown option: " << a << "\n";
      usage(argv[0]);
      return 2;
    }
  }

  if (dev.empty()) {
    std::cerr << "error: --dev is required\n";
    usage(argv[0]);
    return 2;
  }

  if (test) {
    return run_test(dev, baud, size);
  }

  std::cerr << "update mode not implemented yet - try --test\n";
  return 1;
}
