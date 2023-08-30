# Anchor Application Code

### Prerequisites

1. Download version 5.68 of [SEGGER Embedded Studio for ARM](https://www.segger.com/downloads/embedded-studio/)

2. Download the [nRF CLI Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download)

3. Download protobuf compiler

```bash
# Debian-based linux
sudo apt install protobuf-compiler
# Macos
brew install protobuf
```

### Build

```
make
```

### Flash

```
make flash
```

# Bootloader

### Prerequisites

1. Download the [9-2020-q2-update of the GNU ARM Toolchain](https://developer.arm.com/downloads/-/gnu-rm)
    * Make sure the version is correct, you want the **9-2020-q2** update

2. Download protobuf compiler

```bash
# Debian-based linux
sudo apt install protobuf-compiler

# Macos
brew install protobuf
```

### Build

```bash
# Use the Makefile in the ./Bootloader directory...
cd ./Bootloader

make
```

### Flash

```bash
# Use the Makefile in the ./Bootloader directory...
cd ./Bootloader

make flash
```

