// community HAL config (ChibiOS-Contrib). pulled in by halconf.h when
// HAL_USE_COMMUNITY is TRUE, then by mainline hal.h via hal_community.h.

#ifndef HALCONF_COMMUNITY_H
#define HALCONF_COMMUNITY_H

// usb host stack
#if !defined(HAL_USE_USBH) || defined(__DOXYGEN__)
#define HAL_USE_USBH TRUE
#endif

// class drivers: all off for now - the keyboard just enumerates (milestone 1).
// ADDITIONAL_CLASS_DRIVERS is the hook for a custom MIDI driver later.
#define HAL_USBH_USE_MSD                         FALSE
#define HAL_USBH_USE_FTDI                        FALSE
#define HAL_USBH_USE_AOA                         FALSE
#define HAL_USBH_USE_UVC                         FALSE
#define HAL_USBH_USE_HID                         FALSE
#define HAL_USBH_USE_HUB                         TRUE
#define HAL_USBHHUB_MAX_INSTANCES                1
#define HAL_USBHHUB_MAX_PORTS                    6
// off until the MIDI driver lands - it #includes usbh_additional_class_drivers.h
#define HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS    FALSE

// enumeration timing (defaults from the contrib examples)
#define HAL_USBH_PORT_DEBOUNCE_TIME              200
#define HAL_USBH_PORT_RESET_TIMEOUT              500
#define HAL_USBH_DEVICE_ADDRESS_STABILIZATION    20
#define HAL_USBH_CONTROL_REQUEST_DEFAULT_TIMEOUT OSAL_MS2I(1000)

// debug: routed to the bsp debug uart via usbh_debug_output (defined in the test).
// INFO shows enumeration steps + descriptors; TRACE is too chatty for now.
#define USBH_DEBUG_ENABLE                        TRUE
#define USBH_DEBUG_MULTI_HOST                    FALSE
#define USBH_DEBUG_SINGLE_HOST_SELECTION         USBHD2
#define USBH_DEBUG_BUFFER                        4096
#define USBH_DEBUG_OUTPUT_CALLBACK               usbh_debug_output
#define USBH_DEBUG_ENABLE_TRACE                  FALSE
#define USBH_DEBUG_ENABLE_INFO                   TRUE
#define USBH_DEBUG_ENABLE_WARNINGS               TRUE
#define USBH_DEBUG_ENABLE_ERRORS                 TRUE
#define USBH_LLD_DEBUG_ENABLE_TRACE              FALSE
#define USBH_LLD_DEBUG_ENABLE_INFO               TRUE
#define USBH_LLD_DEBUG_ENABLE_WARNINGS           TRUE
#define USBH_LLD_DEBUG_ENABLE_ERRORS             TRUE

#endif /* HALCONF_COMMUNITY_H */
