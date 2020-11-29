#ifndef CONFIG_H
#define CONFIG_H

#include "esp_wifi_types.h"

#include <string>

class Config {
    std::string ssid;
    std::string psk;
    wifi_auth_mode_t security;
    bool commit();
public:
    Config();
    bool reload();
    bool uninitialized();
    std::string get_ssid();
    std::string get_psk();
    wifi_auth_mode_t get_security();
    bool set_network(
        std::string ssid, 
        std::string psk, 
        wifi_auth_mode_t security
    );
};

#endif