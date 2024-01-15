# Prerequisites

1. Download version 5.68 of [SEGGER Embedded Studio for ARM](https://www.segger.com/downloads/embedded-studio/)

2. Download the [9-2020-q2-update of the GNU ARM Toolchain](https://developer.arm.com/downloads/-/gnu-rm)
    * **Make sure the version is correct, you want the 9-2020-q2 update**

3. Download the [nRF CLI Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download)

4. Install script prerequisites

```bash
python3 -m pip install -r ./Scripts/requirements.txt
```

```bash
git submodule update --init --recursive
```

# Quickstart

TLDR: run `make help`

There are three main components to this codebase:
1. Application code
2. Bootloader code
3. Config image generation/flashing (flash anchor ID, etc.)

<br>

For a one-shot build and flash, you can do:

```bash
make everything                             # Only build (not flash)
make flash_everything ANCHOR_ID=<anchor_id> # Build all code and flash the device
```

The application code can be built separately with:

```bash
make
make flash
```

The bootloader can be built separately with:

```bash
make bl
make flash_bl
```

<br>

The anchor ID and other configuration data has to be flashed onto the device.
This can be done like so:

```bash
make config_imgs
make flash_config_img ANCHOR_ID=<anchor_id>
```

# DFU

```bash
python3 ./Scripts/perform_dfu.py --anchor <anchor_id> --img-path ./Projects/QANI/FreeRTOS/DWM3001CDK/ses/Output/Common/Exe/DWM3001CDK-QANI-FreeRTOS.bin
```
