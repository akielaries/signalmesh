// x86 application to update the signalmesh components over USB->UART
//
// run like:
//   ./update.bin --dev /dev/ttyUSB0 --component [APM | ACM] --file foo.bin
//
// test/benchmark mode streams dummy bytes to exercise the STM32 USART3 RX
// benchmark (test_uart3_rx_bench.c):
//   ./update.bin --dev /dev/ttyUSB0 --test
//   ./update.bin --dev /dev/ttyUSB0 --test --baud 2000000 --size 4M

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include <chrono>

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "bootloader/protocol.h"
#include "bootloader/crc32.h"
#include "bootloader/frame.h"
#include "bootloader/image.h"

#include "custom_baud.h"

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

// open the serial device and configure it raw 8N1, no flow control, at an exact
// baud via termios2 (see custom_baud.c) - any integer, not just the fixed
// Bxxxxxx constants, so 6M/7.5M etc. work
int open_serial(const std::string &dev, int baud) {
  int fd = ::open(dev.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd < 0) {
    std::cerr << "open " << dev << ": " << std::strerror(errno) << "\n";
    return -1;
  }
  if (serial_setup(fd, (unsigned)baud) != 0) {
    std::cerr << "serial_setup " << baud << ": " << std::strerror(errno) << "\n";
    ::close(fd);
    return -1;
  }
  return fd;
}

bool handshake(int fd); // defined below; used by run_test

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

  // framed handshake first so both ends confirm the link before the transfer
  if (!handshake(fd)) {
    ::close(fd);
    return 1;
  }
  usleep(20000); // let the firmware arm its DMA receiver after the ack

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

// send one framed packet, retrying short writes
bool send_frame(int fd, uint8_t type, uint16_t seq, const void *pl,
                uint16_t len) {
  uint8_t tx[8U + BL_MAX_PAYLOAD + 4U];
  size_t n = bl_frame_encode(type, seq, pl, len, tx, sizeof(tx));
  if (n == 0) {
    return false;
  }
  size_t off = 0;
  while (off < n) {
    ssize_t w = ::write(fd, tx + off, n - off);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    off += static_cast<size_t>(w);
  }
  return true;
}

// wait up to timeout_ms for one complete frame; feeds a local parser so a frame
// split across reads still assembles
bool recv_frame(int fd, int timeout_ms, bl_frame &out) {
  bl_frame_rx rx;
  bl_frame_rx_init(&rx);
  auto deadline = std::chrono::steady_clock::now() +
                  std::chrono::milliseconds(timeout_ms);
  uint8_t buf[256];
  while (std::chrono::steady_clock::now() < deadline) {
    ssize_t n = ::read(fd, buf, sizeof(buf));
    if (n < 0) {
      if (errno == EAGAIN || errno == EINTR) {
        usleep(1000);
        continue;
      }
      return false;
    }
    if (n == 0) {
      usleep(1000);
      continue;
    }
    for (ssize_t i = 0; i < n; i++) {
      if (bl_frame_feed(&rx, buf[i]) == BL_FRAME_OK) {
        out = rx.frame;
        return true;
      }
    }
  }
  return false;
}

// HELLO/HELLO_ACK: prove the link and learn the mcu's protocol version
bool handshake(int fd) {
  bl_hello h;
  h.version = BL_PROTO_VERSION;
  h.max_payload = BL_MAX_PAYLOAD;
  if (!send_frame(fd, BL_HELLO, 0, &h, sizeof(h))) {
    std::cerr << "failed to send HELLO\n";
    return false;
  }
  tcdrain(fd);

  bl_frame f;
  if (recv_frame(fd, 1000, f) && f.type == BL_HELLO_ACK) {
    bl_hello ack;
    memcpy(&ack, f.payload, sizeof(ack));
    printf("link established: mcu proto v%u, max payload %u\n", ack.version,
           ack.max_payload);
    return true;
  }
  std::cerr << "no HELLO_ACK (check wiring, baud, and that the framed receiver "
               "firmware is running)\n";
  return false;
}

// full framed transfer of dummy data: HELLO, MANIFEST, streamed DATA, DONE,
// then read back the RESULT verdict. exercises test_fw_receiver.c end to end
int run_stream(const std::string &dev, int baud, size_t size, uint16_t target) {
  std::vector<uint8_t> data(size);
  std::mt19937 rng(0xACE1u);
  for (size_t i = 0; i < size; i++) {
    data[i] = static_cast<uint8_t>(rng());
  }
  uint32_t crc = bl_crc32(data.data(), data.size());

  std::cout << "dev " << dev << "  baud " << baud << "  size " << size
            << " bytes  target " << target << "\n";
  printf("image crc32: 0x%08X\n", crc);

  int fd = open_serial(dev, baud);
  if (fd < 0) {
    return 1;
  }
  if (!handshake(fd)) {
    ::close(fd);
    return 1;
  }
  usleep(20000); // let the firmware arm its stream receiver after the ack

  bl_manifest m;
  m.magic = BL_MANIFEST_MAGIC;
  m.target = target;
  m.reserved = 0;
  m.version = 1;
  m.length = static_cast<uint32_t>(size);
  m.crc32 = crc;
  if (!send_frame(fd, BL_MANIFEST, 0, &m, sizeof(m))) {
    ::close(fd);
    return 1;
  }

  auto t0 = std::chrono::steady_clock::now();
  uint16_t seq = 0;
  size_t off = 0;
  while (off < size) {
    uint16_t chunk =
        static_cast<uint16_t>(std::min<size_t>(BL_MAX_PAYLOAD, size - off));
    if (!send_frame(fd, BL_DATA, seq++, data.data() + off, chunk)) {
      ::close(fd);
      return 1;
    }
    off += chunk;
  }
  send_frame(fd, BL_DONE, seq, nullptr, 0);
  tcdrain(fd);
  auto t1 = std::chrono::steady_clock::now();

  int rc = 1;
  bl_frame f;
  if (recv_frame(fd, 2000, f) && f.type == BL_RESULT) {
    bl_result r;
    memcpy(&r, f.payload, sizeof(r));
    double dt = std::chrono::duration<double>(t1 - t0).count();
    if (dt <= 0.0) {
      dt = 1e-6;
    }
    printf("RESULT status=%u bytes=%u  (%.1f KB/s framed)\n", r.status, r.bytes,
           (static_cast<double>(size) / 1024.0) / dt);
    rc = (r.status == BL_OK) ? 0 : 1;
  } else {
    std::cerr << "no RESULT frame\n";
  }
  ::close(fd);
  return rc;
}

// send a real image blob (built by mkupdate: bl_image_header + app). the target
// is read from the blob's header. streams it as one framed transfer and reports
// the RESULT.
int run_file(const std::string &dev, int baud, const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::cerr << "open " << path << "\n";
    return 1;
  }
  std::vector<uint8_t> blob((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
  if (blob.size() < sizeof(bl_image_header)) {
    std::cerr << path << ": too small to be an image blob\n";
    return 1;
  }
  bl_image_header h;
  memcpy(&h, blob.data(), sizeof(h));
  if (h.magic != BL_IMAGE_MAGIC) {
    std::cerr << path << ": bad magic - run mkupdate first\n";
    return 1;
  }

  uint32_t crc = bl_crc32(blob.data(), blob.size());
  std::cout << "file " << path << "  " << blob.size() << " bytes  target "
            << h.target << "  image " << h.length << "\n";
  printf("blob crc32: 0x%08X\n", crc);

  int fd = open_serial(dev, baud);
  if (fd < 0) {
    return 1;
  }
  if (!handshake(fd)) {
    ::close(fd);
    return 1;
  }
  usleep(20000);

  bl_manifest m;
  m.magic = BL_MANIFEST_MAGIC;
  m.target = h.target;
  m.reserved = 0;
  m.version = h.version;
  m.length = static_cast<uint32_t>(blob.size());
  m.crc32 = crc;
  if (!send_frame(fd, BL_MANIFEST, 0, &m, sizeof(m))) {
    ::close(fd);
    return 1;
  }

  uint16_t seq = 0;
  size_t off = 0;
  while (off < blob.size()) {
    uint16_t chunk =
        static_cast<uint16_t>(std::min<size_t>(BL_MAX_PAYLOAD, blob.size() - off));
    if (!send_frame(fd, BL_DATA, seq++, blob.data() + off, chunk)) {
      ::close(fd);
      return 1;
    }
    off += chunk;
  }
  send_frame(fd, BL_DONE, seq, nullptr, 0);

  int rc = 1;
  bl_frame f;
  if (recv_frame(fd, 4000, f) && f.type == BL_RESULT) {
    bl_result r;
    memcpy(&r, f.payload, sizeof(r));
    printf("RESULT status=%u bytes=%u\n", r.status, r.bytes);
    rc = (r.status == BL_OK) ? 0 : 1;
  } else {
    std::cerr << "no RESULT frame\n";
  }
  ::close(fd);
  return rc;
}

void usage(const char *prog) {
  std::cout
      << "usage: " << prog << " --dev <path> [options]\n"
      << "  --dev <path>        serial device (e.g. /dev/ttyUSB0)\n"
      << "  --hello             framed HELLO handshake, print link status\n"
      << "  --stream            full framed dummy transfer (HELLO..DONE + RESULT)\n"
      << "  --test              raw dummy bytes (pairs with the raw RX benchmark fw)\n"
      << "  --baud <n>          baud rate (default 2000000)\n"
      << "  --size <n[K|M]>     payload size for --test/--stream (default 1M)\n"
      << "  --component <name>  APM | ACM   (update mode, not yet implemented)\n"
      << "  --file <path>       image to send (update mode, not yet implemented)\n"
      << "  --help              this message\n"
      << "\n"
      << "note: --hello/--stream need the framed receiver fw (test_fw_receiver);\n"
      << "      --test needs the raw benchmark fw (test_uart3_rx_bench).\n";
}

} // namespace

int main(int argc, char **argv) {
  std::string dev;
  std::string component;
  std::string file;
  int baud = 2000000;
  size_t size = 1024 * 1024;
  bool test = false;
  bool hello = false;
  bool stream = false;

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
    } else if (a == "--hello") {
      hello = true;
    } else if (a == "--stream") {
      stream = true;
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

  // map --component to a manifest target for framed transfers
  uint16_t target = BL_TARGET_APM_H755;
  if (component == "ACM") {
    target = BL_TARGET_FPGA_GW2AR18;
  }

  if (hello) {
    int fd = open_serial(dev, baud);
    if (fd < 0) {
      return 1;
    }
    bool ok = handshake(fd);
    ::close(fd);
    return ok ? 0 : 1;
  }

  if (!file.empty()) {
    return run_file(dev, baud, file);
  }

  if (stream) {
    return run_stream(dev, baud, size, target);
  }

  if (test) {
    return run_test(dev, baud, size);
  }

  std::cerr << "pick a mode: --file, --hello, --stream, or --test (see --help)\n";
  return 1;
}
