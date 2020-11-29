#ifndef NETCONFIG_H
#define NETCONFIG_H

#include "esp_wifi_types.h"

#include <string>

#define NETCONFIG_AP_SSID               "ESP32-AP"
#define NETCONFIG_RECONNECT_ATTEMPTS    -1

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

class NetConfig {
public:
    bool connect_station(
        std::string ssid, 
        std::string psk, 
        wifi_auth_mode_t security
    );
    bool publish_ap();
};

#endif