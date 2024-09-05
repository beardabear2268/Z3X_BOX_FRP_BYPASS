#include "zrxbox.h"
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USB_ENDPOINT_OUT 0x01
#define USB_ENDPOINT_IN 0x81
#define MAX_BUFFER_SIZE 64
#define MAX_RETRIES 5
#define RETRY_DELAY_MS 1000

// Vendor ID and Product ID for Z3X BOX (Future Technology Devices International, Ltd)
#define Z3X_VENDOR_ID  0x0403
#define Z3X_PRODUCT_ID 0x0011

static libusb_context *ctx = NULL;

static int init_libusb() {
				int r = libusb_init(&ctx);
				if (r < 0) {
								fprintf(stderr, "Failed to initialize libusb: %s\n", libusb_error_name(r));
								return ZRXBOX_ERROR_INIT;
				}
				libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
				return ZRXBOX_SUCCESS;
}

static void cleanup_libusb() {
				if (ctx) {
								libusb_exit(ctx);
								ctx = NULL;
				}
}

static libusb_device_handle* open_device(uint16_t vendor_id, uint16_t product_id) {
				libusb_device_handle *handle = libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
				if (!handle) {
								fprintf(stderr, "Failed to open USB device %04x:%04x\n", vendor_id, product_id);
								return NULL;
				}
				return handle;
}

int get_zrxbox_device_info(char* manufacturer, char* product, char* serial, int buffer_size) {
				int result = init_libusb();
				if (result != ZRXBOX_SUCCESS) {
								return result;
				}

				libusb_device_handle *handle = open_device(Z3X_VENDOR_ID, Z3X_PRODUCT_ID);
				if (!handle) {
								cleanup_libusb();
								return ZRXBOX_ERROR_OPEN_DEVICE;
				}

				if (libusb_get_string_descriptor_ascii(handle, 1, (unsigned char*)manufacturer, buffer_size) < 0 ||
								libusb_get_string_descriptor_ascii(handle, 2, (unsigned char*)product, buffer_size) < 0 ||
								libusb_get_string_descriptor_ascii(handle, 3, (unsigned char*)serial, buffer_size) < 0) {
								libusb_close(handle);
								cleanup_libusb();
								return ZRXBOX_ERROR_GET_DESCRIPTOR;
				}

				libusb_close(handle);
				cleanup_libusb();
				return ZRXBOX_SUCCESS;
}

int send_zrxbox_command(libusb_device_handle *handle, const uint8_t *command, int command_length) {
				int transferred;
				int r = libusb_bulk_transfer(handle, USB_ENDPOINT_OUT, (unsigned char *)command, command_length, &transferred, 0);
				if (r != 0 || transferred != command_length) {
								fprintf(stderr, "Error sending command to ZRXBox: %s\n", libusb_error_name(r));
								return ZRXBOX_ERROR_SEND_COMMAND;
				}
				return ZRXBOX_SUCCESS;
}

int receive_zrxbox_response(libusb_device_handle *handle, uint8_t *response, int response_length) {
				int transferred;
				int r = libusb_bulk_transfer(handle, USB_ENDPOINT_IN, response, response_length, &transferred, 0);
				if (r != 0) {
								fprintf(stderr, "Error receiving response from ZRXBox: %s\n", libusb_error_name(r));
								return ZRXBOX_ERROR_RECEIVE_RESPONSE;
				}
				return transferred;
}

int zrxbox_perform_operation(const uint8_t *command, int command_length, uint8_t *response, int response_length) {
				int result = init_libusb();
				if (result != ZRXBOX_SUCCESS) {
								return result;
				}

				libusb_device_handle *handle = open_device(Z3X_VENDOR_ID, Z3X_PRODUCT_ID);
				if (!handle) {
								cleanup_libusb();
								return ZRXBOX_ERROR_OPEN_DEVICE;
				}

				result = send_zrxbox_command(handle, command, command_length);
				if (result != ZRXBOX_SUCCESS) {
								libusb_close(handle);
								cleanup_libusb();
								return result;
				}

				result = receive_zrxbox_response(handle, response, response_length);
				if (result < 0) {
								libusb_close(handle);
								cleanup_libusb();
								return result;
				}

				libusb_close(handle);
				cleanup_libusb();
				return ZRXBOX_SUCCESS;
}