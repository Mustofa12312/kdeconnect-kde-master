pkg_check_modules(PKG_libei IMPORTED_TARGET libei-1.0>=1.6)
if(NOT PKG_libei_FOUND)
    message(WARNING "shareinputdevices: libei >= 1.6 not found. Skipping shareinputdevices plugin.")
    return()
endif()

kdeconnect_add_plugin(kdeconnect_shareinputdevices SOURCES shareinputdevicesplugin.cpp inputcapturesession.cpp)

target_link_libraries(kdeconnect_shareinputdevices
    kdeconnectcore
    kdeconnectdbusinterfaces
    KF6::I18n
    Qt::GuiPrivate
    PkgConfig::PKG_libei
)

install(FILES "kdeconnect_shareinputdevices_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
