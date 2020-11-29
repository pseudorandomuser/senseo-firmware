#ifndef JSON_H
#define JSON_H

#include <string>

class JSON {
    
    bool empty;
    bool finalized;
    std::string json_string;
    JSON& add(std::string key, std::string value, bool quotes, bool escape);
    void escape(std::string &value);
    void indent(std::string &value);

public:

    JSON();
    static std::string simple_response(bool success, std::string message);
    JSON& add_bool(std::string key, bool value);
    JSON& add_int(std::string key, int value);
    JSON& add_float(std::string key, float value);
    JSON& add_double(std::string key, double value);
    JSON& add_string(std::string key, std::string value);
    JSON& add_json(std::string key, JSON json);
    size_t length();
    std::string finalize();

};

#endif