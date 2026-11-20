# ClearClock

ClearClock is a compact, translucent Qt 6 panel for Linux. It starts as a digital clock. Right-click the panel to select any combination of four items, which stack vertically:

- **Clock** displays local time in 24-hour format, including seconds.
- **Battery** shows the charge level as a horizontal bar and a percentage. A dash indicates that no battery reading is available.
- **CPU** plots overall processor usage over the last 60 seconds.
- **Mem** plots the percentage of memory in use over the last 60 seconds, calculated from total and available memory.

Left-click to switch between light and dark backgrounds. Drag with the left mouse button to move the panel.

## Build and run

Install CMake 3.16 or newer, a C++17 compiler, and the Qt 6 development packages for Widgets, D-Bus, and Concurrent. From the project directory, run:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
./build/release/ClearClock
```

To install the executable at `/usr/bin/ClearClock` and add the supplied application launcher:

```sh
sudo install -m 755 build/release/ClearClock /usr/bin/ClearClock
sudo install -Dm644 ClearClock.desktop /usr/share/applications/ClearClock.desktop
```

## License

ClearClock is licensed under the [GNU General Public License, version 3 only](LICENSE) (`GPL-3.0-only`).
