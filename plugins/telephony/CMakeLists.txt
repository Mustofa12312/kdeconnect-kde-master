kdeconnect_add_plugin(kdeconnect_telephony SOURCES
    telephonyplugin.cpp
    ../notifications/sendreplydialog.cpp
)
target_include_directories(kdeconnect_telephony PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../notifications
)
target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF6::I18n
    KF6::Notifications
    Qt::DBus
    Qt6::Widgets
)
