qt_add_executable(app
    main.cpp
)

set_source_files_properties(Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
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
        LoginWindow.qml
)

target_link_libraries(app
    PRIVATE
        Qt6::Quick
        core
)

if(WIN32)
    target_link_libraries(app PRIVATE dwmapi)
endif()
