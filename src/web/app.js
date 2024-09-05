document.addEventListener('DOMContentLoaded', () => {
				const deviceList = document.getElementById('devices');
				const actionButtons = document.querySelectorAll('#actions button');
				const logContent = document.getElementById('logContent');
				const toast = document.getElementById('toast');
				const refreshDevicesButton = document.getElementById('refreshDevices');

				let selectedDevice = null;

				function updateButtonStates() {
								actionButtons.forEach(button => {
												button.disabled = !selectedDevice;
								});
				}

				function showToast(message, isError = false) {
								toast.textContent = message;
								toast.classList.add('show');
								if (isError) toast.classList.add('error');
								setTimeout(() => {
												toast.classList.remove('show', 'error');
								}, 3000);
				}

				function addLogMessage(message) {
								const timestamp = new Date().toLocaleTimeString();
								logContent.textContent += `[${timestamp}] ${message}\n`;
								logContent.scrollTop = logContent.scrollHeight;
				}

				async function performAction(action, deviceId) {
								try {
												const response = await fetch('api.php', {
																method: 'POST',
																headers: {
																				'Content-Type': 'application/x-www-form-urlencoded',
																},
																body: `action=${action}&device_id=${deviceId}&csrf_token=${csrfToken}`,
												});
												const result = await response.json();
												if (result.success) {
																addLogMessage(`${action} completed successfully`);
																showToast(`${action} completed successfully`);
												} else {
																addLogMessage(`${action} failed: ${result.message}`);
																showToast(`${action} failed: ${result.message}`, true);
												}
								} catch (error) {
												addLogMessage(`Error during ${action}: ${error.message}`);
												showToast(`Error during ${action}`, true);
								}
				}

				deviceList.addEventListener('click', (e) => {
								if (e.target.tagName === 'LI') {
												selectedDevice = e.target.dataset.deviceId;
												document.querySelectorAll('#devices li').forEach(li => li.classList.remove('selected'));
												e.target.classList.add('selected');
												updateButtonStates();
								}
				});

				refreshDevicesButton.addEventListener('click', async () => {
								refreshDevicesButton.classList.add('spinning');
								try {
												const response = await fetch('get_devices.php', {
																method: 'POST',
																headers: {
																				'Content-Type': 'application/x-www-form-urlencoded',
																},
																body: `csrf_token=${csrfToken}`,
												});
												const devices = await response.json();
												deviceList.innerHTML = '';
												devices.forEach(device => {
																const li = document.createElement('li');
																li.dataset.deviceId = device.id;
																li.innerHTML = `<i class="fas fa-mobile-alt"></i> ${device.name}`;
																deviceList.appendChild(li);
												});
												showToast('Devices refreshed successfully');
								} catch (error) {
												showToast('Failed to refresh devices', true);
								} finally {
												refreshDevicesButton.classList.remove('spinning');
								}
				});

				document.getElementById('switchToModem').addEventListener('click', () => performAction('switchToModem', selectedDevice));
				document.getElementById('enableADB').addEventListener('click', () => performAction('enableADB', selectedDevice));
				document.getElementById('frpBypass').addEventListener('click', () => performAction('frpBypass', selectedDevice));
				document.getElementById('manualFrpBypass').addEventListener('click', () => performAction('manualFrpBypass', selectedDevice));
				document.getElementById('checkPOC').addEventListener('click', () => performAction('checkPOC', selectedDevice));
				document.getElementById('checkZRXBox').addEventListener('click', () => performAction('checkZRXBox', selectedDevice));

				document.querySelectorAll('.dropdown-content a').forEach(link => {
								link.addEventListener('click', (e) => {
												e.preventDefault();
												performAction(e.target.id, selectedDevice);
								});
				});
});