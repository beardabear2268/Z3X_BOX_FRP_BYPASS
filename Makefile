# Variables
CC = gcc
CFLAGS = -Wall -shared -fPIC -I/usr/include/libusb-1.0
LDFLAGS = -lusb-1.0
LIBUSB_OP = libusb_operations.so
LIBZRXBOX = libzrxbox.so

# Targets
all: $(LIBUSB_OP) $(LIBZRXBOX)

$(LIBUSB_OP): src/usb/usb_operations.c src/usb/usb_operations.h
	$(CC) $(CFLAGS) -o src/usb/$(LIBUSB_OP) src/usb/usb_operations.c $(LDFLAGS)

$(LIBZRXBOX): src/zrxbox/zrxbox.c src/zrxbox/zrxbox.h
	$(CC) $(CFLAGS) -o src/zrxbox/$(LIBZRXBOX) src/zrxbox/zrxbox.c $(LDFLAGS)

clean:
	rm -f src/usb/$(LIBUSB_OP) src/zrxbox/$(LIBZRXBOX)