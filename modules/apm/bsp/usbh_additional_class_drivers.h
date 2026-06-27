// registers custom USBH class drivers. included by hal_usbh.c when
// HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS is enabled.

#ifndef USBH_ADDITIONAL_CLASS_DRIVERS_H
#define USBH_ADDITIONAL_CLASS_DRIVERS_H

#include "hal_usbh.h"

#if HAL_USE_USBH && HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS

extern const usbh_classdriverinfo_t usbhMidiClassDriverInfo;

// comma-separated list of additional class drivers
#define HAL_USBH_ADDITIONAL_CLASS_DRIVERS  \
  &usbhMidiClassDriverInfo,

#endif

#endif /* USBH_ADDITIONAL_CLASS_DRIVERS_H */
