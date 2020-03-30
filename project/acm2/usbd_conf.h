#ifndef USBD_CONF_H
#define USBD_CONF_H

#include "gd32f1x0.h"

#define USBD_CFG_MAX_NUM                   1U
#define USBD_ITF_MAX_NUM                   1U

/* define if low power mode is enabled; it allows entering the device into DEEP_SLEEP mode
   following USB suspend event and wakes up after the USB wakeup event is received. */
//#define USB_DEVICE_LOW_PWR_MODE_SUPPORT

/* USB feature -- Self Powered */
/* #define USBD_SELF_POWERED */

/* endpoint count used by the CDC ACM device */
#define CDC_ACM_CMD_EP                     EP2_IN
#define CDC_ACM_DATA_IN_EP                 EP1_IN
#define CDC_ACM_DATA_OUT_EP                EP3_OUT

#define CDC_ACM_CMD_PACKET_SIZE            8U
#define CDC_ACM_DATA_PACKET_SIZE           64U
#define CDC_ACM_IN_FRAME_INTERVAL          4U

/* endpoint count used by the CDC ACM device */
#define EP_COUNT                           4U
#define USB_STRING_COUNT                   4U

/* base address of the allocation buffer, used for buffer descriptor table and packet memory */
#define BUFFER_ADDRESS                     0x0000U

#endif /* USBD_CONF_H */
