# kwin-ease-cursor-crossing

A third-party KWin plugin that fixes [KDE bug 517413](https://bugs.kde.org/show_bug.cgi?id=517413):
on multi-monitor setups with different resolutions or scale factors, parts of the shared screen edge have no adjacent screen in the logical layout ("deadzones") and the cursor gets stuck there. This plugin detects the cursor being pushed into a deadzone and eases it across to the neighboring screen, similar to Windows 11's "Ease cursor movement between displays".

## How it works

KWin clamps the cursor to the current output's edge when the position beyond it is not covered by any output. The plugin installs a `KWin::InputEventFilter` that watches pointer motion: when the cursor is pinned at an edge, the user keeps pushing outward, and no output exists just beyond that edge (so neither normal crossing nor the soft edge barrier applies), it accumulates the outward motion. Past a threshold it warps the cursor onto the nearest output on the far side of that edge. Pointer constraints held by applications (games, VMs) are respected and synthetic warps are ignored.

## Requirements

- KWin (Wayland) — see the compatibility table below; CMake enforces the supported
  range at configure time, so building against an unsupported KWin fails with a clear
  version error rather than compile errors
- The plugin uses KWin's private API, which has no ABI stability: **rebuild it after
  every KWin update** (typically each Plasma release)
- Build deps: `cmake`, `extra-cmake-modules`, `kwin` (Arch ships the private headers in
  the main package; on other distros install `kwin-devel`/`kwin-dev`)

### KWin compatibility

| Plugin version | KWin      |
| -------------- | --------- |
| 0.1.x          | 6.5 – 6.6 |

The 6.5/6.6 API differences are small enough for type deduction to absorb, so one
source tree covers the whole range. Future KWin versions may change the private API
in ways that need explicit handling (version checks in CMake plus `#ifdef`s); the
supported range lives in the `find_package(KWin ...)` call in `CMakeLists.txt` and
should be widened only after verifying the build against the new release.

## Build & install

### Arch Linux

The repo ships a `PKGBUILD` that builds a pacman package from the working tree:

```sh
cd dist/arch
makepkg -si
```

(`makepkg -si` builds the package and installs it via pacman in one step; use
`makepkg -f` to rebuild after source changes, then `sudo pacman -U
kwin-ease-cursor-crossing-*.pkg.tar.zst`.)

Remember to rebuild and reinstall the package after every `kwin` update (see
Requirements).

### NixOS

The repo ships a Nix expression under `dist/nixos`.

Test-build it standalone:

```sh
nix-build dist/nixos
# result/lib/qt-6/plugins/kwin/plugins/easecursorcrossing.so
```

To install without a local checkout, fetch the repo directly in your
`configuration.nix`:

```nix
{ pkgs, ... }:
let
  kwin-ease-cursor-crossing = builtins.fetchGit {
    url = "https://github.com/jannikac/kwin-ease-cursor-crossing";
    ref = "main";
    # pin to a commit for reproducible builds; update the hash to pull in changes
    rev = "<commit hash>";
  };
in
{
  environment.systemPackages = [
    (pkgs.kdePackages.callPackage "${kwin-ease-cursor-crossing}/dist/nixos/package.nix" { })
  ];
}
```

Alternatively, with a local checkout of the repo:

```nix
environment.systemPackages = [
  (pkgs.kdePackages.callPackage /path/to/kwin-ease-cursor-crossing/dist/nixos/package.nix { })
];
```

then `sudo nixos-rebuild switch` and log out/in. Because the plugin is built by
your system's own `pkgs`, it always links against the exact KWin your session
runs, and gets rebuilt automatically when a system update changes KWin.

### Install from source

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_QTPLUGINDIR=lib/qt6/plugins
cmake --build build
sudo cmake --install build
```

### What gets installed

All methods install `lib/qt6/plugins/kwin/plugins/easecursorcrossing.so` (the
KWin plugin, next to KWin's built-in plugins) and
`lib/qt6/plugins/plasma/kcms/systemsettings/kcm_easecursorcrossing.so` (the
System Settings module) — under `/usr` on Arch, under the package's store path
(plugin dir `lib/qt-6/plugins`) on NixOS. Then restart KWin (log out/in) — or load the plugin into the
running session without restarting:

```sh
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadPlugin easecursorcrossing
```

The settings module shows up in System Settings right away.

The plugin is enabled by default once installed. To disable it without uninstalling, add
to `~/.config/kwinrc`:

```ini
[Plugins]
easecursorcrossingEnabled=false
```

## Configuration

### System Settings

The project ships a System Settings module: **System Settings → Display & Monitor →
Ease Cursor Crossing** (or just search for "cursor crossing"). It has a checkbox to
toggle the behavior plus the edge-barrier and crossing-position settings. Changes
apply immediately to the running session — the plugin watches `kwinrc` live.

The **Edge barrier** setting works like the edge barrier option in **Display &
Monitor → Screen Edges**: it is the distance (in logical pixels) you have to push
the cursor against the edge before it crosses. KDE's built-in option applies where
two screens _do_ meet; this one applies in the deadzones where they don't.

It can also be opened directly with `kcmshell6 kcm_easecursorcrossing` or
`systemsettings kcm_easecursorcrossing`.

### Config file

The same settings live in group `[EaseCursorCrossing]` in `~/.config/kwinrc`:

| Key       | Default   | Meaning                                                                                                                                                                              |
| --------- | --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Active    | `true`    | Master toggle for the easing behavior (what the System Settings checkbox controls)                                                                                                   |
| Threshold | `25`      | Edge barrier: logical pixels of outward push against a deadzone edge before crossing                                                                                                 |
| Mode      | `nearest` | `nearest`: cross at the closest valid point on the destination edge. `proportional`: map the position along the source edge proportionally onto the destination edge (Windows-style) |

Example:

```ini
[EaseCursorCrossing]
Active=true
Threshold=15
Mode=proportional
```

## Testing

The repo ships an automated end-to-end test that needs no second display:

```sh
./tests/run.sh
```

It builds the plugin, starts a sandboxed headless KWin (`--virtual`) with two
virtual outputs arranged so part of their shared edge is a deadzone, injects
pointer motion through KWin's `org_kde_kwin_fake_input` protocol (the small
client in `tests/fakeinput.c` is compiled on the fly against the vendored
`tests/fake-input.xml`), and asserts on the plugin's debug log. It exercises:

- a push through the deadzone warps to the expected point on the other screen
- a push through an aligned edge crosses natively without the plugin acting
- a push at an edge with no screen beyond does not warp
- a config change via change notification (`kwriteconfig6 --notify`, the same
  mechanism the System Settings module uses) is picked up live, and the
  disabled plugin stops easing

The sandboxed compositor runs with a throwaway `XDG_CONFIG_HOME` and its own
Wayland socket, so the running session and the user's config are never touched
(`KWIN_WAYLAND_NO_PERMISSION_CHECKS=1` applies only to that throwaway
instance).

Prerequisites beyond the build deps: the wayland development files
(`libwayland-dev` on Debian/Ubuntu, included with `wayland` on Arch) for
`wayland-scanner` and the client library, plus `kscreen-doctor` and
`kwriteconfig6`, which ship with any Plasma desktop. The test must run inside
a user session (it needs `XDG_RUNTIME_DIR` and a session D-Bus); in a bare
container wrap it with `dbus-run-session`.

For interactive testing without a second monitor, the same setup works nested
and visible: `kwin_wayland --width 1280 --height 720 --output-count 2 --socket
wayland-test` opens each output as a window in the running session, and
`WAYLAND_DISPLAY=wayland-test kscreen-doctor output.WL-1.position.1280,300`
misaligns them so the deadzone can be felt with a real mouse.

## Releasing

Versions are tagged with `release.sh`, which keeps the version number in sync across
`CMakeLists.txt`, `dist/arch/PKGBUILD` (resetting `pkgrel` to 1), and
`dist/nixos/package.nix`:

```sh
./release.sh 0.2.0          # bump versions, commit "release 0.2.0", tag v0.2.0
git push && git push origin v0.2.0
```

The script requires a clean working tree and refuses to overwrite an existing tag.

## License

GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
