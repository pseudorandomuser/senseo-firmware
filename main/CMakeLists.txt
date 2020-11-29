idf_component_register(

    SRCS
    "boot.cpp"
    "config.cpp"
    "interface.cpp"
    "io.cpp"
    "json.cpp"
    "netconfig.cpp"
    "nvstorage.cpp"
    "query.cpp"

    INCLUDE_DIRS ""

)

target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)
