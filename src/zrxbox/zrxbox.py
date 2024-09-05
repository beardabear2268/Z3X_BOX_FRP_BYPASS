import ctypes
import os

# Define constants
MAX_BUFFER_SIZE = 256

# Load shared library
lib_path = os.path.join(os.path.dirname(__file__), 'libzrxbox.so')
zrxbox_lib = ctypes.CDLL(lib_path)

# Define function signatures
zrxbox_lib.get_zrxbox_device_info.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
zrxbox_lib.get_zrxbox_device_info.restype = ctypes.c_int

zrxbox_lib.zrxbox_perform_operation.argtypes = [
				ctypes.POINTER(ctypes.c_uint8),
				ctypes.c_int,
				ctypes.POINTER(ctypes.c_uint8),
				ctypes.c_int
]
zrxbox_lib.zrxbox_perform_operation.restype = ctypes.c_int

def get_device_info():
				manufacturer = ctypes.create_string_buffer(MAX_BUFFER_SIZE)
				product = ctypes.create_string_buffer(MAX_BUFFER_SIZE)
				serial = ctypes.create_string_buffer(MAX_BUFFER_SIZE)

				result = zrxbox_lib.get_zrxbox_device_info(manufacturer, product, serial, MAX_BUFFER_SIZE)

				if result != 0:
								raise Exception(f"Failed to get device info with error code {result}")

				return {
								"manufacturer": manufacturer.value.decode('utf-8'),
								"product": product.value.decode('utf-8'),
								"serial": serial.value.decode('utf-8'),
				}

def perform_zrxbox_operation(command, response_length=64):
				command_bytes = (ctypes.c_uint8 * len(command))(*command)
				response = (ctypes.c_uint8 * response_length)()

				result = zrxbox_lib.zrxbox_perform_operation(command_bytes, len(command), response, response_length)

				if result != 0:
								raise Exception(f"ZRXBox operation failed with code {result}")

				return bytes(response[:response_length])