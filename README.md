# lang-switch

A Linux (X11) utility written in C++ that tracks keyboard layout switching and shows a popup with the name of the new layout in the center of the bottom third of the screen. The window hides automatically after 1 second.

## How it works

- **Layout tracking.** `XkbMonitor` opens its own connection to the X server and subscribes to `XkbStateNotify` events from the XKB extension. When the keyboard group changes, a notification arrives with the new layout index.
- **Layout list.** Read from the `_XKB_RULES_NAMES` property of the root window (a string like `us,ru`).
- **Display.** SDL2 + SDL2_ttf render a frameless window showing the layout name (for example, `Русский (ru)`). The window is marked as `override-redirect`, so the window manager does not manage it: the app does not appear in the taskbar, the window does not take focus, and it is drawn above the current window (for example, the browser). The window is centered horizontally on the screen and positioned at 5/6 of the screen height, i.e. in the middle of the bottom third.
- **Timer.** `PopupController` — a pure state machine: the window is visible for exactly 1000 ms; a repeated layout switch resets the timer and updates the text.

## Dependencies

Debian/Ubuntu/Linux Mint:

```sh
sudo apt install build-essential cmake pkg-config \
    libx11-dev libsdl2-dev libsdl2-ttf-dev
```

GoogleTest is fetched automatically via CMake `FetchContent` (internet access is required at configure time; if the `libgtest-dev` package is available, it can be used instead).

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Binary: `build/lang-switch`.

## Running

```sh
./build/lang-switch
```

Stop with `Ctrl+C` or by closing the window (SIGTERM is handled as well).
XKB monitor debug messages: `LANGSWITCH_DEBUG=1 ./build/lang-switch`.

## Installing the .deb package (dpkg)

Building the package:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cpack -C Release -G DEB -d build
```

This produces `build/lang-switch-1.1.1-Linux.deb`. To install:

```sh
sudo dpkg -i build/lang-switch-1.1.1-Linux.deb
# if dependencies are missing:
sudo apt -f install
```

Remove with: `sudo dpkg -r lang-switch`.

The package installs:

- `/usr/bin/lang-switch` — the utility itself;
- `/etc/xdg/autostart/lang-switch.desktop` — autostart at desktop session login (XDG autostart: the app starts in the background at login, with no terminal window and no taskbar entry).

After installation, the session will pick up the autostart entry at the next login; to start it right now: `/usr/bin/lang-switch &` (or `pkill -x lang-switch` to stop it).

Disable autostart for a specific user:

```sh
mkdir -p ~/.config/autostart
cp /etc/xdg/autostart/lang-switch.desktop ~/.config/autostart/
sed -i 's/X-GNOME-Autostart-enabled=true/X-GNOME-Autostart-enabled=false/' \
    ~/.config/autostart/lang-switch.desktop
```

## Tests

Unit tests (GoogleTest) cover the pure logic: parsing the layout list, human-readable layout names, stripping variants (`us(dvorak)` -> `us`), and the popup visibility state machine (visibility, auto-hide after 1 second, timer reset, XKB group conversion).

```sh
ctest --test-dir build --output-on-failure
```

## Project structure

```
include/langswitch/
  layout_names.h      layout name parsing (pure logic)
  popup_controller.h  popup visibility state machine (pure logic)
  xkb_monitor.h       XkbStateNotify listener (X11/XKB)
src/
  layout_names.cpp
  popup_controller.cpp
  xkb_monitor.cpp
  main.cpp            SDL2 app: window, text rendering, event loop
tests/
  test_layout_names.cpp
  test_popup_controller.cpp
packaging/
  lang-switch.desktop  XDG autostart entry for /etc/xdg/autostart
```

## Limitations

- X11 only (on Wayland, without an XWayland session, keyboard events are not intercepted).
- Layout names are mapped via a built-in dictionary of commonly used layouts; unknown ones are shown as-is (the code stays the code, e.g. `zz`).

