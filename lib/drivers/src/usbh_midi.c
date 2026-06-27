// USB-MIDI host class driver. registered via HAL_USBH_ADDITIONAL_CLASS_DRIVERS;
// matches the MIDIStreaming interface (audio class 0x01 / subclass 0x03), opens
// its bulk IN endpoint, and parses 4-byte USB-MIDI event packets into note
// events. modeled on the contrib usbh_custom_class_example.

#include "hal.h"

#if HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS

#include <string.h>
#include "usbh_midi.h"
#include "usbh/internal.h"
#include "usbh/desciter.h"

#if USBH_DEBUG_ENABLE_INFO
#define uinfof(f, ...)  usbDbgPrintf(f, ##__VA_ARGS__)
#define uinfo(f, ...)   usbDbgPuts(f, ##__VA_ARGS__)
#else
#define uinfof(f, ...)  do {} while(0)
#define uinfo(f, ...)   do {} while(0)
#endif
#if USBH_DEBUG_ENABLE_WARNINGS
#define uwarn(f, ...)   usbDbgPuts(f, ##__VA_ARGS__)
#else
#define uwarn(f, ...)   do {} while(0)
#endif

USBHMIDIDriver USBHMIDID[USBH_MIDI_MAX_INSTANCES];

// DMA-capable read buffers, one per instance
static USBH_DEFINE_BUFFER(uint8_t midi_inbuf[USBH_MIDI_MAX_INSTANCES][USBH_MIDI_BUF_SIZE]);

static void _init(void);
static usbh_baseclassdriver_t *_load(usbh_device_t *dev, const uint8_t *descriptor, uint16_t rem);
static void _unload(usbh_baseclassdriver_t *drv);

static const usbh_classdriver_vmt_t class_driver_vmt = {
  _init, _load, _unload
};

const usbh_classdriverinfo_t usbhMidiClassDriverInfo = {
  "MIDI", &class_driver_vmt
};

// completion callback: parse 4-byte USB-MIDI packets, then re-arm the read
static void _in_cb(usbh_urb_t *urb) {
  USBHMIDIDriver *const midip = (USBHMIDIDriver *)urb->userData;

  if (urb->status == USBH_URBSTATUS_OK) {
    const uint8_t *const p = midi_inbuf[midip - USBHMIDID];
    const uint32_t n = urb->actualLength;
    for (uint32_t i = 0; i + 4U <= n; i += 4U) {
      const uint8_t cin = p[i] & 0x0FU;       // code index number
      const uint8_t st  = p[i + 1U];          // MIDI status byte
      const uint8_t d1  = p[i + 2U];
      const uint8_t d2  = p[i + 3U];
      const uint8_t ch  = st & 0x0FU;
      if (cin == 0x9U && d2 != 0U) {
        uinfof("MIDI note ON  ch=%u note=%u vel=%u", ch, d1, d2);
      }
      else if (cin == 0x8U || (cin == 0x9U && d2 == 0U)) {
        uinfof("MIDI note OFF ch=%u note=%u", ch, d1);
      }
      else if (cin != 0U) {
        uinfof("MIDI cin=%X %02x %02x %02x", cin, st, d1, d2);
      }
    }
  }
  else if (urb->status == USBH_URBSTATUS_DISCONNECTED) {
    return;
  }

  usbhURBObjectResetI(&midip->in_urb);
  usbhURBSubmitI(&midip->in_urb);
}

static usbh_baseclassdriver_t *_load(usbh_device_t *dev, const uint8_t *descriptor, uint16_t rem) {
  (void)descriptor;
  (void)rem;

  // composite device: the stack hands us the device descriptor, so scan the full
  // configuration ourselves for a MIDIStreaming interface (audio class 0x01 /
  // midi subclass 0x03) and its bulk IN endpoint.
  if (dev->fullConfigurationDescriptor == NULL) {
    return NULL;
  }

  const usbh_endpoint_descriptor_t *bulk_in = NULL;
  uint8_t midi_ifnum = 0;
  generic_iterator_t icfg;
  if_iterator_t iif;

  cfg_iter_init(&icfg, dev->fullConfigurationDescriptor, dev->basicConfigDesc.wTotalLength);
  for (if_iter_init(&iif, &icfg); iif.valid && (bulk_in == NULL); if_iter_next(&iif)) {
    const usbh_interface_descriptor_t *const ifdesc = (const usbh_interface_descriptor_t *)iif.curr;
    if (ifdesc->bInterfaceClass != 0x01U || ifdesc->bInterfaceSubClass != 0x03U) {
      continue;
    }
    generic_iterator_t iep;
    for (ep_iter_init(&iep, &iif); iep.valid; ep_iter_next(&iep)) {
      const usbh_endpoint_descriptor_t *const epdesc = ep_get(&iep);
      if ((epdesc->bEndpointAddress & 0x80U) && (epdesc->bmAttributes == USBH_EPTYPE_BULK)) {
        bulk_in = epdesc;
        midi_ifnum = ifdesc->bInterfaceNumber;
        break;
      }
    }
  }

  if (bulk_in == NULL) {
    return NULL;                 // no MIDIStreaming bulk IN on this device
  }

  USBHMIDIDriver *midip = NULL;
  uint8_t idx = 0;
  for (uint8_t i = 0; i < USBH_MIDI_MAX_INSTANCES; i++) {
    if (USBHMIDID[i].dev == NULL) {
      midip = &USBHMIDID[i];
      idx = i;
      break;
    }
  }
  if (midip == NULL) {
    uwarn("MIDI: no free instance");
    return NULL;
  }

  midip->ifnum = midi_ifnum;
  usbhEPObjectInit(&midip->epin, dev, bulk_in);
  usbhEPSetName(&midip->epin, "MIDI[BIN ]");
  usbhURBObjectInit(&midip->in_urb, &midip->epin, _in_cb, midip,
                    midi_inbuf[idx], USBH_MIDI_BUF_SIZE);
  usbhEPOpen(&midip->epin);
  usbhURBSubmit(&midip->in_urb);

  midip->state = USBHMIDI_STATE_READY;
  uinfof("MIDI: ready - IF%u bulk IN 0x%02x", midi_ifnum, bulk_in->bEndpointAddress);
  return (usbh_baseclassdriver_t *)midip;
}

static void _unload(usbh_baseclassdriver_t *drv) {
  USBHMIDIDriver *const midip = (USBHMIDIDriver *)drv;
  usbhEPClose(&midip->epin);
  midip->state = USBHMIDI_STATE_STOP;
}

static void _object_init(USBHMIDIDriver *midip) {
  memset(midip, 0, sizeof(*midip));
  midip->state = USBHMIDI_STATE_STOP;
}

static void _init(void) {
  for (uint8_t i = 0; i < USBH_MIDI_MAX_INSTANCES; i++) {
    _object_init(&USBHMIDID[i]);
  }
}

#endif
