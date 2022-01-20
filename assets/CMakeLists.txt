
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

install(FILES "org.paparazzi.pprzgcs.desktop"
            DESTINATION ${KDE_INSTALL_APPDIR}
    )
