# desktop integration: icons, menu entry and AppStream metadata, at the standard XDG places
# (GNUInstallDirs is included by the top-level CMakeLists.txt)

foreach(size 16 32 48 64 128)
    install(FILES ${size}-apps-pprzgcs.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME pprzgcs.png
    )
endforeach()

install(FILES pprzgcs.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

configure_file(
    org.paparazzi.pprzgcs.appdata.xml.in
    org.paparazzi.pprzgcs.appdata.xml
)

install(FILES ${CMAKE_BINARY_DIR}/assets/org.paparazzi.pprzgcs.appdata.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
)

install(FILES org.paparazzi.pprzgcs.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
