#include "nvstorage.hpp"

#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_handle.hpp"

#include "esp_log.h"

#define LOG_TAG "nvstorage.cpp"

bool NVStorage::init() {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(LOG_TAG, "Open: NVS was possibly truncated, erasing...");
        if (nvs_flash_erase() != ESP_OK) {
            ESP_LOGE(LOG_TAG, "Open: NVS could not be erased!");
            return false;
        } result = nvs_flash_init();
    }
    if (result != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Open: NVS could not be initialized!");
        return false;
    }
    return true;
}

bool NVStorage::deinit() {
    return nvs_flash_deinit() == ESP_OK;
}

bool NVStorage::erase() {
    return nvs_flash_erase() == ESP_OK;
}

NVStorage::NVStorage(const char* ns, bool rw) {
    ESP_LOGD(LOG_TAG, "Open: Getting handle to NVS...");
    nvs_open_mode_t mode = (rw ? NVS_READWRITE : NVS_READONLY);
    esp_err_t result = nvs_open(ns, mode, &this->handle);
    if (result == ESP_ERR_NVS_NOT_INITIALIZED) {
        if (!NVStorage::init()) {
            throw;
        }
        result = nvs_open(ns, mode, &this->handle);
    }
    if (result != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Open: Could not open NVS namespace '%s'! (%d)", ns, result);
        throw;
    }
    ESP_LOGD(LOG_TAG, "Open: Successfully obtained NVS handle!");
}

NVStorage::~NVStorage() {
    ESP_LOGD(LOG_TAG, "Destructor: Destructing NVStorage instance...");
    if (nvs_commit(this->handle) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Destructor: Could not commit changes to NVS.");
    }
    ESP_LOGD(LOG_TAG, "Destructor: Closing handle to NVS...");
    nvs_close(this->handle);
}

std::string NVStorage::get_str(const char* key) {
    ESP_LOGD(LOG_TAG, "Get: Getting value for key '%s'...", key);
    size_t length;
    if (nvs_get_str(this->handle, key, NULL, &length) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Get: Could not get size for key '%s'!", key);
        return {};
    }
    char* out = (char*)malloc(sizeof(char) * length);
    if (nvs_get_str(this->handle, key, out, &length) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Get: Could not get value for key '%s'!", key);
        return {};
    }
    std::string result(out);
    free(out);
    return result;
}

bool NVStorage::set_str(const char* key, const char* value) {
    ESP_LOGD(LOG_TAG, "Set: Setting value for key '%s'...", key);
    if (nvs_set_str(this->handle, key, value) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Set: Could not set value for key '%s'!", key);
        return false;
    }
    return nvs_commit(this->handle) == ESP_OK;
}

uint8_t NVStorage::get_uint8(const char* key) {
    ESP_LOGD(LOG_TAG, "Get: Getting value for key '%s'...", key);
    uint8_t value;
    if (nvs_get_u8(this->handle, key, &value) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Get: Could not get value for key '%s'!", key);
        return 0;
    }
    return value;
}

bool NVStorage::set_uint8(const char* key, uint8_t value) {
    ESP_LOGD(LOG_TAG, "Set: Setting value for key '%s'...", key);
    if (nvs_set_u8(this->handle, key, value) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Set: Could not set value for key '%s'!", key);
        return false;
    }
    return nvs_commit(this->handle) == ESP_OK;
}

bool NVStorage::reset() {
    return nvs_erase_all(this->handle) == ESP_OK;
}