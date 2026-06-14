cmake_minimum_required(VERSION 3.16)
project(cutiedatetime VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

# Qt6::DBus is the only addition over a typical Cutie library
find_package(Qt6 REQUIRED COMPONENTS Core Quick DBus)
qt_standard_project_setup()

# ── Library sources ───────────────────────────────────────────────────────────
set(lib_SOURCES
    src/cutiedatetime.h
    src/cutiedatetime.cpp)

# ── QML plugin sources ────────────────────────────────────────────────────────
set(plugin_SOURCES
    src/plugin.cpp
    src/plugin.h)

# ── Shared library ────────────────────────────────────────────────────────────
qt6_add_library(cutiedatetime SHARED ${lib_SOURCES})

# ── QML module ────────────────────────────────────────────────────────────────
qt6_add_qml_module(qmlcutiedatetimeplugin
    URI Cutie.Datetime
    VERSION ${PROJECT_VERSION}
    PLUGIN_TARGET qmlcutiedatetimeplugin
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        ${plugin_SOURCES}
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/Cutie/Datetime
)

# ── Target properties ─────────────────────────────────────────────────────────
set_target_properties(cutiedatetime PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    PUBLIC_HEADER src/cutiedatetime.h
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(cutiedatetime PUBLIC
    Qt::Core
    Qt::Quick
    Qt::DBus
)

target_link_libraries(qmlcutiedatetimeplugin PUBLIC
    Qt::Core
    Qt::Quick
    Qt::DBus
    cutiedatetime
)

# ── pkg-config ────────────────────────────────────────────────────────────────
configure_file(src/cutiedatetime.pc.in cutiedatetime.pc @ONLY)

# ── Install rules ─────────────────────────────────────────────────────────────
install(TARGETS cutiedatetime
    RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    BUNDLE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(FILES ${CMAKE_BINARY_DIR}/cutiedatetime.pc
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig")

install(TARGETS qmlcutiedatetimeplugin
    RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml/Cutie/Datetime"
    BUNDLE  DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml/Cutie/Datetime"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml/Cutie/Datetime"
)

install(DIRECTORY src/qml/Cutie/Datetime
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml/Cutie")
