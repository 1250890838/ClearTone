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

# 用 PRIVATE：login.h 只前置声明 HttpClient，不把 network 的头文件路径和
# 链接依赖泄漏给 app。以后 core 的公开头文件真要用到 network 类型再改 PUBLIC。
target_link_libraries(core
    PRIVATE
        network
)
