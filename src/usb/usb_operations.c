#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <time.h>

#define VENDOR_ID 0x0403
#define PRODUCT_ID 0x0011
#define INTERFACE_NUMBER 0
#define ENDPOINT_OUT 0x02
#define ENDPOINT_IN 0x81
#define TIMEOUT 5000

#define USB_OP_SUCCESS 0
#define USB_OP_ERROR_INIT -1
#define USB_OP_ERROR_DEVICE_NOT_FOUND -2
#define USB_OP_ERROR_CLAIM_INTERFACE -3
#define USB_OP_ERROR_BULK_TRANSFER -4

// Payloads
static const unsigned char payload_modem_mode[] = {
    0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char payload_normal_mode[] = {
    0x55, 0x53, 0x42, 0x43, 0x87, 0x65, 0x43, 0x21,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char payload_cass[] = {
    0x55, 0x53, 0x42, 0x43, 0xA1, 0xB2, 0xC3, 0xD4,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char payload_change_udid[] = {
    0x55, 0x53, 0x42, 0x43, 0xE5, 0xF6, 0x07, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char payload_disable_mtk_secure_boot[] = {
    0x55, 0x53, 0x42, 0x43, 0x29, 0x3A, 0x4B, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char payload_sec_ctrl_status[] = {
    0x55, 0x53, 0x42, 0x43, 0x6D, 0x7E, 0x8F, 0x90,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char payload_flash_frp[] = {
    0x55, 0x53, 0x42, 0x43, 0xA1, 0xB2, 0xC3, 0xD4,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int verbose_mode = 0;
static FILE* log_file = NULL;

void log_message(const char* format, ...) {
    va_list args;
    char timestamp[20];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    if (log_file) {
        fprintf(log_file, "[%s] ", timestamp);
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
    if (verbose_mode) {
        printf("[%s] ", timestamp);
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

void log_hex_dump(const unsigned char* data, int length) {
    for (int i = 0; i < length; i++) {
        log_message("%02X ", data[i]);
        if ((i + 1) % 16 == 0) log_message("\n");
    }
    if (length % 16 != 0) log_message("\n");
}

const char* get_error_message(int error_code) {
    switch (error_code) {
        case USB_OP_SUCCESS:
            return "Operation successful";
        case USB_OP_ERROR_INIT:
            return "Failed to initialize libusb";
        case USB_OP_ERROR_DEVICE_NOT_FOUND:
            return "USB device not found";
        case USB_OP_ERROR_CLAIM_INTERFACE:
            return "Failed to claim interface";
        case USB_OP_ERROR_BULK_TRANSFER:
            return "Bulk transfer failed";
        default:
            return "Unknown error";
    }
}

int switch_device_mode(uint16_t vendor_id, uint16_t product_id, const unsigned char* payload, int payload_size) {
    libusb_device_handle *dev_handle = NULL;
    int result;

    result = libusb_init(NULL);
    if (result < 0) {
        log_message("Error initializing libusb: %s\n", libusb_error_name(result));
        return USB_OP_ERROR_INIT;
    }

    dev_handle = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
    if (dev_handle == NULL) {
        log_message("Error finding USB device\n");
        libusb_exit(NULL);
        return USB_OP_ERROR_DEVICE_NOT_FOUND;
    }

    if (libusb_kernel_driver_active(dev_handle, INTERFACE_NUMBER) == 1) {
        log_message("Kernel driver active, detaching...\n");
        if (libusb_detach_kernel_driver(dev_handle, INTERFACE_NUMBER) != 0) {
            log_message("Error detaching kernel driver\n");
            libusb_close(dev_handle);
            libusb_exit(NULL);
            return USB_OP_ERROR_CLAIM_INTERFACE;
        }
    }

    result = libusb_claim_interface(dev_handle, INTERFACE_NUMBER);
    if (result < 0) {
        log_message("Error claiming interface: %s\n", libusb_error_name(result));
        libusb_close(dev_handle);
        libusb_exit(NULL);
        return USB_OP_ERROR_CLAIM_INTERFACE;
    }

    log_message("Sending payload:\n");
    log_hex_dump(payload, payload_size);

    int actual_length;
    result = libusb_bulk_transfer(dev_handle, ENDPOINT_OUT, (unsigned char*)payload, payload_size, &actual_length, TIMEOUT);
    if (result < 0) {
        log_message("Error in bulk transfer: %s\n", libusb_error_name(result));
        libusb_release_interface(dev_handle, INTERFACE_NUMBER);
        libusb_close(dev_handle);
        libusb_exit(NULL);
        return USB_OP_ERROR_BULK_TRANSFER;
    }

    log_message("Sent %d bytes\n", actual_length);

    libusb_release_interface(dev_handle, INTERFACE_NUMBER);
    libusb_close(dev_handle);
    libusb_exit(NULL);

    return USB_OP_SUCCESS;
}

int main(int argc, char *argv[]) {
    uint16_t vendor_id = VENDOR_ID;
    uint16_t product_id = PRODUCT_ID;
    int opt;
    int mode = 0;

    while ((opt = getopt(argc, argv, "v:p:l:m:h")) != -1) {
        switch (opt) {
            case 'v':
                vendor_id = (uint16_t)strtol(optarg, NULL, 16);
                break;
            case 'p':
                product_id = (uint16_t)strtol(optarg, NULL, 16);
                break;
            case 'l':
                log_file = fopen(optarg, "w");
                if (!log_file) {
                    fprintf(stderr, "Error opening log file: %s\n", optarg);
                    return 1;
                }
                break;
            case 'm':
                mode = atoi(optarg);
                break;
            case 'h':
                printf("Usage: %s [-v vendor_id] [-p product_id] [-l log_file] [-m mode] [-h]\n", argv[0]);
                printf("Modes:\n");
                printf("  0: Modem mode (default)\n");
                printf("  1: Normal mode\n");
                printf("  2: CASS\n");
                printf("  3: Change UDID\n");
                printf("  4: Disable MTK Secure Boot\n");
                printf("  5: SEC CTRL Status\n");
                printf("  6: Flash FRP.bin\n");
                return 0;
            default:
                fprintf(stderr, "Usage: %s [-v vendor_id] [-p product_id] [-l log_file] [-m mode] [-h]\n", argv[0]);
                return 1;
        }
    }

    verbose_mode = 1;  // Enable verbose mode by default

    log_message("Starting device operation...\n");
    log_message("Vendor ID: 0x%04x, Product ID: 0x%04x\n", vendor_id, product_id);

    const unsigned char* payload;
    int payload_size;
    const char* mode_name;

    switch (mode) {
        case 0:
            payload = payload_modem_mode;
            payload_size = sizeof(payload_modem_mode);
            mode_name = "Modem";
            break;
        case 1:
            payload = payload_normal_mode;
            payload_size = sizeof(payload_normal_mode);
            mode_name = "Normal";
            break;
        case 2:
            payload = payload_cass;
            payload_size = sizeof(payload_cass);
            mode_name = "CASS";
            break;
        case 3:
            payload = payload_change_udid;
            payload_size = sizeof(payload_change_udid);
            mode_name = "Change UDID";
            break;
        case 4:
            payload = payload_disable_mtk_secure_boot;
            payload_size = sizeof(payload_disable_mtk_secure_boot);
            mode_name = "Disable MTK Secure Boot";
            break;
        case 5:
            payload = payload_sec_ctrl_status;
            payload_size = sizeof(payload_sec_ctrl_status);
            mode_name = "SEC CTRL Status";
            break;
        case 6:
            payload = payload_flash_frp;
            payload_size = sizeof(payload_flash_frp);
            mode_name = "Flash FRP.bin";
            break;
        default:
            log_message("Invalid mode selected\n");
            if (log_file) fclose(log_file);
            return 1;
    }

    log_message("Mode: %s\n", mode_name);

    int result = switch_device_mode(vendor_id, product_id, payload, payload_size);
    if (result != USB_OP_SUCCESS) {
        log_message("Error: %s\n", get_error_message(result));
        if (log_file) fclose(log_file);
        return result;
    }

    log_message("Operation '%s' completed successfully.\n", mode_name);
    if (log_file) fclose(log_file);
    return 0;
}
