document.addEventListener('DOMContentLoaded', () => {
				const deviceSelect = document.getElementById('deviceSelect');
				const enableMITMButton = document.getElementById('enableMITM');
				const disableMITMButton = document.getElementById('disableMITM');
				const mitmStatus = document.getElementById('mitmStatus');
				const mitmLogContent = document.getElementById('mitmLogContent');
				const toast = document.getElementById('toast');

				function updateMITMStatus(status) {
								mitmStatus.textContent = `MITM Status: ${status}`;
								enableMITMButton.disabled = status === 'Enabled';
								disableMITMButton.disabled = status === 'Disabled';
				}

				function showToast(message, isError = false) {
								toast.textContent = message;
								toast.classList.add('show');
								if (isError) {
												toast.classList.add('error');
								} else {
												toast.classList.remove('error');
								}
								setTimeout(() => {
												toast.classList.remove('show');
								}, 3000);
				}

				function addLogMessage(message) {
								const timestamp = new Date().toLocaleTimeString();
								mitmLogContent.textContent += `[${timestamp}] ${message}\n`;
								mitmLogContent.scrollTop = mitmLogContent.scrollHeight;
				}

				async function performMITMAction(action, deviceId) {
								try {
												const response = await fetch('mitm.php', {
																method: 'POST',
																headers: {
																				'Content-Type': 'application/x-www-form-urlencoded',
																},
																body: `action=${action}&device_id=${deviceId}&csrf_token=${csrfToken}`,
												});
												const result = await response.json();
												if (result.success) {
																updateMITMStatus(action === 'enable_mitm' ? 'Enabled' : 'Disabled');
																addLogMessage(`MITM ${action === 'enable_mitm' ? 'enabled' : 'disabled'} successfully`);
																showToast(`MITM ${action === 'enable_mitm' ? 'enabled' : 'disabled'} successfully`);
												} else {
																showToast(`Failed to ${action === 'enable_mitm' ? 'enable' : 'disable'} MITM: ${result.message}`, true);
												}
								} catch (error) {
												showToast(`Error ${action === 'enable_mitm' ? 'enabling' : 'disabling'} MITM`, true);
								}
				}

				deviceSelect.addEventListener('change', () => {
								const selectedDevice = deviceSelect.value;
								enableMITMButton.disabled = !selectedDevice;
								disableMITMButton.disabled = !selectedDevice;
								if (selectedDevice) {
												fetchMITMStatus(selectedDevice);
								} else {
												updateMITMStatus('No device selected');
								}
				});

				async function fetchMITMStatus(deviceId) {
								try {
												const response = await fetch('mitm.php', {
																method: 'POST',
																headers: {
																				'Content-Type': 'application/x-www-form-urlencoded',
																},
																body: `action=get_mitm_status&device_id=${deviceId}&csrf_token=${csrfToken}`,
												});
												const result = await response.json();
												updateMITMStatus(result.status);
								} catch (error) {
												showToast('Failed to fetch MITM status', true);
								}
				}

				enableMITMButton.addEventListener('click', () => {
								const deviceId = deviceSelect.value;
								if (deviceId) {
												performMITMAction('enable_mitm', deviceId);
								}
				});

				disableMITMButton.addEventListener('click', () => {
								const deviceId = deviceSelect.value;
								if (deviceId) {
												performMITMAction('disable_mitm', deviceId);
								}
				});
});