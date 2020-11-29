#ifndef QUERY_H
#define QUERY_H

#include "esp_http_server.h"

#include <string>

class Query {
    bool empty;
    std::string query;
    std::string decode(std::string value);
public:
    Query(const char* query);
    Query(httpd_req_t* request);
    std::string get(std::string key);
};

#endif