#ifndef USB_OPERATIONS_H
#define USB_OPERATIONS_H

#include <libusb-1.0/libusb.h>

#define USB_OP_SUCCESS 0
#define USB_OP_ERROR_INIT -1
#define USB_OP_ERROR_OPEN_DEVICE -2
#define USB_OP_ERROR_SET_CONFIGURATION -3
#define USB_OP_ERROR_CLAIM_INTERFACE -4
#define USB_OP_ERROR_DETACH_KERNEL -5
#define USB_OP_ERROR_RESET_DEVICE -6
#define USB_OP_ERROR_SEND_PAYLOAD -7
#define USB_OP_ERROR_RECEIVE_RESPONSE -8
#define USB_OP_ERROR_GET_DESCRIPTOR -9

#ifdef __cplusplus
extern "C" {
#endif

int switch_device_to_modem_mode(uint16_t vendor_id, uint16_t product_id);
const char* get_error_message(int error_code);
int get_device_info(uint16_t vendor_id, uint16_t product_id, char* manufacturer, char* product, char* serial, int buffer_size);

#ifdef __cplusplus
}
#endif

#endif // USB_OPERATIONS_H