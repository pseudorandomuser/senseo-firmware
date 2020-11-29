#include "config.hpp"
#include "netconfig.hpp"
#include "interface.hpp"
#include "nvstorage.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "nvs_flash.h"

#define LOG_TAG "boot.cpp"

extern "C" void app_main(void) {

    ESP_LOGW(LOG_TAG, "Controller is up!");
    NVStorage::init();

    Config config;
    NetConfig netconfig;

    if (config.uninitialized()) {
        ESP_LOGW(LOG_TAG, "Controller needs to be configured.");
        netconfig.publish_ap();
    } else {
        std::string ssid = config.get_ssid();
        std::string psk = config.get_psk();
        wifi_auth_mode_t security = config.get_security();
        netconfig.connect_station(ssid, psk, security);
    }
    
    Interface interface;
    if (interface.start_server()) {
        while (true) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }

}