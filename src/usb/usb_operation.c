#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#define Z3X_VENDOR_ID 0x0403
#define Z3X_PRODUCT_ID 0x0011
#define INTERFACE_NUMBER 0
#define ENDPOINT_OUT 0x02
#define ENDPOINT_IN 0x81
#define TIMEOUT 5000

#define MAX_BUFFER_SIZE 1024

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
    unsigned char handshake[] = {0x55, 0xAA, 0x5A, 0xA5};
    if (send_command(handle, handshake, sizeof(handshake)) < 0) {
        return -1;
    }
    
    unsigned char response[MAX_BUFFER_SIZE];
    int response_length = receive_response(handle, response, MAX_BUFFER_SIZE);
    if (response_length < 0) {
        return -1;
    }
    
    // Check for expected handshake response
    if (response_length != 4 || memcmp(response, "\xAA\x55\xA5\x5A", 4) != 0) {
        log_message("Unexpected handshake response\n");
        return -1;
    }
    
    return 0;
}

int z3x_switch_to_modem_mode(libusb_device_handle *handle, const char *target_device) {
    // This command structure is hypothetical and needs to be adjusted based on actual Z3X protocol
    unsigned char switch_cmd[64] = {0};
    switch_cmd[0] = 0x01;  // Command identifier for mode switch
    switch_cmd[1] = strlen(target_device);
    memcpy(&switch_cmd[2], target_device, switch_cmd[1]);
    
    if (send_command(handle, switch_cmd, switch_cmd[1] + 2) < 0) {
        return -1;
    }
    
    unsigned char response[MAX_BUFFER_SIZE];
    int response_length = receive_response(handle, response, MAX_BUFFER_SIZE);
    if (response_length < 0) {
        return -1;
    }
    
    // Check for successful mode switch response
    if (response_length < 2 || response[0] != 0x01 || response[1] != 0x00) {
        log_message("Mode switch failed\n");
        return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    libusb_device_handle *z3x_handle = NULL;
    int result;
    char *target_device = NULL;
    char *log_filename = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "t:l:h")) != -1) {
        switch (opt) {
            case 't':
                target_device = optarg;
                break;
            case 'l':
                log_filename = optarg;
                break;
            case 'h':
                printf("Usage: %s -t target_device -l logfile\n", argv[0]);
                return 0;
            default:
                fprintf(stderr, "Usage: %s -t target_device -l logfile\n", argv[0]);
                return 1;
        }
    }

    if (!target_device) {
        fprintf(stderr, "Target device not specified\n");
        return 1;
    }

    if (log_filename) {
        log_file = fopen(log_filename, "w");
        if (!log_file) {
            fprintf(stderr, "Error opening log file: %s\n", log_filename);
            return 1;
        }
    }

    log_message("Starting USB operation for target device: %s\n", target_device);

    result = libusb_init(NULL);
    if (result < 0) {
        log_message("Error initializing libusb: %s\n", libusb_error_name(result));
        return 1;
    }

    z3x_handle = libusb_open_device_with_vid_pid(NULL, Z3X_VENDOR_ID, Z3X_PRODUCT_ID);
    if (z3x_handle == NULL) {
        log_message("Error finding Z3X Box\n");
        libusb_exit(NULL);
        return 1;
    }

    if (libusb_claim_interface(z3x_handle, INTERFACE_NUMBER) < 0) {
        log_message("Error claiming interface\n");
        libusb_close(z3x_handle);
        libusb_exit(NULL);
        return 1;
    }

    // Perform Z3X Box handshake
    log_message("Performing Z3X Box handshake\n");
    if (z3x_handshake(z3x_handle) < 0) {
        log_message("Handshake failed\n");
        goto cleanup;
    }

    // Switch Samsung device to modem mode
    log_message("Attempting to switch Samsung device to modem mode\n");
    if (z3x_switch_to_modem_mode(z3x_handle, target_device) < 0) {
        log_message("Failed to switch device to modem mode\n");
        goto cleanup;
    }

    log_message("Operation completed successfully\n");

cleanup:
    libusb_release_interface(z3x_handle, INTERFACE_NUMBER);
    libusb_close(z3x_handle);
    libusb_exit(NULL);
    if (log_file) {
        fclose(log_file);
    }
    return 0;
}
