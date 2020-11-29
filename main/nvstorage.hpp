#ifndef NVSTORAGE_H
#define NVSTORAGE_H

#include "nvs.h"

#include <string>

class NVStorage {
    nvs_handle_t handle;
public:
    NVStorage(const char* ns, bool rw);
    ~NVStorage();

    static bool init();
    static bool deinit();
    static bool erase();

    uint8_t get_uint8(const char* key);
    std::string get_str(const char* key);
    bool set_uint8(const char* key, uint8_t value);
    bool set_str(const char* key, const char* value);

    bool reset();
    bool close();
};

#endif