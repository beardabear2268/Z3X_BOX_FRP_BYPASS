import usb.core
import usb.util
from typing import List, Dict, Any
from dataclasses import dataclass, asdict
import logging
import concurrent.futures
import os
import subprocess
import sys
import csv
import json
from ctypes import CDLL, c_uint16, c_char_p

from at_utils import *
from adb_utils import *

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# USB configuration constants
USB_MODEM_CONFIGURATION = 0x2
MAX_RETRIES = 5
RETRY_DELAY = 1.5
TIMEOUT = 10.0

@dataclass
class DeviceInfo:
    vendor_id: int
    product_id: int
    manufacturer: str
    product: str
    serial_number: str

class USBDeviceError(Exception):
    """Custom exception for USB device errors."""
    pass

def compile_shared_library():
    libusb_path = find_library('usb-1.0')
    if not libusb_path:
        logger.error("Error: libusb-1.0 not found. Please install it.")
        sys.exit(1)

    source_file = os.path.join(os.path.dirname(__file__), 'src/usb/usb_operations.c')
    output_file = os.path.join(os.path.dirname(__file__), 'src/usb/libusb_operations.so')

    # Remove the old shared library if it exists
    if os.path.exists(output_file):
        os.remove(output_file)

    compile_command = [
        'gcc',
        '-shared',
        '-fPIC',
        '-o', output_file,
        source_file,
        '-lusb-1.0',
        '-I/usr/include/libusb-1.0'
    ]

    try:
        subprocess.run(compile_command, check=True, stderr=subprocess.PIPE, universal_newlines=True)
        logger.info(f"Shared library {output_file} created successfully.")
    except subprocess.CalledProcessError as e:
        logger.error(f"Error compiling shared library: {e}")
        logger.error(f"Compiler output: {e.stderr}")
        sys.exit(1)

def load_usb_library():
    lib_path = os.path.join(os.path.dirname(__file__), 'src/usb/libusb_operations.so')
    if not os.path.exists(lib_path):
        logger.info("USB operations library not found. Compiling...")
        compile_shared_library()

    try:
        lib = CDLL(lib_path)
        lib.switch_device_to_modem_mode.argtypes = [c_uint16, c_uint16]
        lib.switch_device_to_modem_mode.restype = c_int
        lib.get_error_message.argtypes = [c_int]
        lib.get_error_message.restype = c_char_p
        return lib
    except Exception as e:
        logger.error(f"Failed to load USB operations library: {e}")
        sys.exit(1)

def find_android_devices() -> List[DeviceInfo]:
    """Find all connected Android devices."""
    devices = usb.core.find(find_all=True)
    android_devices = []
    for dev in devices:
        try:
            if dev.manufacturer and "Android" in dev.manufacturer:
                device_info = DeviceInfo(
                    vendor_id=dev.idVendor,
                    product_id=dev.idProduct,
                    manufacturer=dev.manufacturer,
                    product=dev.product,
                    serial_number=dev.serial_number if dev.serial_number else "Unknown"
                )
                android_devices.append(device_info)
        except Exception as e:
            logger.warning(f"Error accessing device information: {str(e)}")
    return android_devices

def android_to_modem_mode(device_info: DeviceInfo) -> Dict[str, Any]:
    """Switch an Android device to modem mode using the C library."""
    usb_lib = load_usb_library()
    try:
        result = usb_lib.switch_device_to_modem_mode(
            c_uint16(device_info.vendor_id),
            c_uint16(device_info.product_id)
        )
        if result == 0:
            logger.info(f"Device {device_info.product} successfully switched to modem mode")
            return {"device": asdict(device_info), "success": True, "message": "Switched to modem mode"}
        else:
            error_message = usb_lib.get_error_message(result).decode('utf-8')
            logger.error(f"Failed to switch device {device_info.product} to modem mode: {error_message}")
            return {"device": asdict(device_info), "success": False, "message": error_message}
    except Exception as e:
        logger.error(f"Error switching device {device_info.product} to modem mode: {str(e)}")
        return {"device": asdict(device_info), "success": False, "message": str(e)}

def switch_device_to_modem_mode_with_timeout(device_info: DeviceInfo) -> Dict[str, Any]:
    """Switch a device to modem mode with a timeout."""
    try:
        with concurrent.futures.ThreadPoolExecutor() as executor:
            future = executor.submit(android_to_modem_mode, device_info)
            result = future.result(timeout=TIMEOUT)
            return result
    except concurrent.futures.TimeoutError:
        logger.error(f"Operation timed out for device {device_info.product}")
        return {"device": asdict(device_info), "success": False, "message": "Operation timed out"}

def switch_devices_to_modem_mode(devices: List[DeviceInfo]) -> List[Dict[str, Any]]:
    """Switch multiple devices to modem mode using a thread pool."""
    with concurrent.futures.ThreadPoolExecutor(max_workers=len(devices)) as executor:
        futures = [executor.submit(switch_device_to_modem_mode_with_timeout, device) for device in devices]
        results = [future.result() for future in concurrent.futures.as_completed(futures)]
    return results

def export_results_to_json(results: List[Dict[str, Any]], filename: str):
    """Export results to a JSON file."""
    with open(filename, 'w') as f:
        json.dump(results, f, indent=2)
    logger.info(f"Results exported to {filename}")

def export_results_to_csv(results: List[Dict[str, Any]], filename: str):
    """Export results to a CSV file."""
    if not results:
        logger.warning("No results to export")
        return

    keys = results[0]['device'].keys()
    with open(filename, 'w', newline='') as f:
        dict_writer = csv.DictWriter(f, fieldnames=['success', 'message', 'frp_bypass'] + list(keys))
        dict_writer.writeheader()
        for result in results:
            row = {'success': result['success'], 'message': result['message'], 'frp_bypass': result.get('frp_bypass', 'N/A')}
            row.update(result['device'])
            dict_writer.writerow(row)
    logger.info(f"Results exported to {filename}")