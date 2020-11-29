#include "interface.hpp"

#include <string>

#include "esp_log.h"

#include "html_template.h"

#include "config.hpp"
#include "nvstorage.hpp"

#define LOG_TAG "interface.cpp"

std::string url_decode(const char* src) {
    std::string ret;
    for (int i=0; i<strlen(src); i++) {
        if (int(src[i])==37) { //%
            char hex[3];
            memcpy(hex, &src[i+1], 2);
            hex[2] = '\0';
            int chr;
            sscanf(hex, "%x", &chr);
            ret += (char)chr;
            i += 2;
        } else if (int(src[i]) == 43) { //+
            ret += ' ';
        } else {
            ret+=src[i];
        }
    }
    return (ret);
}

std::string get_key_value(std::string query, const char* key) {
    esp_err_t result = ESP_OK;
    ESP_LOGD(LOG_TAG, "Get query value: %s", key);
    size_t buffer_size = 32;
    char* buffer = (char*)malloc(buffer_size);
    do  {
        if (result == ESP_ERR_HTTPD_RESULT_TRUNC) {
            buffer_size *= 2;
            free(buffer);
            buffer = (char*)malloc(buffer_size);
        }
        ESP_LOGD(LOG_TAG, "Trying with result %d and buffer size %d...", result, buffer_size);
        result = httpd_query_key_value(query.c_str(), key, buffer, buffer_size);
    } while (result == ESP_ERR_HTTPD_RESULT_TRUNC);
    if (result != ESP_OK) {
        free(buffer);
        return {};
    }
    std::string query_val(buffer);
    free(buffer);
    return query_val;
}

std::string create_result(bool success, std::string message) {
    std::string success_str = (success ? "true" : "false");
    return "{'success':" + success_str + ",'message':'" + message + "'}";
}

esp_err_t command_handler(httpd_req_t* request) {
    size_t query_len = httpd_req_get_url_query_len(request) + 1;
    if (query_len > 1) {
        char* query_buffer = (char*)calloc(sizeof(char), query_len + 1);
        if (httpd_req_get_url_query_str(request, query_buffer, query_len) == ESP_OK) {
            std::string query(query_buffer);
            free(query_buffer);
            ESP_LOGD(LOG_TAG, "Got query string! %s", query_buffer);
            std::string command = get_key_value(query, "command");
            if (command.empty()) {
                std::string response = create_result(false, "Could not find command parameter!");
                httpd_resp_send(request, response.c_str(), response.length());
            } else {
                if (command == "get_network") {
                    Config config;
                    std::string response = create_result(true, config.get_ssid());
                    httpd_resp_send(request, response.c_str(), response.length());
                } else if (command == "set_network") {
                    std::string ssid = get_key_value(query, "ssid");
                    std::string psk = get_key_value(query, "psk");
                    if (ssid.empty() || psk.empty()) {
                        std::string response = create_result(
                            false, "Missing required parameters. Usage: command=set_network&ssid=ssid&psk=psk"
                        );
                        httpd_resp_send(request, response.c_str(), response.length());
                    } else {
                        Config config;
                        config.set_network(ssid, psk, WIFI_AUTH_WPA2_PSK);
                        std::string response = create_result(
                            true, "Set network to '" + ssid + "', going down in " +
                            std::to_string(RESET_DELAY_SECS) + " seconds..."
                        );
                        httpd_resp_send(request, response.c_str(), response.length());
                        vTaskDelay((RESET_DELAY_SECS * 1000) / portTICK_PERIOD_MS);
                        esp_restart();
                    }
                } else if (command == "reboot") {
                    std::string response = create_result(
                        true, "Going down in " + std::to_string(RESET_DELAY_SECS) + " seconds..."
                    );
                    httpd_resp_send(request, response.c_str(), response.length());
                    vTaskDelay((RESET_DELAY_SECS * 1000) / portTICK_PERIOD_MS);
                    esp_restart();
                } else if (command == "reset") {
                    if (NVStorage::deinit() && NVStorage::erase()) {
                        std::string response = create_result(
                            true, "Going to try resetting NVS, going down in " + std::to_string(RESET_DELAY_SECS) + " seconds..."
                        );
                        httpd_resp_send(request, response.c_str(), response.length());
                        vTaskDelay((RESET_DELAY_SECS * 1000) / portTICK_PERIOD_MS);
                        esp_restart();
                    } else {
                        std::string response = create_result(false, "Failed to reset NVS!");
                        httpd_resp_send(request, response.c_str(), response.length());
                    }
                } else {
                    std::string response = create_result(false, "Command not found: " + command);
                    httpd_resp_send(request, response.c_str(), response.length());
                }
            }
        } else {
            free(query_buffer);
            std::string response = create_result(false, "Error reading URL query!");
            httpd_resp_send(request, response.c_str(), response.length());
        }
    } else {
        std::string response = create_result(false, "No URL query present!");
        httpd_resp_send(request, response.c_str(), response.length());
    }
    return ESP_OK;
}

esp_err_t root_handler(httpd_req_t* request) {
    std::string response = HTML_TEMPLATE_NETCONFIG;
    httpd_resp_send(request, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t set_netcfg_handler(httpd_req_t* request) {

    char* buffer = (char*)calloc(sizeof(char), request->content_len + 1);
    int result = httpd_req_recv(request, buffer, request->content_len);
    if (result <= 0) {
        if (result == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(request);
        }
        free(buffer);
        return ESP_FAIL;
    }
    std::string decoded = url_decode(buffer);
    free(buffer);

    std::string ssid = get_key_value(decoded, "ssid");
    std::string psk = get_key_value(decoded, "psk");

    std::string response;

    Config config;
    if (config.set_network(ssid, psk, WIFI_AUTH_WPA2_PSK)) {
        std::string response = create_result(
            true, "Successfully set network to '" + ssid + "'! "
            "Going down in " + std::to_string(RESET_DELAY_SECS) + " seconds..."
        );
        httpd_resp_send(request, response.c_str(), response.length());
        vTaskDelay((RESET_DELAY_SECS * 1000) / portTICK_PERIOD_MS);
        esp_restart();
    } else {
        response = create_result(false, "Failed to set network.");
        httpd_resp_send(request, response.c_str(), response.length());
    }
    return ESP_OK;

}

httpd_uri_t command_uri {
    .uri = "/cmd",
    .method = HTTP_GET,
    .handler = command_handler,
    .user_ctx = NULL
};

httpd_uri_t root_uri {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL
};

httpd_uri_t set_netcfg_uri {
    .uri = "/netcfg",
    .method = HTTP_POST,
    .handler = set_netcfg_handler,
    .user_ctx = NULL
};

bool Interface::start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&this->server, &config) == ESP_OK) {
        httpd_register_uri_handler(this->server, &command_uri);
        httpd_register_uri_handler(this->server, &root_uri);
        httpd_register_uri_handler(this->server, &set_netcfg_uri);
        return true;
    }
    return false;
}

bool Interface::stop_server() {
    if (this->server) {
        return httpd_stop(this->server) == ESP_OK;
    }
    return false;
}