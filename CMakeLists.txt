cmake_minimum_required(VERSION 3.20)
project(kwin-ease-cursor-crossing VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

set(QT_DEFAULT_MAJOR_VERSION 6)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick)
find_package(KF6 REQUIRED COMPONENTS Config CoreAddons KCMUtils)
# supported KWin range; the private API may change in any minor release, so widen
# this only after verifying the build (see README "Requirements")
find_package(KWin 6.5...<6.7 REQUIRED)

add_subdirectory(kcm)

kcoreaddons_add_plugin(easecursorcrossing INSTALL_NAMESPACE "kwin/plugins")

target_sources(easecursorcrossing PRIVATE
    src/main.cpp
    src/easecursorfilter.cpp
)

ecm_qt_declare_logging_category(easecursorcrossing
    HEADER easecursor_debug.h
    IDENTIFIER KWIN_EASECURSOR
    CATEGORY_NAME kwin_easecursorcrossing
    DEFAULT_SEVERITY Warning
)

target_link_libraries(easecursorcrossing PRIVATE
    KWin::kwin
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::ConfigCore
    KF6::CoreAddons
)
