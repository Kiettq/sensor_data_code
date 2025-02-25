<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dữ liệu Cảm Biến</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
        }
        table {
            width: 90%;
            margin: 20px auto;
            border-collapse: collapse;
        }
        th, td {
            border: 1px solid black;
            padding: 8px;
            text-align: center;
        }
        th {
            background-color: green;
            color: white;
        }
        tr:nth-child(even) {
            background-color: #f2f2f2;
        }
        .chart-container {
            display: flex;
            justify-content: center;
            align-items: center;
            gap: 30px;
            margin-top: 20px;
            flex-wrap: wrap;
        }
        .chart-box {
            width: 30%;
            min-width: 400px;
            height: 350px;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        canvas {
            width: 100% !important;
            height: 300px !important;
        }
    </style>
</head>
<body>
    <h2>Dữ liệu Cảm Biến</h2>

    <!-- Bảng dữ liệu -->
    <table>
        <tr>
            <th>Tên Cảm biến</th>
            <th>Thiết bị</th>
            <th>Giá trị</th>
            <th>Ngưỡng an toàn</th>
            <th>Thời gian</th>
        </tr>
        <?php
        $servername = "localhost";
        $username = "KietCDT24";
        $password = "kiet";
        $database = "sensor_data";

        $conn = new mysqli($servername, $username, $password, $database);
        if ($conn->connect_error) {
            die("Kết nối thất bại: " . $conn->connect_error);
        }

        $sql = "SELECT * FROM sensor_summary ORDER BY timestamp DESC LIMIT 100";
        $result = $conn->query($sql);

        while ($row = $result->fetch_assoc()) {
            echo "<tr>
                    <td>{$row['sensor_name']}</td>
                    <td>{$row['device']}</td>
                    <td>{$row['value']}</td>
                    <td>{$row['safe_limit']}</td>
                    <td>{$row['timestamp']}</td>
                  </tr>";
        }
        $conn->close();
        ?>
    </table>

    <!-- Biểu đồ 3 nhóm ngang -->
    <div class="chart-container">
        <div class="chart-box">
            <h3>Cảm biến Nhiệt</h3>
            <canvas id="temperatureChart"></canvas>
        </div>
        <div class="chart-box">
            <h3>CPU Temp</h3>
            <canvas id="cpuChart"></canvas>
        </div>
        <div class="chart-box">
            <h3>PIN</h3>
            <canvas id="batteryChart"></canvas>
        </div>
    </div>

    <script>
        async function fetchSensorData() {
            const response = await fetch('get_data.php');
            const data = await response.json();
            return data;
        }

        async function drawCharts() {
            const sensorData = await fetchSensorData();

            // Phân loại dữ liệu theo nhóm
            const tempData = sensorData.filter(d => d.sensor_name.includes("TEMP"));
            const cpuData = sensorData.filter(d => d.sensor_name.includes("CPU"));
            const batteryData = sensorData.filter(d => d.sensor_name.includes("BATTERY"));

            // Hàm vẽ biểu đồ
            function createChart(canvasId, label, dataset, minDataPoints = 5) {
                const ctx = document.getElementById(canvasId).getContext('2d');
                let labels = dataset.map(d => d.timestamp).reverse();
                let values = dataset.map(d => d.value).reverse();

                // Nếu CPU chỉ có một giá trị, lặp lại giá trị đó để tạo hiệu ứng đường ngang
                if (values.length === 1) {
                    for (let i = 0; i < minDataPoints; i++) {
                        labels.push(labels[0]);  // Lặp lại timestamp
                        values.push(values[0]);  // Lặp lại giá trị
                    }
                }

                new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: labels,
                        datasets: [{
                            label: label,
                            data: values,
                            borderColor: 'purple',
                            borderWidth: 2,
                            fill: false,
                            tension: 0.3
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        scales: {
                            x: { 
                                title: { display: true, text: 'Thời gian' }, 
                                ticks: { autoSkip: true, maxTicksLimit: 5 }
                            },
                            y: { title: { display: true, text: 'Giá trị' }}
                        }
                    }
                });
            }

            // Xóa biểu đồ cũ trước khi vẽ mới
            document.getElementById("temperatureChart").remove();
            document.getElementById("cpuChart").remove();
            document.getElementById("batteryChart").remove();

            let chartContainer = document.querySelector(".chart-container");
            chartContainer.innerHTML = `
                <div class="chart-box">
                    <h3>Cảm biến Nhiệt</h3>
                    <canvas id="temperatureChart"></canvas>
                </div>
                <div class="chart-box">
                    <h3>CPU Temp</h3>
                    <canvas id="cpuChart"></canvas>
                </div>
                <div class="chart-box">
                    <h3>PIN</h3>
                    <canvas id="batteryChart"></canvas>
                </div>
            `;

            // Vẽ 3 biểu đồ mới
            createChart("temperatureChart", "Cảm biến Nhiệt", tempData);
            createChart("cpuChart", "CPU Temp", cpuData, 5);
            createChart("batteryChart", "PIN", batteryData);
        }

        drawCharts();
        setInterval(drawCharts, 5000); // Cập nhật mỗi 5 giây
    </script>
</body>
</html>
