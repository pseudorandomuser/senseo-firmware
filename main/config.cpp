#include "config.hpp"
#include "nvstorage.hpp"

#include "esp_log.h"

#define LOG_TAG "config.cpp"

Config::Config() {
    this->reload();
}

bool Config::reload() {
    ESP_LOGD(LOG_TAG, "Reload: Reloading config...");
    try {
        NVStorage storage("config", true);
        this->ssid = storage.get_str("ssid");
        this->psk = storage.get_str("psk");
        this->security = static_cast<wifi_auth_mode_t>(storage.get_uint8("security"));
        ESP_LOGD(LOG_TAG, "Reload: Config reloaded successful!");
        return true;
    } catch (int error) {
        ESP_LOGE(LOG_TAG, "Reload: Unable to access NVS.");
        return false;
    }
}

bool Config::commit() {
    try {
        NVStorage storage("config", true);
        storage.set_str("ssid", this->ssid.c_str());
        storage.set_str("psk", this->psk.c_str());
        storage.set_uint8("security", this->security);
        return true;
    }
    catch (int error) {
        ESP_LOGE(LOG_TAG, "Commit: Unable to access NVS.");
        return false;
    }
}

std::string Config::get_ssid() {
    return this->ssid;
}

std::string Config::get_psk() {
    return this->psk;
}

wifi_auth_mode_t Config::get_security() {
    return this->security;
}

bool Config::set_network(std::string ssid, std::string psk, wifi_auth_mode_t security) {
    this->ssid = ssid;
    this->psk = psk;
    this->security = security;
    return this->commit();
}

bool Config::uninitialized() {
    return  this->ssid.empty() || 
            this->psk.empty();
}