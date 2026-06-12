cmake_minimum_required(VERSION 3.20)
project(kwin-ease-cursor-crossing VERSION 0.1.1)

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
find_package(KWin 6.3...<6.7 REQUIRED)

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

# .deb packaging via `cpack -G DEB --config build/CPackConfig.cmake` (see README)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CONTACT "jannikac <jannikac@mailbox.org>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/jannikac/kwin-ease-cursor-crossing")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KWin plugin easing cursor movement between mismatched screen edges")
set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
# resolve runtime dependencies from the linked libraries on the build machine
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
include(CPack)
