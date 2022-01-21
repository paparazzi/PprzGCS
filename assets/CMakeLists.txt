
find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(ECMInstallIcons)
include(KDEInstallDirs)

ecm_install_icons(
    ICONS
        16-apps-pprzgcs.png
        32-apps-pprzgcs.png
        48-apps-pprzgcs.png
        64-apps-pprzgcs.png
        128-apps-pprzgcs.png

    THEME
        hicolor

    DESTINATION
        ${KDE_INSTALL_ICONDIR}
)

configure_file(
    org.paparazzi.pprzgcs.appdata.xml.in
    org.paparazzi.pprzgcs.appdata.xml
)

install(FILES ${CMAKE_BINARY_DIR}/assets/org.paparazzi.pprzgcs.appdata.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR}
)


install(FILES pprzgcs.svg
        DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps
)


install(FILES "org.paparazzi.pprzgcs.desktop"
        DESTINATION ${KDE_INSTALL_APPDIR}
)

