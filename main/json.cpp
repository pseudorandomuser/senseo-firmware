#include "json.hpp"

#include "esp_log.h"

#include <cstring>
#include <algorithm>

#define LOG_TAG "json.cpp"

JSON::JSON() {
    this->empty = true;
    this->finalized = false;
    this->json_string = "{";
}

std::string JSON::simple_response(bool success, std::string message) {
    return JSON()
        .add_bool("success", success)
        .add_string("message", message)
        .finalize();
}

void JSON::escape(std::string &value) {
    const char* escape_chars = "\\\"";
    for (int i = 0; i < strlen(escape_chars); i++) {
        const char escape_char = escape_chars[i];
        size_t pos = value.find(escape_char);
        while (pos != std::string::npos) {
            value.insert(pos, "\\");
            pos = value.find(escape_char, pos + 2);
        }
    }
}

void JSON::indent(std::string &value) {
    size_t pos = value.find('\n');
    while (pos != std::string::npos) {
        value.insert(pos + 1, "\t");
        pos = value.find('\n', pos + 2);
    }
}

JSON& JSON::add(std::string key, std::string value, bool quotes, bool escape) {
    if (this->finalized) {
        return *this;
    }
    this->empty = false;
    if (escape) {
        this->escape(key);
        this->escape(value);
    }
    std::string quot = (quotes ? "\"" : "");
    this->json_string += "\n\t\"" + key + "\": " + quot + value + quot + ",";
    return *this;
}

JSON& JSON::add_json(std::string key, JSON json) {
    std::string value = json.finalize();
    this->indent(value);
    return this->add(key, value, false, false);
}

JSON& JSON::add_bool(std::string key, bool value) {
    return this->add(key, (value ? "true" : "false"), false, true);
}

JSON& JSON::add_int(std::string key, int value) {
    return this->add(key, std::to_string(value), false, true);
}

JSON& JSON::add_float(std::string key, float value) {
    return this->add(key, std::to_string(value), false, true);
}

JSON& JSON::add_double(std::string key, double value) {
    return this->add(key, std::to_string(value), false, true);
}

JSON& JSON::add_string(std::string key, std::string value) {
    return this->add(key, value, true, true);
}

size_t JSON::length() {
    return this->json_string.length();
}

std::string JSON::finalize() {
    if (this->finalized) {
        return this->json_string;
    }
    this->finalized = true;
    if (!this->empty) {
        this->json_string.erase(this->json_string.length() - 1);
    }
    this->json_string += "\n}";
    return this->json_string;
}