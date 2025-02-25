<?php
header('Content-Type: application/json');

$servername = "localhost";
$username = "KietCDT24";
$password = "kiet";
$database = "sensor_data";

// Kết nối đến MySQL
$conn = new mysqli($servername, $username, $password, $database);
if ($conn->connect_error) {
    die(json_encode(["error" => "Kết nối thất bại: " . $conn->connect_error]));
}

// Lấy 100 dòng dữ liệu mới nhất từ bảng sensor_summary
$sql = "SELECT sensor_name, value, timestamp FROM sensor_summary ORDER BY timestamp DESC LIMIT 100";
$result = $conn->query($sql);

$data = [];
while ($row = $result->fetch_assoc()) {
    $data[] = $row;
}

echo json_encode($data);
$conn->close();
?>
