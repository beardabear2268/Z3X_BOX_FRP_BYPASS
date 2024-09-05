import serial
import time
import logging

logger = logging.getLogger(__name__)

def send_at_command(port, command):
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        ser.write((command + '\r').encode())
        time.sleep(1)
        response = ser.read_all().decode()
        ser.close()
        logger.debug(f"AT command response: {response}")
        return response
    except Exception as e:
        logger.error(f"Error sending AT command: {e}")
        return None

def enable_adb_with_at(port):
    logger.info(f"Enabling ADB using AT commands on port {port}")
    commands = [
        "AT+COMMAND1",
        "AT+COMMAND2",
        "AT+COMMAND3"
    ]
    for command in commands:
        response = send_at_command(port, command)
        if "ERROR" in response:
            logger.error(f"Command {command} failed")
            return False
    return True