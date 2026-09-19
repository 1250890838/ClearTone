qt_add_qml_module(core
    URI core
    SOURCES
    login.h login.cpp
)

target_include_directories(core
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(core
    PRIVATE
        CORE_LIBRARY
)


