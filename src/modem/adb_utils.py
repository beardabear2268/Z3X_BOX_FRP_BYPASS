import subprocess
import logging
import time
import serial  # Ensure pyserial is installed: pip install pyserial

logger = logging.getLogger(__name__)

def adb_command(device_id, command):
    cmd = f"adb -s {device_id} {command}"
    logger.debug(f"Executing command: {cmd}")
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        logger.error(f"Command failed with error: {result.stderr}")
        return None
    logger.info(result.stdout)
    return result.stdout

def enable_adb(device_id):
    logger.info(f"Enabling ADB for device {device_id}")
    return adb_command(device_id, "usb")

def switch_device_to_modem_mode(device_id):
    logger.info(f"Switching device {device_id} to modem mode")
    return adb_command(device_id, "some_modem_command")

def get_AT_serial(port: str):
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        return ser
    except Exception as e:
        logger.error(f"Error opening serial port {port}: {e}")
        return None

def send_AT_command(ser, command: str) -> str:
    ser.write((command + '\r').encode())
    time.sleep(1)
    response = ser.read_all().decode()
    logger.debug(f"AT command response: {response}")
    return response
