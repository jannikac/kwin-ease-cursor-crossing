kcmutils_add_qml_kcm(kcm_easecursorcrossing SOURCES kcm.cpp)

target_link_libraries(kcm_easecursorcrossing PRIVATE
    Qt6::Core
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::KCMUtilsQuick
)
