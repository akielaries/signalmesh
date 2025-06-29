#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID  0x17cc
#define PRODUCT_ID 0x1750
#define ENDPOINT_IN 0x81  // Update based on lsusb
#define INTERFACE 2       // Update based on lsusb

int main() {
    libusb_device_handle *handle;
    libusb_context *ctx = NULL;
    unsigned char data[64];
    int transferred;
    int r;

    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);

    handle = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
    if (!handle) {
        fprintf(stderr, "Device not found\n");
        return 1;
    }

    if (libusb_kernel_driver_active(handle, INTERFACE)) {
        libusb_detach_kernel_driver(handle, INTERFACE);
    }

    libusb_claim_interface(handle, INTERFACE);

    printf("Reading USB packets...\n");
    while (1) {
        r = libusb_bulk_transfer(handle, ENDPOINT_IN, data, sizeof(data), &transferred, 0);
        if (r == 0 && transferred > 0) {
            printf("Received %d bytes:\n", transferred);
            for (int i = 0; i < transferred; i += 4) {
                if (i + 3 < transferred) {
                    printf("  MIDI packet: %02X %02X %02X %02X\n", data[i], data[i+1], data[i+2], data[i+3]);
                }
            }
        }
    }

    libusb_release_interface(handle, INTERFACE);
    libusb_close(handle);
    libusb_exit(ctx);
    return 0;
}

