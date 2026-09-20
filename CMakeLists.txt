cmake_minimum_required(VERSION 3.16)

project(ClearTone VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

# network 必须排在 core 之前：core 要链接它，而 target 名是在处理该子目录时
# 才解析的，排到后面会直接报找不到 target。
add_subdirectory(network)
add_subdirectory(core)
add_subdirectory(app)
