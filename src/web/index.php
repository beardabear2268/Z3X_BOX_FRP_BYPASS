<?php
session_start();
require_once 'config.php';
require_once 'functions.php';

// Check if the user is logged in
if (!isset($_SESSION['user_id'])) {
				header("Location: login.php");
				exit();
}

// CSRF protection
$csrf_token = bin2hex(random_bytes(32));
$_SESSION['csrf_token'] = $csrf_token;

// Get the list of connected devices
$devices = get_connected_devices();
?>

<!DOCTYPE html>
<html lang="en">
<head>
				<meta charset="UTF-8">
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<title>Samsung FRP Bypass Pro</title>
				<link rel="stylesheet" href="styles.css">
</head>
<body>
				<div class="container">
								<header>
												<h1>Samsung FRP Bypass Pro</h1>
														<p>Welcome, <?php echo htmlspecialchars($_SESSION['username']); ?> | <a href="logout.php">Logout</a> | <?php if ($_SESSION['user_role'] === 'admin'): ?><a href="mitm.php">MITM Panel</a><?php endif; ?></p>
																	</header>
																	<main>
																					<section id="deviceList">
																									<h2>Connected Devices <button id="refreshDevices" title="Refresh Devices"><i class="fas fa-sync-alt"></i></button></h2>
																									<ul id="devices">
																													<?php foreach ($devices as $device): ?>
																																	<li data-device-id="<?php echo htmlspecialchars($device['id']); ?>">
																																					<i class="fas fa-mobile-alt"></i> <?php echo htmlspecialchars($device['name']); ?>
																																	</li>
																													<?php endforeach; ?>
																									</ul>
																					</section>
																					<section id="actions">
																									<h2>Actions</h2>
																									<div class="action-group">
																													<button id="switchToModem" disabled><i class="fas fa-exchange-alt"></i> Switch to Modem Mode</button>
																													<button id="enableADB" disabled><i class="fas fa-terminal"></i> Enable ADB</button>
																													<button id="frpBypass" disabled><i class="fas fa-unlock"></i> FRP Bypass</button>
																													<button id="manualFrpBypass" disabled><i class="fas fa-tools"></i> Manual FRP Bypass</button>
																									</div>
																									<div class="action-group">
																													<button id="checkPOC" disabled><i class="fas fa-check-circle"></i> Check POC Connection</button>
																													<button id="checkZRXBox" disabled><i class="fas fa-box"></i> Check ZRXBox</button>
																									</div>
																									<div class="action-group">
																													<h3>Connection Options</h3>
																													<div class="dropdown">
																																	<button class="dropbtn" disabled><i class="fas fa-network-wired"></i> Connection Options</button>
																																	<div class="dropdown-content">
																																					<a href="#" id="reverseTethering">Reverse Tethering</a>
																																					<a href="#" id="reverseHost">Reverse Host</a>
																																					<a href="#" id="reverseTCP">Reverse TCP</a>
																																					<a href="#" id="usbConnection">USB Connection</a>
																																	</div>
																													</div>
																									</div>
																					</section>
																					<section id="log">
																									<h2>Operation Log</h2>
																									<pre id="logContent"></pre>
																					</section>
																	</main>
																	<footer>
																					<p>&copy; 2023 Samsung FRP Bypass Pro. All rights reserved.</p>
																	</footer>
													</div>
													<div id="toast" class="toast"></div>
													<script src="app.js"></script>
													<script>
																	const csrfToken = "<?php echo htmlspecialchars($csrf_token); ?>";
													</script>
									</body>
									</html>|