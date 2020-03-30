#include "cdc_acm.h"
#include "usbd_int.h"
#include "usbd_conf.h"
#include "gd32f1x0_usart.h"

#define USBD_VID                          0x28E9
#define USBD_PID                          0x018A

uint8_t usb_cmd_buffer[CDC_ACM_CMD_PACKET_SIZE];
uint8_t usart_data_buffer[CDC_ACM_DATA_PACKET_SIZE];
uint8_t usb_usart_buffer[1024];
volatile uint16_t usb_usart_used = 0;
volatile uint16_t usb_usart_tcur = 0;
volatile uint16_t usb_usart_hcur = 0;

volatile uint8_t cdc_altset = 0;
volatile uint8_t cdc_cmd = NO_CMD;
volatile uint8_t cdc_tx = 0;
volatile uint8_t cdc_send_end = 0;
volatile uint32_t cdc_usart = USART0;

uint8_t cdc_acm_sof(usbd_core_handle_struct *pudev);
usbd_int_cb_struct usb_inthandler = { cdc_acm_sof };
usbd_int_cb_struct *usbd_int_fops = &usb_inthandler;

typedef struct
{
    uint32_t dwDTERate;   /* data terminal rate */
    uint8_t  bCharFormat; /* stop bits */
    uint8_t  bParityType; /* parity */
    uint8_t  bDataBits;   /* data bits */
}line_coding_struct;

line_coding_struct linecoding =
{
    115200, /* baud rate     */
    0x00,   /* stop bits - 1 */
    0x00,   /* parity - none */
    0x08    /* num of bits 8 */
};

/* note:it should use the C99 standard when compiling the below codes */
/* USB standard device descriptor */
const usb_descriptor_device_struct device_descriptor =
{
    .Header = 
     {
         .bLength = USB_DEVICE_DESC_SIZE, 
         .bDescriptorType = USB_DESCTYPE_DEVICE
     },
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x02,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = USBD_EP0_MAX_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = USBD_MFC_STR_IDX,
    .iProduct = USBD_PRODUCT_STR_IDX,
    .iSerialNumber = USBD_SERIAL_STR_IDX,
    .bNumberConfigurations = USBD_CFG_MAX_NUM
};

/* USB device configuration descriptor */
const usb_descriptor_configuration_set_struct configuration_descriptor = 
{
    .config = 
    {
        .Header = 
         {
            .bLength = sizeof(usb_descriptor_configuration_struct), 
            .bDescriptorType = USB_DESCTYPE_CONFIGURATION
         },
        .wTotalLength = USB_CDC_ACM_CONFIG_DESC_SIZE,
        .bNumInterfaces = 0x02,
        .bConfigurationValue = 0x01,
        .iConfiguration = 0x00,
        .bmAttributes = 0x80,
        .bMaxPower = 0x32
    },

    .cdc_loopback_interface = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_interface_struct), 
             .bDescriptorType = USB_DESCTYPE_INTERFACE 
         },
        .bInterfaceNumber = 0x00,
        .bAlternateSetting = 0x00,
        .bNumEndpoints = 0x01,
        .bInterfaceClass = 0x02,
        .bInterfaceSubClass = 0x02,
        .bInterfaceProtocol = 0x01,
        .iInterface = 0x00
    },

    .cdc_loopback_header = 
    {
        .Header =
         {
            .bLength = sizeof(usb_descriptor_header_function_struct), 
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
         },
        .bDescriptorSubtype = 0x00,
        .bcdCDC = 0x0110
    },

    .cdc_loopback_call_managment = 
    {
        .Header = 
         {
            .bLength = sizeof(usb_descriptor_call_managment_function_struct), 
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
         },
        .bDescriptorSubtype = 0x01,
        .bmCapabilities = 0x00,
        .bDataInterface = 0x01
    },

    .cdc_loopback_acm = 
    {
        .Header = 
         {
            .bLength = sizeof(usb_descriptor_acm_function_struct), 
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
         },
        .bDescriptorSubtype = 0x02,
        .bmCapabilities = 0x02,
    },

    .cdc_loopback_union = 
    {
        .Header = 
         {
            .bLength = sizeof(usb_descriptor_union_function_struct), 
            .bDescriptorType = USB_DESCTYPE_CS_INTERFACE
         },
        .bDescriptorSubtype = 0x06,
        .bMasterInterface = 0x00,
        .bSlaveInterface0 = 0x01,
    },

    .cdc_loopback_cmd_endpoint = 
    {
        .Header = 
         {
            .bLength = sizeof(usb_descriptor_endpoint_struct), 
            .bDescriptorType = USB_DESCTYPE_ENDPOINT
         },
        .bEndpointAddress = CDC_ACM_CMD_EP,
        .bmAttributes = 0x03,
        .wMaxPacketSize = CDC_ACM_CMD_PACKET_SIZE,
        .bInterval = 0x0A
    },

    .cdc_loopback_data_interface = 
    {
        .Header = 
         {
            .bLength = sizeof(usb_descriptor_interface_struct), 
            .bDescriptorType = USB_DESCTYPE_INTERFACE
         },
        .bInterfaceNumber = 0x01,
        .bAlternateSetting = 0x00,
        .bNumEndpoints = 0x02,
        .bInterfaceClass = 0x0A,
        .bInterfaceSubClass = 0x00,
        .bInterfaceProtocol = 0x00,
        .iInterface = 0x00
    },

    .cdc_loopback_out_endpoint = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_endpoint_struct), 
             .bDescriptorType = USB_DESCTYPE_ENDPOINT 
         },
        .bEndpointAddress = CDC_ACM_DATA_OUT_EP,
        .bmAttributes = 0x02,
        .wMaxPacketSize = CDC_ACM_DATA_PACKET_SIZE,
        .bInterval = 0x00
    },

    .cdc_loopback_in_endpoint = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_endpoint_struct), 
             .bDescriptorType = USB_DESCTYPE_ENDPOINT 
         },
        .bEndpointAddress = CDC_ACM_DATA_IN_EP,
        .bmAttributes = 0x02,
        .wMaxPacketSize = CDC_ACM_DATA_PACKET_SIZE,
        .bInterval = 0x00
    }
};

/* USB language ID Descriptor */
const usb_descriptor_language_id_struct usbd_language_id_desc = 
{
    .Header = 
     {
         .bLength = sizeof(usb_descriptor_language_id_struct), 
         .bDescriptorType = USB_DESCTYPE_STRING
     },
    .wLANGID = ENG_LANGID
};

void *const usbd_strings[] = 
{
    [USBD_LANGID_STR_IDX] = (uint8_t *)&usbd_language_id_desc,
    [USBD_MFC_STR_IDX] = USBD_STRING_DESC("GigaDevice"),
    [USBD_PRODUCT_STR_IDX] = USBD_STRING_DESC("GD32 USB CDC ACM in FS Mode"),
    [USBD_SERIAL_STR_IDX] = USBD_STRING_DESC("GD32F1x0-3.0.0-7z8x9yer")
};

void cdc_acm_usart_configure()
{
    uint32_t stop_type, parity_type, data_type;;
    
    switch (linecoding.bParityType) {
    case 0:
        parity_type = USART_PM_NONE;
        break;
    case 1:
        parity_type = USART_PM_EVEN;
        break;
    case 2: 
        parity_type = USART_PM_ODD;
        break;
    default:
        parity_type = USART_PM_NONE;
        break;
    }
    
    switch (linecoding.bCharFormat) {
    case 0: 
        stop_type = USART_STB_1BIT; 
        break;
    case 1: 
        stop_type = USART_STB_1_5BIT; 
        break;
    case 2: 
        stop_type = USART_STB_2BIT; 
        break;
    default:
        stop_type = USART_STB_1BIT; 
        break;
    }
    
    switch (linecoding.bDataBits) {
    case 0x07:
        data_type = USART_WL_8BIT;
        break;
    case 0x08:
        if (parity_type == USART_PM_NONE)
            data_type = USART_WL_8BIT;
        else
            data_type = USART_WL_9BIT;
        break;
    default:
        data_type = USART_WL_8BIT;
        break;
    }
    
    usart_baudrate_set(cdc_usart, linecoding.dwDTERate);
    usart_parity_config(cdc_usart, parity_type);
    usart_stop_bit_set(cdc_usart, stop_type); 
    usart_word_length_set(cdc_usart, data_type);
    usart_enable(cdc_usart);
}

void cdc_acm_update_linecoding_from_usb_buffer()
{   
    linecoding.dwDTERate = usb_cmd_buffer[0];
    linecoding.dwDTERate |= (uint32_t)usb_cmd_buffer[1] << 8;
    linecoding.dwDTERate |= (uint32_t)usb_cmd_buffer[2] << 16;
    linecoding.dwDTERate |= (uint32_t)usb_cmd_buffer[3] << 24;
    linecoding.bCharFormat = usb_cmd_buffer[4];
    linecoding.bParityType = usb_cmd_buffer[5];
    linecoding.bDataBits = usb_cmd_buffer[6];
}

void cdc_acm_update_linecoding_to_usb_buffer()
{
    usb_cmd_buffer[0] = linecoding.dwDTERate;
    usb_cmd_buffer[1] = linecoding.dwDTERate >> 8;
    usb_cmd_buffer[2] = linecoding.dwDTERate >> 16;
    usb_cmd_buffer[3] = linecoding.dwDTERate >> 24;
    usb_cmd_buffer[4] = linecoding.bCharFormat;
    usb_cmd_buffer[5] = linecoding.bParityType;
    usb_cmd_buffer[6] = linecoding.bDataBits;
}

usbd_status_enum cdc_acm_init(void *pudev, uint8_t config_index)
{
    usbd_ep_init(pudev, ENDP_SNG_BUF, &(configuration_descriptor.cdc_loopback_in_endpoint));
    usbd_ep_init(pudev, ENDP_SNG_BUF, &(configuration_descriptor.cdc_loopback_out_endpoint));
    usbd_ep_init(pudev, ENDP_SNG_BUF, &(configuration_descriptor.cdc_loopback_cmd_endpoint));
    usbd_ep_rx(pudev, CDC_ACM_DATA_OUT_EP, usart_data_buffer, CDC_ACM_DATA_PACKET_SIZE);
    return USBD_OK;
}

usbd_status_enum cdc_acm_deinit(void *pudev, uint8_t config_index)
{
    usbd_ep_deinit(pudev, CDC_ACM_DATA_IN_EP);
    usbd_ep_deinit(pudev, CDC_ACM_DATA_OUT_EP);
    usbd_ep_deinit(pudev, CDC_ACM_CMD_EP);
    return USBD_OK;
}

usbd_status_enum cdc_acm_data_handler(void *pudev, usbd_dir_enum rx_tx, uint8_t ep_id)
{
    if ((USBD_RX == rx_tx) && ((EP0_OUT & 0x7FU) == ep_id)) {
        if (NO_CMD == cdc_cmd)
            return USBD_OK;
        cdc_acm_update_linecoding_from_usb_buffer();
        cdc_acm_usart_configure();
        cdc_cmd = NO_CMD;
    } else if ((USBD_TX == rx_tx) && ((CDC_ACM_DATA_IN_EP & 0x7F) == ep_id)) {
        cdc_acm_data_in(pudev);
    } else if ((USBD_RX == rx_tx) && ((CDC_ACM_DATA_OUT_EP & 0x7FU) == ep_id)) {
        cdc_acm_data_out(pudev);
    } else {
        return USBD_FAIL;
    }
    return USBD_OK;
}

void usb_acm_control(usb_device_req_struct *req)
{
    switch (req->bRequest) {
    case SEND_ENCAPSULATED_COMMAND:
        break;
    case GET_ENCAPSULATED_RESPONSE:
        break;
    case SET_COMM_FEATURE:
        break;
    case GET_COMM_FEATURE:
        break;
    case CLEAR_COMM_FEATURE:
        break;
    case SET_LINE_CODING:
        cdc_acm_update_linecoding_from_usb_buffer();
        cdc_acm_usart_configure();
        break;
    case GET_LINE_CODING:
        cdc_acm_update_linecoding_to_usb_buffer();
        break;
    case SET_CONTROL_LINE_STATE:
        break;
    case SEND_BREAK:
        break;
    default:
        break;
    }
}

usbd_status_enum cdc_acm_req_handler(void *pudev, usb_device_req_struct *req)
{
    switch (req->bmRequestType & USB_REQ_MASK) {
    case USB_CLASS_REQ:
        if (req->wLength) {
            if (req->bmRequestType & 0x80) {
                usb_acm_control(req);
                usbd_ep_tx(pudev, EP0_IN, usb_cmd_buffer, req->wLength);
            } else {
                cdc_cmd = req->bRequest;
                usbd_ep_rx(pudev, EP0_OUT, usb_cmd_buffer, req->wLength);
            }
        } else
            usb_acm_control(req);
        break;
        
    case USB_STANDARD_REQ:
        /* standard device request */
        switch(req->bRequest) {
        case USBREQ_GET_INTERFACE: {
            usbd_ep_tx(pudev, EP0_IN, (uint8_t *)&cdc_altset, 1);
            break; }
            
        case USBREQ_SET_INTERFACE: {
            if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM) {
                cdc_altset = req->wValue;
            } else {
                /* call the error management function (command will be nacked */
                usbd_enum_error(pudev, req);
            }
            break; }
            
        case USBREQ_GET_DESCRIPTOR: {
            uint16_t len = CDC_ACM_DESC_SIZE;
            uint8_t  *pbuf= (uint8_t*)(&configuration_descriptor) + 9;

            if (CDC_ACM_DESC_TYPE == (req->wValue >> 8)) {
                len = MIN(CDC_ACM_DESC_SIZE, req->wLength);
                pbuf = (uint8_t*)(&configuration_descriptor) + 9 + (9 * USBD_ITF_MAX_NUM);
            }
            usbd_ep_tx(pudev, EP0_IN, pbuf, len);
            break; }
            
        default:
            break;
        }
    default:
        break;
    }

    return USBD_OK;
}

void cdc_acm_enable_usart(uint32_t usart_periph)
{
    cdc_usart = usart_periph;
    cdc_acm_usart_configure();
}

void cdc_acm_data_out(void *pudev)
{
    uint16_t rx_len, i;
    
    rx_len = usbd_rx_count_get(pudev, CDC_ACM_DATA_OUT_EP);
    for (i = 0; i < rx_len; i++) {
        usart_data_transmit(cdc_usart, usart_data_buffer[i]);
        while(RESET == usart_flag_get(cdc_usart, USART_FLAG_TBE));
    }
    
    usbd_ep_rx(pudev, CDC_ACM_DATA_OUT_EP, usart_data_buffer, CDC_ACM_DATA_PACKET_SIZE);
}

uint8_t cdc_acm_sof(usbd_core_handle_struct *pudev)
{
    static uint8_t frame_count;
    uint16_t tx_len;
    
    if (frame_count++ != CDC_ACM_IN_FRAME_INTERVAL)
        return USBD_OK;
    frame_count = 0;
    
    if (cdc_tx == 1)
        return USBD_OK;
    
    if (usb_usart_tcur == usb_usart_hcur)
        return USBD_OK; // no data received.
    
    if (usb_usart_hcur > usb_usart_tcur)
        usb_usart_used = sizeof(usb_usart_buffer) - usb_usart_hcur;
    else
        usb_usart_used = usb_usart_tcur - usb_usart_hcur;
    
    if (usb_usart_used > CDC_ACM_DATA_PACKET_SIZE) {
        tx_len = CDC_ACM_DATA_PACKET_SIZE;
    } else {
        tx_len = usb_usart_used;
        if (tx_len == CDC_ACM_DATA_PACKET_SIZE)
            cdc_send_end = 1;
    }
    
    usbd_ep_tx(pudev, CDC_ACM_DATA_IN_EP, usb_usart_buffer + usb_usart_hcur, tx_len);
    usb_usart_hcur += tx_len;
    usb_usart_used -= tx_len;
    
    cdc_tx = 1;
    if (usb_usart_hcur >= sizeof(usb_usart_buffer))
        usb_usart_hcur = 0;
    
    return USBD_OK;
}

void cdc_acm_data_in(void *pudev)
{
    uint16_t tx_len;
    
    if (cdc_tx == 0)
        return;

    if (usb_usart_used == 0) {
        if (cdc_send_end == 1) {
            cdc_send_end = 0;
            usbd_ep_tx(pudev, CDC_ACM_DATA_IN_EP, 0, 0);
        } else {
            cdc_tx = 0;
        }
        return;
    }
    
    if (usb_usart_used > CDC_ACM_DATA_PACKET_SIZE) {
        tx_len = CDC_ACM_DATA_PACKET_SIZE;
    } else {
        tx_len = usb_usart_used;
        if (usb_usart_used == CDC_ACM_DATA_PACKET_SIZE)
            cdc_send_end = 1;
    }
    
    usbd_ep_tx(pudev, CDC_ACM_DATA_IN_EP, usb_usart_buffer + usb_usart_hcur, tx_len);
    usb_usart_hcur += tx_len;
    usb_usart_used -= tx_len;
}

void cdc_acm_isr(void)
{
    if (RESET != usart_interrupt_flag_get(cdc_usart, USART_INT_FLAG_RBNE)) {
        switch (linecoding.bDataBits) {
        case 7:
            usb_usart_buffer[usb_usart_tcur] = usart_data_receive(cdc_usart) & 0x7f;
            break;
        case 8:
            usb_usart_buffer[usb_usart_tcur] = usart_data_receive(cdc_usart);
            break;
        }
        
        usb_usart_tcur++;
        if (usb_usart_tcur >= sizeof(usb_usart_buffer))
            usb_usart_tcur = 0;
    }
    
    if (RESET != usart_interrupt_flag_get(cdc_usart, USART_INT_FLAG_RBNE_ORERR))
        usart_data_receive(cdc_usart);
}
