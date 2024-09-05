#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <stdarg.h>

#define Z3X_VENDOR_ID 0x0403
#define Z3X_PRODUCT_ID 0x0011
#define SAMSUNG_VENDOR_ID 0x04e8
#define SAMSUNG_PRODUCT_ID 0x685d
#define Z3X_BUS 1
#define Z3X_DEVICE 17
#define SAMSUNG_BUS 1
#define SAMSUNG_DEVICE 18
#define INTERFACE_NUMBER 0
#define ENDPOINT_OUT 0x02
#define ENDPOINT_IN 0x81
#define TIMEOUT 5000

#define MAX_BUFFER_SIZE 4096

FILE *log_file = NULL;

void log_message(const char *format, ...) {
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

    printf("[%s] ", timestamp);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void log_hex_dump(const unsigned char *data, int length) {
    for (int i = 0; i < length; i++) {
        log_message("%02X ", data[i]);
        if ((i + 1) % 16 == 0) log_message("\n");
    }
    if (length % 16 != 0) log_message("\n");
}

libusb_device_handle* open_device_by_bus_and_device(libusb_context *ctx, uint8_t bus, uint8_t device) {
    libusb_device **devs;
    libusb_device *dev;
    libusb_device_handle *handle = NULL;
    ssize_t cnt;
    int i = 0;

    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        log_message("Error getting device list\n");
        return NULL;
    }

    while ((dev = devs[i++]) != NULL) {
        if (libusb_get_bus_number(dev) == bus && libusb_get_device_address(dev) == device) {
            int ret = libusb_open(dev, &handle);
            if (ret < 0) {
                log_message("Error opening device: %s\n", libusb_error_name(ret));
                handle = NULL;
            }
            break;
        }
    }

    libusb_free_device_list(devs, 1);
    return handle;
}

int send_command(libusb_device_handle *handle, const unsigned char *command, int length) {
    int actual_length;
    log_message("Sending command:\n");
    log_hex_dump(command, length);
    
    int result = libusb_bulk_transfer(handle, ENDPOINT_OUT, (unsigned char*)command, length, &actual_length, TIMEOUT);
    if (result < 0) {
        log_message("Error sending command: %s\n", libusb_error_name(result));
        return -1;
    }
    log_message("Sent %d bytes\n", actual_length);
    return 0;
}

int receive_response(libusb_device_handle *handle, unsigned char *response, int max_length) {
    int actual_length;
    int result = libusb_bulk_transfer(handle, ENDPOINT_IN, response, max_length, &actual_length, TIMEOUT);
    if (result < 0) {
        log_message("Error receiving response: %s\n", libusb_error_name(result));
        return -1;
    }
    log_message("Received %d bytes:\n", actual_length);
    log_hex_dump(response, actual_length);
    return actual_length;
}

int z3x_handshake(libusb_device_handle *handle) {
    log_message("Performing Z3X handshake\n");
    unsigned char handshake_cmd[] = {0x55, 0xAA, 0x5A, 0xA5};
    if (send_command(handle, handshake_cmd, sizeof(handshake_cmd)) < 0) {
        return -1;
    }
    unsigned char response[MAX_BUFFER_SIZE];
    int response_length = receive_response(handle, response, MAX_BUFFER_SIZE);
    if (response_length < 4 || memcmp(response, "\xAA\x55\xA5\x5A", 4) != 0) {
        log_message("Invalid handshake response\n");
        return -1;
    }
    log_message("Handshake successful\n");
    return 0;
}

int z3x_switch_to_modem_mode(libusb_device_handle *handle) {
    log_message("Switching Samsung device to modem mode\n");
    unsigned char switch_cmd[] = {0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
                                  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if (send_command(handle, switch_cmd, sizeof(switch_cmd)) < 0) {
        return -1;
    }
    unsigned char response[MAX_BUFFER_SIZE];
    int response_length = receive_response(handle, response, MAX_BUFFER_SIZE);
    if (response_length < 1 || response[0] != 0x00) {
        log_message("Failed to switch to modem mode\n");
        return -1;
    }
    log_message("Successfully switched to modem mode\n");
    return 0;
}

int execute_additional_command(libusb_device_handle *handle, int command_number) {
    log_message("Executing additional command %d\n", command_number);
    unsigned char cmd[MAX_BUFFER_SIZE];
    int cmd_length = 0;
    switch(command_number) {
        case 1: // CASS
            cmd[0] = 0x55; cmd[1] = 0x53; cmd[2] = 0x42; cmd[3] = 0x43;
            cmd_length = 4;
            break;
        case 2: // Change UDID
            cmd[0] = 0x55; cmd[1] = 0x53; cmd[2] = 0x42; cmd[3] = 0x43;
            cmd[4] = 0xE5; cmd[5] = 0xF6; cmd[6] = 0x07; cmd[7] = 0x18;
            cmd_length = 8;
            break;
        case 3: // Disable MTK Secure Boot
            cmd[0] = 0x55; cmd[1] = 0x53; cmd[2] = 0x42; cmd[3] = 0x43;
            cmd[4] = 0x29; cmd[5] = 0x3A; cmd[6] = 0x4B; cmd[7] = 0x5C;
            cmd_length = 8;
            break;
        case 4: // SEC CTRL Status
            cmd[0] = 0x55; cmd[1] = 0x53; cmd[2] = 0x42; cmd[3] = 0x43;
            cmd[4] = 0x6D; cmd[5] = 0x7E; cmd[6] = 0x8F; cmd[7] = 0x90;
            cmd_length = 8;
            break;
        case 5: // Flash FRP.bin
            cmd[0] = 0x55; cmd[1] = 0x53; cmd[2] = 0x42; cmd[3] = 0x43;
            cmd[4] = 0xA1; cmd[5] = 0xB2; cmd[6] = 0xC3; cmd[7] = 0xD4;
            cmd_length = 8;
            break;
        default:
            log_message("Invalid command number\n");
            return -1;
    }
    if (send_command(handle, cmd, cmd_length) < 0) {
        return -1;
    }
    unsigned char response[MAX_BUFFER_SIZE];
    int response_length = receive_response(handle, response, MAX_BUFFER_SIZE);
    if (response_length < 1 || response[0] != 0x00) {
        log_message("Command %d failed\n", command_number);
        return -1;
    }
    log_message("Command %d executed successfully\n", command_number);
    return 0;
}

int main(int argc, char *argv[]) {
    libusb_context *ctx = NULL;
    libusb_device_handle *z3x_handle = NULL;
    int result;
    char *log_filename = NULL;
    int opt;
    int continue_operations = 0;

    while ((opt = getopt(argc, argv, "l:ch")) != -1) {
        switch (opt) {
            case 'l':
                log_filename = optarg;
                break;
            case 'c':
                continue_operations = 1;
                break;
            case 'h':
                printf("Usage: %s -l logfile [-c]\n", argv[0]);
                printf("  -c : Continue with additional operations after modem mode switch\n");
                return 0;
            default:
                fprintf(stderr, "Usage: %s -l logfile [-c]\n", argv[0]);
                return 1;
        }
    }

    if (log_filename) {
        log_file = fopen(log_filename, "w");
        if (!log_file) {
            fprintf(stderr, "Error opening log file: %s\n", log_filename);
            return 1;
        }
    }

    result = libusb_init(&ctx);
    if (result < 0) {
        log_message("Error initializing libusb: %s\n", libusb_error_name(result));
        return 1;
    }

    z3x_handle = open_device_by_bus_and_device(ctx, Z3X_BUS, Z3X_DEVICE);
    if (z3x_handle == NULL) {
        log_message("Error finding Z3X Box\n");
        libusb_exit(ctx);
        return 1;
    }

    if (libusb_claim_interface(z3x_handle, INTERFACE_NUMBER) < 0) {
        log_message("Error claiming interface\n");
        libusb_close(z3x_handle);
        libusb_exit(ctx);
        return 1;
    }

    if (z3x_handshake(z3x_handle) < 0) {
        log_message("Handshake failed\n");
        goto cleanup;
    }

    if (z3x_switch_to_modem_mode(z3x_handle) < 0) {
        log_message("Failed to switch device to modem mode\n");
        goto cleanup;
    }

    if (continue_operations) {
        log_message("Continuing with additional operations\n");
        for (int i = 1; i <= 5; i++) {
            if (execute_additional_command(z3x_handle, i) < 0) {
                log_message("Failed to execute command %d\n", i);
                goto cleanup;
            }
        }
    }

    log_message("All operations completed successfully\n");

cleanup:
    libusb_release_interface(z3x_handle, INTERFACE_NUMBER);
    libusb_close(z3x_handle);
    libusb_exit(ctx);
    if (log_file) {
        fclose(log_file);
    }
    return 0;
}
