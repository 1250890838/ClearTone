qt_add_executable(app
    main.cpp
)

qt_add_qml_module(app
    URI ClearTone
    QML_FILES
        Main.qml
)

target_link_libraries(app
    PRIVATE Qt6::Quick
)