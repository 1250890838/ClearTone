find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_library(network SHARED
    network_global.h
    networkerror.h
    networkerror.cc
    httprequest.h
    httprequest.cc
    httpresponse.h
    httpresponse.cc
    httpreply.h
    httpreply.cc
    httpclient.h
    httpclient.cc
)

target_include_directories(network
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network
    PUBLIC
        Qt6::Core
    PRIVATE
        Qt6::Network
)

target_compile_definitions(network PRIVATE NETWORK_LIBRARY)
