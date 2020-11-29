#include "interface.hpp"
#include "nvstorage.hpp"
#include "config.hpp"
#include "query.hpp"
#include "json.hpp"

#include "html_template.h"

#include "esp_log.h"

#define LOG_TAG "interface.cpp"

esp_err_t command_handler(httpd_req_t* request) {

    Query query(request);
    std::string command = query.get("type");
    httpd_resp_set_type(request, "text/plain");

    if (command.empty()) {
        std::string response = JSON::simple_response(false, "Missing 'type' parameter!");
        httpd_resp_send(request, response.c_str(), response.length());
    }
    
    else {

        if (command == "network") {
            Config config;
            std::string ssid = config.get_ssid();
            std::string response;
            if (ssid.empty()) {
                response = JSON()
                    .add_bool("success", false)
                    .add_string("message", "Unable to read network from config!")
                    .finalize();
            } else {
                response = JSON()
                    .add_bool("success", true)
                    .add_string("message", "Successfully read network from config,")
                    .add_json("network", JSON()
                        .add_string("ssid", ssid)
                        .add_string("psk", config.get_psk())
                        .add_int("security", config.get_security())
                    )
                    .finalize();
            }
            httpd_resp_send(request, response.c_str(), response.length());
        } 
        
        else if (command == "reboot") {
            std::string response = JSON::simple_response(
                true, "Going down in " + std::to_string(RESET_DELAY_SECS) + "seconds..."
            );
            httpd_resp_send(request, response.c_str(), response.length());
            vTaskDelay((RESET_DELAY_SECS * 1000) / portTICK_PERIOD_MS);
            esp_restart();
        } 
        
        else if (command == "reset") {
            bool deinit = NVStorage::deinit();
            bool erase = NVStorage::erase();
            bool success = deinit && erase;
            std::string response = JSON()
                .add_bool("success", success)
                .add_bool("deinit", deinit)
                .add_bool("erase", erase)
                .add_string("message", (success ?
                    "Successfully erased NVS, going down in" +
                    std::to_string(RESET_DELAY_SECS) + " seconds..." :
                    "Failed to erase NVS!"))
                .finalize();
            httpd_resp_send(request, response.c_str(), response.length());
            if (success) {
                vTaskDelay((RESET_DELAY_SECS * 1000) / portTICK_PERIOD_MS);
                esp_restart();
            }
        } 
        
        else {
            std::string response = JSON::simple_response(false, "Invalid command: '" + command + "'.");
            httpd_resp_send(request, response.c_str(), response.length());
        }
    }

    return ESP_OK;
}

esp_err_t root_handler(httpd_req_t* request) {
    const char* response = HTML_TEMPLATE_NETCONFIG;
    httpd_resp_send(request, response, strlen(response));
    return ESP_OK;
}

esp_err_t netconfig_handler(httpd_req_t* request) {

    char* buffer = (char*)calloc(sizeof(char), request->content_len + 1);
    int result = httpd_req_recv(request, buffer, request->content_len);
    if (result <= 0) {
        if (result == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(request);
        } free(buffer);
        return ESP_FAIL;
    }

    Query query(buffer);
    free(buffer);

    std::string ssid = query.get("ssid");
    std::string psk = query.get("psk");

    if (ssid.empty() || psk.empty()) {
        std::string response = JSON::simple_response(false, "Missing SSID/PSK parameters!");
        httpd_resp_send(request, response.c_str(), response.length());
        return ESP_OK;
    }
    
    Config config;
    bool success = config.set_network(ssid, psk, WIFI_AUTH_WPA2_PSK);
    std::string response = JSON::simple_response(
        success, success ? "Successfully set network to '" +
        ssid + "! Going down in " + std::to_string(RESET_DELAY_SECS) + 
        " seconds..." : "Failed to set network."
    );
    httpd_resp_send(request, response.c_str(), response.length());
    if (success) {
        vTaskDelay((RESET_DELAY_SECS * 1000) / portTICK_PERIOD_MS);
        esp_restart();
    }

    return ESP_OK;

}

httpd_uri_t root_uri {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL
};

httpd_uri_t command_uri {
    .uri = "/command",
    .method = HTTP_GET,
    .handler = command_handler,
    .user_ctx = NULL
};

httpd_uri_t netconfig_uri {
    .uri = "/netconfig",
    .method = HTTP_POST,
    .handler = netconfig_handler,
    .user_ctx = NULL
};

bool Interface::start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&this->server, &config) == ESP_OK) {
        httpd_register_uri_handler(this->server, &root_uri);
        httpd_register_uri_handler(this->server, &command_uri);
        httpd_register_uri_handler(this->server, &netconfig_uri);
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