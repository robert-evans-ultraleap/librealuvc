# Set CMAKE_INSTALL_* if not defined
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${LRS_TARGET}")

add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

include(CMakePackageConfigHelpers)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/realuvcConfigVersion.cmake"
    VERSION ${REALSENSE_VERSION_STRING} COMPATIBILITY AnyNewerVersion)

configure_package_config_file(CMake/realuvcConfig.cmake.in realuvcConfig.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/bin
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
configure_file(config/librealuvc.pc.in config/realuvc.pc @ONLY)

install(TARGETS ${LRS_TARGET}
    EXPORT realuvcTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/librealuvc"
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealuvc
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT realuvcTargets
        FILE realuvcTargets.cmake
        NAMESPACE ${LRS_TARGET}::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/realuvcConfig.cmake"
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/realuvcConfigVersion.cmake"
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(CODE "execute_process(COMMAND ldconfig)")

# Set library pkgconfig file for facilitating 3rd party integration
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/config/realuvc.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
