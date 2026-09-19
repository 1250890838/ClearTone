qt_add_executable(app
    main.cpp
)

qt_add_qml_module(app
    URI ClearTone
    SOURCES
        framelesswindow.h
        framelesswindow.cpp
        windowbutton.h
        windowbutton.cpp
    QML_FILES
        Main.qml
        Theme.qml
        TitleBar.qml
        CaptionButton.qml
)

target_link_libraries(app
    PRIVATE
        Qt6::Quick
        core
)

if(WIN32)
    target_link_libraries(app PRIVATE dwmapi)
endif()
