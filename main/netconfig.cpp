#include "netconfig.hpp"
#include "nvstorage.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include <cstring>

#define LOG_TAG "netconfig.cpp"

static int retry_count = 0;
static EventGroupHandle_t s_wifi_event_group;

static void ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

    ESP_LOGD(LOG_TAG, "AP event handler triggered!");

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(LOG_TAG, "Client " MACSTR " joined, AID=%d.", MAC2STR(event->mac), event->aid);
    }

    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(LOG_TAG, "Client " MACSTR " disconnected, AID=%d.", MAC2STR(event->mac), event->aid);
    }
    
}

static void sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

    ESP_LOGD(LOG_TAG, "Station event handler triggered!");

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(LOG_TAG, "Connecting to access point...");
        esp_wifi_connect();
    }
    
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (NETCONFIG_RECONNECT_ATTEMPTS < 0 || retry_count < NETCONFIG_RECONNECT_ATTEMPTS) {
            ESP_LOGW(LOG_TAG, "Connection to access point lost, attempting reconnect...");
            esp_wifi_connect();
            retry_count++;
        } else {
            ESP_LOGE(LOG_TAG, "Connection failed!");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        } 
    }
    
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        retry_count = 0;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(LOG_TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

}

bool NetConfig::publish_ap() { 

    ESP_LOGI(LOG_TAG, "Setting up network in AP mode...");

    ESP_LOGD(LOG_TAG, "Initializing network interface...");
    if (esp_netif_init() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Network interface initialization failed!");
        return false;
    }

    ESP_LOGD(LOG_TAG, "Setting up default event loop...");
    if (esp_event_loop_create_default() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not set up default event loop!");
        return false;
    }

    esp_netif_create_default_wifi_ap();
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_LOGD(LOG_TAG, "Initializing default wireless configuration...");
    if (esp_wifi_init(&config) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not initialize default wireless configuration!");
        return false;
    }

    ESP_LOGD(LOG_TAG, "Registering wireless event handler...");
    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_event_handler, NULL) != ESP_OK) {
        ESP_LOGW(LOG_TAG, "Could not register wireless event handler!");
    }

    ESP_LOGD(LOG_TAG, "Setting wireless mode to AP...");
    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not set wireless mode to AP!");
        return false;
    }

    wifi_config_t wireless_cfg = {};
    strncpy((char*)wireless_cfg.ap.ssid, NETCONFIG_AP_SSID, 32);
    wireless_cfg.ap.ssid_len = strlen(NETCONFIG_AP_SSID);
    wireless_cfg.ap.authmode = WIFI_AUTH_OPEN;
    wireless_cfg.ap.channel = 1;
    wireless_cfg.ap.ssid_hidden = 0;
    wireless_cfg.ap.max_connection = 4;
    wireless_cfg.ap.beacon_interval = 100;

    ESP_LOGD(LOG_TAG, "Setting wireless configuration...");
    if (esp_wifi_set_config(ESP_IF_WIFI_AP, &wireless_cfg) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not set wireless configuration!");
        return false;
    }

    ESP_LOGD(LOG_TAG, "Starting wireless interface...");
    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not start wireless interface!");
        return false;
    }

    ESP_LOGI(LOG_TAG, "Wireless AP setup finished! SSID: %s", NETCONFIG_AP_SSID);
    return true;

}

bool NetConfig::connect_station(std::string ssid, std::string psk, wifi_auth_mode_t security) {

    ESP_LOGI(LOG_TAG, "Connecting to network '%s'...", ssid.c_str());
    ESP_LOGD(LOG_TAG, "Network PSK is %s", psk.c_str());

    s_wifi_event_group = xEventGroupCreate();
    ESP_LOGD(LOG_TAG, "Initializing network interface...");
    if (esp_netif_init() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Network interface initialization failed!");
        return false;
    }
    
    if (esp_event_loop_create_default() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not set up default event loop!");
        return false;
    }

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    
    ESP_LOGD(LOG_TAG, "Initializing default wireless configuration...");
    if (esp_wifi_init(&config) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not initialize default wireless configuration!");
        return false;
    }

    ESP_LOGD(LOG_TAG, "Setting up event handlers...");
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    if ((esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL, &instance_any_id) |
         esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL, &instance_got_ip)) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed setting up event handlers!");
        return false;
    }

    wifi_config_t wireless_cfg = {};
    strncpy((char*)wireless_cfg.sta.ssid, ssid.c_str(), 32);
    strncpy((char*)wireless_cfg.sta.password, psk.c_str(), 64);
    wireless_cfg.sta.threshold.authmode = security;
    wireless_cfg.sta.pmf_cfg.capable = true;
    wireless_cfg.sta.pmf_cfg.required = false;

    ESP_LOGD(LOG_TAG, "Setting wireless mode to station...");
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed setting wireless mode to station!");
        return false;
    }
    
    ESP_LOGD(LOG_TAG, "Setting wireless configuration...");
    if (esp_wifi_set_config(ESP_IF_WIFI_STA, &wireless_cfg) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not set wireless configuration!");
        return false;
    }

    ESP_LOGD(LOG_TAG, "Starting wireless interface...");
    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Could not start wireless interface!");
        return false;
    }

    // Waiting until connection established (WIFI_CONNECTED_BIT) or connection failed over number of re-tries (WIFI_FAIL_BIT)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    // xEventGroupWaitBits() returns the bits before the call returned
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(LOG_TAG, "Connected to SSID '%s'.", ssid.c_str());
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(LOG_TAG, "Failed to connect to SSID '%s'.", ssid.c_str());
    } else {
        ESP_LOGE(LOG_TAG, "Got an unexpected event bit!");
    }

    ESP_LOGD(LOG_TAG, "Unregistering event handlers...");
    if ((esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip) |
         esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id)) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed unregistering event handlers!");
        return false;
    }

    vEventGroupDelete(s_wifi_event_group);
    return true; 

}