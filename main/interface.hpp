#ifndef INTERFACE_H
#define INTERFACE_H

#include "esp_http_server.h"

#define RESET_DELAY_SECS 5

class Interface {
    httpd_handle_t server;
public:
    bool start_server();
    bool stop_server();
};

#endif