#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <unistd.h>
#include <dirent.h>

#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883
#define BASE_TOPIC "KIET/SENSOR/"

// Hàm đọc giá trị từ file hệ thống
int read_value_from_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return -1;

    int value;
    if (fscanf(fp, "%d", &value) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return value;
}

// Hàm gửi dữ liệu qua MQTT
void publish_data(struct mosquitto *mosq, const char *sensor, float value) {
    char topic[256], payload[128];
    snprintf(topic, sizeof(topic), "%s%s", BASE_TOPIC, sensor);
    snprintf(payload, sizeof(payload), "%.2f", value);

    if (mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false) == MOSQ_ERR_SUCCESS) {
        printf("Gửi %s: %s\n", sensor, payload);
    } else {
        printf("Lỗi gửi %s!\n", sensor);
    }
}

int main() {
    struct mosquitto *mosq;
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "Lỗi tạo MQTT client\n");
        return 1;
    }
    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Không thể kết nối MQTT Broker\n");
        return 1;
    }

    while (1) {
        // CPU Temperature
        int temp = read_value_from_file("/sys/class/thermal/thermal_zone0/temp");
        if (temp != -1) publish_data(mosq, "CPU_TEMP", temp / 1000.0);

        // Quét cảm biến từ hwmon
        DIR *dir = opendir("/sys/class/hwmon/");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (entry->d_name[0] == '.') continue;

                for (int i = 1; i <= 5; i++) {
                    char temp_path[256];
                    snprintf(temp_path, sizeof(temp_path), "/sys/class/hwmon/%.50s/temp%d_input", entry->d_name, i);
                    int temp_value = read_value_from_file(temp_path);
                    if (temp_value != -1) {
                        char sensor_name[64];
                        snprintf(sensor_name, sizeof(sensor_name), "%.50s_TEMP%d", entry->d_name, i);
                        publish_data(mosq, sensor_name, temp_value / 1000.0);
                    }
                }

                for (int i = 1; i <= 2; i++) {
                    char fan_path[256];
                    snprintf(fan_path, sizeof(fan_path), "/sys/class/hwmon/%.50s/fan%d_input", entry->d_name, i);
                    int fan_speed = read_value_from_file(fan_path);
                    if (fan_speed != -1) {
                        char sensor_name[64];
                        snprintf(sensor_name, sizeof(sensor_name), "%.50s_FAN%d", entry->d_name, i);
                        publish_data(mosq, sensor_name, fan_speed);
                    }
                }
            }
            closedir(dir);
        }

        // Battery Info
        int voltage = read_value_from_file("/sys/class/power_supply/BAT0/voltage_now");
        int current = read_value_from_file("/sys/class/power_supply/BAT0/current_now");
        int capacity = read_value_from_file("/sys/class/power_supply/BAT0/capacity");

        if (voltage != -1) publish_data(mosq, "BATTERY_VOLTAGE", voltage / 1e6);
        if (current != -1) publish_data(mosq, "BATTERY_CURRENT", current / 1e6);
        if (capacity != -1) publish_data(mosq, "BATTERY_CAPACITY", capacity);

        sleep(1);
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}

