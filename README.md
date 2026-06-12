# kwin-ease-cursor-crossing

A third-party KWin plugin that fixes [KDE bug 517413](https://bugs.kde.org/show_bug.cgi?id=517413):
on multi-monitor setups with different resolutions or scale factors, parts of the shared screen edge have no adjacent screen in the logical layout ("deadzones") and the cursor gets stuck there. This plugin detects the cursor being pushed into a deadzone and eases it across to the neighboring screen, similar to Windows 11's "Ease cursor movement between displays".

## How it works

KWin clamps the cursor to the current output's edge when the position beyond it is not covered by any output. The plugin installs a `KWin::InputEventFilter` that watches pointer motion: when the cursor is pinned at an edge, the user keeps pushing outward, and no output exists just beyond that edge (so neither normal crossing nor the soft edge barrier applies), it accumulates the outward motion. Past a threshold it warps the cursor onto the nearest output on the far side of that edge. Pointer constraints held by applications (games, VMs) are respected and synthetic warps are ignored.

## Requirements

- KWin (Wayland) — built and tested against **KWin 6.6.5** on Arch Linux
- The plugin uses KWin's private API, which has no ABI stability: **rebuild it after
  every KWin update** (typically each Plasma release)
- Build deps: `cmake`, `extra-cmake-modules`, `kwin` (Arch ships the private headers in
  the main package; on other distros install `kwin-devel`/`kwin-dev`)

## Build & install

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_QTPLUGINDIR=lib/qt6/plugins
cmake --build build
sudo cmake --install build
```

This installs `/usr/lib/qt6/plugins/kwin/plugins/easecursorcrossing.so` (the KWin
plugin, next to KWin's built-in plugins) and
`/usr/lib/qt6/plugins/plasma/kcms/systemsettings/kcm_easecursorcrossing.so` (the
System Settings module). Then restart KWin (log out/in). The settings module shows up in System Settings right away.

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

## License

GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
