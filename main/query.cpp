#include "query.hpp"

#include <cstring>

Query::Query(const char* query) {
    this->query = std::string(query);
    this->empty = false;
}

Query::Query(httpd_req_t* request) {
    this->empty = true;
    size_t query_len = httpd_req_get_url_query_len(request);
    if (query_len > 1) {
        char* query_buffer = (char*)calloc(sizeof(char), query_len + 1);
        if (httpd_req_get_url_query_str(request, query_buffer, query_len) == ESP_OK) {
            this->query = std::string(query_buffer);
            this->empty = false;
        }
        free(query_buffer);
    }
}

std::string Query::get(std::string key) {
    if (!this->empty) {
        size_t pos = this->query.find(key + "=");
        if (pos != std::string::npos) {
            size_t start = pos + key.length() + 1;
            size_t end = this->query.find("&", start);
            size_t len = (end == std::string::npos ? std::string::npos : end - start);
            std::string value = this->query.substr(start, len);
            return this->decode(value);
        }
    }
    return {};
}

std::string Query::decode(std::string value) {
    std::string result;
    for (int i = 0; i < value.length(); i++) {
        if (value[i] == '%') {
            int current;
            sscanf(value.substr(i + 1, 2).c_str(), "%x", &current);
            result += (char)current;
            i = i + 2;
        } else if (value[i] == '+') {
            result += ' ';
        } else {
            result += value[i];
        }
    }
    return result;
}