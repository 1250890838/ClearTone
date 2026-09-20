set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# 让 core 能直接 #include "httpclient.h"
target_include_directories(network
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 公开头文件里只前置声明 Qt Network 类型，实现才用到，所以 Network 用 PRIVATE。
# 注意：最终二进制仍然依赖 Qt6Network.dll，部署时 windeployqt 要能扫到。
target_link_libraries(network
    PUBLIC
        Qt6::Core
    PRIVATE
        Qt6::Network
)

target_compile_definitions(network PRIVATE NETWORK_LIBRARY)
