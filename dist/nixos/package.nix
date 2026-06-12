# Build with the kdePackages scope so all Qt6/KF6 deps match the system's Plasma:
#   kdePackages.callPackage ./dist/nixos/package.nix { }
{
  lib,
  stdenv,
  cmake,
  extra-cmake-modules,
  qtbase,
  qtdeclarative,
  kwin,
  kconfig,
  kcoreaddons,
  kcmutils,
  kirigami,
}:

stdenv.mkDerivation {
  pname = "kwin-ease-cursor-crossing";
  version = "0.1.1";

  # The repo this file lives in, minus VCS/build/packaging clutter
  src = lib.cleanSourceWith {
    src = lib.cleanSource ../..;
    filter =
      path: type:
      !(builtins.elem (baseNameOf path) [
        "build"
        "dist"
      ]);
  };

  nativeBuildInputs = [
    cmake
    extra-cmake-modules
  ];

  buildInputs = [
    qtbase
    qtdeclarative
    kwin
    kconfig
    kcoreaddons
    kcmutils
    kirigami
  ];

  # Only plugins are installed, no executables to wrap
  dontWrapQtApps = true;

  meta = {
    description = "KWin plugin that eases cursor movement between screens with mismatched edges";
    longDescription = ''
      Fixes KDE bug 517413: on multi-monitor setups with different resolutions
      or scale factors, the cursor gets stuck in deadzones along shared screen
      edges. The plugin warps it across after a short push, similar to Windows
      11's "Ease cursor movement between displays". Includes a System Settings
      module under Display & Monitor.
    '';
    homepage = "https://bugs.kde.org/show_bug.cgi?id=517413";
    license = with lib.licenses; [
      gpl2Only
      gpl3Only
    ];
    platforms = lib.platforms.linux;
  };
}
