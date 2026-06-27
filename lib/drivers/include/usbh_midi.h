// USB-MIDI host class driver: claims a device's MIDIStreaming interface and
// reads 4-byte USB-MIDI event packets from its bulk IN endpoint.

#ifndef USBH_MIDI_H
#define USBH_MIDI_H

#include "hal_usbh.h"

#if HAL_USE_USBH && HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS

#define USBH_MIDI_MAX_INSTANCES  1
#define USBH_MIDI_BUF_SIZE       64

typedef enum {
  USBHMIDI_STATE_UNINIT = 0,
  USBHMIDI_STATE_STOP   = 1,
  USBHMIDI_STATE_ACTIVE = 2,
  USBHMIDI_STATE_READY  = 3
} usbhmidi_state_t;

typedef struct USBHMIDIDriver USBHMIDIDriver;

struct USBHMIDIDriver {
  _usbh_base_classdriver_data

  usbh_ep_t        epin;
  usbh_urb_t       in_urb;
  uint8_t          ifnum;
  usbhmidi_state_t state;
};

extern USBHMIDIDriver USBHMIDID[USBH_MIDI_MAX_INSTANCES];
extern const usbh_classdriverinfo_t usbhMidiClassDriverInfo;

#endif

#endif /* USBH_MIDI_H */
