from flask import Flask, render_template, request, jsonify
import subprocess
import os

app = Flask(__name__)

# Ensure the usb_operations binary is in the same directory as main.py
USB_OPERATIONS_BINARY = './usb_operations'

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/execute', methods=['POST'])
def execute_operation():
    vendor_id = request.form.get('vendor_id', '0403')
    product_id = request.form.get('product_id', '0011')
    mode = request.form.get('mode', '0')
    
    command = [
        USB_OPERATIONS_BINARY,
        '-v', vendor_id,
        '-p', product_id,
        '-m', mode,
        '-l', 'operation_log.txt'
    ]
    
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        output = result.stdout
        with open('operation_log.txt', 'r') as log_file:
            log_content = log_file.read()
        return jsonify({'status': 'success', 'output': output, 'log': log_content})
    except subprocess.CalledProcessError as e:
        return jsonify({'status': 'error', 'output': e.stdout, 'error': e.stderr})

if __name__ == '__main__':
    app.run(debug=True)
