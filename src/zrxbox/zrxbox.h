#ifndef ZRXBOX_H
#define ZRXBOX_H

#include <libusb-1.0/libusb.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define success and error codes
#define ZRXBOX_SUCCESS 0
#define ZRXBOX_ERROR_INIT -1
#define ZRXBOX_ERROR_OPEN_DEVICE -2
#define ZRXBOX_ERROR_SET_CONFIGURATION -3
#define ZRXBOX_ERROR_CLAIM_INTERFACE -4
#define ZRXBOX_ERROR_DETACH_KERNEL -5
#define ZRXBOX_ERROR_RESET_DEVICE -6
#define ZRXBOX_ERROR_SEND_COMMAND -7
#define ZRXBOX_ERROR_RECEIVE_RESPONSE -8
#define ZRXBOX_ERROR_GET_DESCRIPTOR -9

// Function prototypes
int get_zrxbox_device_info(char* manufacturer, char* product, char* serial, int buffer_size);
int zrxbox_perform_operation(const uint8_t *command, int command_length, uint8_t *response, int response_length);

#ifdef __cplusplus
}
#endif

#endif // ZRXBOX_H