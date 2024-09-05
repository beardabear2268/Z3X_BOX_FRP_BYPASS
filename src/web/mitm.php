<?php
session_start();
require_once 'config.php';
require_once 'functions.php';

// Check if the user is logged in and has admin privileges
if (!isset($_SESSION['user_id']) || $_SESSION['user_role'] !== 'admin') {
				header("HTTP/1.1 403 Forbidden");
				echo "Access Denied";
				exit();
}

// CSRF protection
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
				if (!isset($_POST['csrf_token']) || $_POST['csrf_token'] !== $_SESSION['csrf_token']) {
								header("HTTP/1.1 403 Forbidden");
								echo "CSRF token validation failed";
								exit();
				}
}

$csrf_token = bin2hex(random_bytes(32));
$_SESSION['csrf_token'] = $csrf_token;

// Handle AJAX requests
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
				$action = $_POST['action'] ?? '';
				$device_id = $_POST['device_id'] ?? '';

				switch ($action) {
								case 'enable_mitm':
												$result = enable_mitm($device_id);
												break;
								case 'disable_mitm':
												$result = disable_mitm($device_id);
												break;
								case 'get_mitm_status':
												$result = get_mitm_status($device_id);
												break;
								default:
												$result = ['success' => false, 'message' => 'Invalid action'];
				}

				header('Content-Type: application/json');
				echo json_encode($result);
				exit();
}

$devices = get_connected_devices();
?>

<!DOCTYPE html>
<html lang="en">
<head>
				<meta charset="UTF-8">
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<title>MITM Control Panel - Samsung FRP Bypass Pro</title>
				<link rel="stylesheet" href="styles.css">
</head>
<body>
				<div class="container">
								<header>
												<h1>MITM Control Panel</h1>
												<p>Welcome, <?php echo htmlspecialchars($_SESSION['username']); ?> | <a href="logout.php">Logout</a> | <a href="index.php">Main Panel</a></p>
								</header>
								<main>
												<section id="mitmControls">
																<h2>MITM Controls</h2>
																<select id="deviceSelect">
																				<option value="">Select a device</option>
																				<?php foreach ($devices as $device): ?>
																								<option value="<?php echo htmlspecialchars($device['id']); ?>">
																												<?php echo htmlspecialchars($device['name']); ?>
																								</option>
																				<?php endforeach; ?>
																</select>
																<button id="enableMITM" disabled><i class="fas fa-play"></i> Enable MITM</button>
																<button id="disableMITM" disabled><i class="fas fa-stop"></i> Disable MITM</button>
																<div id="mitmStatus"></div>
												</section>
												<section id="log">
																<h2>MITM Log</h2>
																<pre id="mitmLogContent"></pre>
												</section>
								</main>
								<footer>
												<p>&copy; 2023 Samsung FRP Bypass Pro. All rights reserved.</p>
								</footer>
				</div>
				<div id="toast" class="toast"></div>
				<script src="mitm.js"></script>
				<script>
								const csrfToken = "<?php echo $csrf_token; ?>";
				</script>
</body>
</html>