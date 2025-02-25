#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <mysql/mysql.h>

#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883
#define BASE_TOPIC "KIET/SENSOR/"

#define DB_HOST "localhost"
#define DB_USER "KietCDT24"  
#define DB_PASS "kiet"       
#define DB_NAME "sensor_data"

MYSQL *conn;

// 🔹 Kết nối MySQL
void connect_db() {
    conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "❌ Lỗi khởi tạo MySQL!\n");
        exit(1);
    }
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0)) {
        fprintf(stderr, "❌ Lỗi kết nối MySQL: %s\n", mysql_error(conn));
        exit(1);
    }
    printf("✅ Kết nối MySQL thành công!\n");

    // Bật AutoCommit để lưu dữ liệu ngay lập tức
    mysql_query(conn, "SET autocommit = 1;");
}

// 🔹 Hàm lưu dữ liệu vào MySQL
void save_to_db(const char *sensor, float value) {
    char query[512];
    const char *device_type;
    const char *safe_limit;

    // Xác định loại cảm biến và giới hạn an toàn
    if (strstr(sensor, "CPU_TEMP")) {
        device_type = "CPU (Từng lõi)";
        safe_limit = "<85°C";
    } else if (strstr(sensor, "BATTERY")) {
        device_type = "Pin";
        safe_limit = (strstr(sensor, "VOLTAGE") ? "11-13V" : "N/A");
    } else {
        device_type = "Cảm biến nhiệt";
        safe_limit = "<80°C";
    }

    // 🔹 Lệnh SQL cập nhật dữ liệu 
    snprintf(query, sizeof(query),
        "INSERT INTO sensor_summary (sensor_name, device, value, safe_limit, timestamp) "
        "VALUES ('%s', '%s', %.2f, '%s', NOW()) "
        "ON DUPLICATE KEY UPDATE value = VALUES(value), timestamp = NOW();",
        sensor, device_type, value, safe_limit);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "❌ Lỗi MySQL (%s): %s\n", sensor, mysql_error(conn));
    } else {
        printf("✅ Cập nhật thành công: %s = %.2f\n", sensor, value);
    }
}

// 🔹 Xử lý dữ liệu từ MQTT
void callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    float value = atof(msg->payload);
    save_to_db(msg->topic + strlen(BASE_TOPIC), value);
}

int main() {
    struct mosquitto *mosq;

    connect_db();
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "❌ Lỗi tạo MQTT client\n");
        return 1;
    }

    mosquitto_message_callback_set(mosq, callback);
    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "❌ Không thể kết nối MQTT Broker\n");
        return 1;
    }

    // Đăng ký lắng nghe toàn bộ cảm biến
    mosquitto_subscribe(mosq, NULL, "KIET/SENSOR/#", 0);
    printf("📡 Đang lắng nghe dữ liệu MQTT...\n");

    // Chạy vòng lặp MQTT liên tục
    mosquitto_loop_forever(mosq, -1, 1);

    // Cleanup khi kết thúc
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    mysql_close(conn);
    return 0;
}

