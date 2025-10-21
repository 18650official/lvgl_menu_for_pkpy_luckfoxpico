# LVGL Menu for Luckfox Pico

A feature-rich, keyboard-navigable menu system built with LVGL for the Luckfox Pico board. This project provides a clean user interface for accessing system functions like a terminal, viewing system information, and rebooting the device.

![Screenshot Placeholder](https://placehold.co/480x320/1e1e1e/e0e0e0?text=Pico%20Menu%20UI)

## Features

- **Clean, Dark-Themed UI**: Modern and easy-to-read interface.
- **Keyboard Navigation**: Fully controllable via a physical keyboard (uses `evdev`).
- **Real-Time Clock**: Displays the current time on the main screen.
- **Console Mode**: Hides the UI and launches `fbterm` for a full-featured terminal experience on the framebuffer.
- **System Information**: An "About" screen displaying device memory and other system details.
- **Settings Menu**: A persistent settings system for configuring time display (show seconds, 12/24 hour format).
- **Safe Reboot**: A confirmation dialog to prevent accidental reboots.

## Dependencies

- A compatible cross-compiler toolchain (e.g., `arm-rockchip830-linux-uclibcgnueabihf`).
- Standard build tools (`make`).
- The target device must have framebuffer (`/dev/fb0`) and event device (`/dev/input/event*`) support.

## Build Instructions

This project uses a standard `configure` script to generate a Makefile, making it highly portable.

**1. Clone the repository:**
```sh
git clone <your-repository-url>
cd <repository-name>
````

**2. Configure the build:**
Run the `configure` script and point it to your cross-compiler toolchain. The `--cross-compile` flag should be the prefix of your toolchain binaries (e.g., `gcc`, `ld`).

```sh
# Make the script executable
chmod +x ./configure

# Example configuration
./configure --cross-compile=~/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
```

*Tip: If your toolchain's `bin` directory is in your system's `PATH`, you can often omit the full path.*

**3. Build the application:**
Run `make` to compile the source code.

```sh
make
```

The compiled binary will be located at `build/bin/pico-menu`.

## Installation

You can install the compiled binary to a specified directory using `make install`. This is useful for staging files before creating a final firmware image.

By default, it installs to `/usr/local/bin`. You can change this during the configure step:

```sh
# Configure to install to /usr/bin
./configure --prefix=/usr --cross-compile=...

# Install
make install
```

To install into a specific staging directory (e.g., for packaging), use the `DESTDIR` variable:

```sh
make install DESTDIR=./my_rootfs
```

## Usage on Target

1.  Copy the compiled binary `build/bin/pico-menu` to your Luckfox Pico (e.g., to `/usr/bin`).
2.  Ensure your `init.d` or other startup system executes this binary. It is designed to be the main UI application.
3.  **Important**: The `main.c` file has the input device path hardcoded. You may need to change `EVDEV_PATH` if your keyboard is not `/dev/input/event0`.

<!-- end list -->

```c
// In main.c
#define EVDEV_PATH "/dev/input/event0" // <-- CHECK THIS PATH
```

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
