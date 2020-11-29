#ifndef INTERFACE_H
#define INTERFACE_H

#include "esp_http_server.h"

#include <string>

#define RESET_DELAY_SECS 5

class Interface {
    httpd_handle_t server;
    static std::string create_result(
        bool success, 
        std::string message
    );
public:
    bool start_server();
    bool stop_server();
};

#endif