ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    HOST_OS := Windows
else
    HOST_OS := $(shell uname)  # same as "uname -s"
endif

ifeq ($(HOST_OS),Windows)
	EM_BUILD := 'C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 5.68\bin\emBuild.exe'
	RMDIR    := del /f /s /q
else
	EM_BUILD := /Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 5.68/bin/emBuild
	RMDIR    := rm -rf
endif

NRFJPROG := nrfjprog -f nrf52

DWM3001CDK_PROJ_DIR  := ./Projects/QANI/FreeRTOS/DWM3001CDK/ses
DWM3001CDK_PROJ_XML  := $(DWM3001CDK_PROJ_DIR)/DWM3001CDK-QANI-FreeRTOS.emProject
DWM3001CDK_BUILD_DIR := $(DWM3001CDK_PROJ_DIR)/Output

TARGET_BIN_DIR := $(DWM3001CDK_BUILD_DIR)/Common/Exe
TARGET_HEX     := $(TARGET_BIN_DIR)/DWM3001CDK-QANI-FreeRTOS_full.hex
TARGET_ELF     := $(TARGET_BIN_DIR)/DWM3001CDK-QANI-FreeRTOS.elf

SOFTDEVICE_HEX = ./SDK_BSP/Nordic/NORDIC_SDK_17_1_0/components/softdevice/s113/hex/s113_nrf52_7.2.0_softdevice.hex

# fuck windows
ifeq ($(HOST_OS),Windows)
	DWM3001CDK_BUILD_DIR := .\\Projects\\QANI\\FreeRTOS\\DWM3001CDK\\ses\\Output
endif

.PHONY: all
all:
	@$(EM_BUILD) -echo -config "Common" $(DWM3001CDK_PROJ_XML)

.PHONY: clean
clean:
	$(RMDIR) $(DWM3001CDK_BUILD_DIR)

.PHONY: flash
flash: all
	$(NRFJPROG) --eraseall
	$(NRFJPROG) --program $(TARGET_HEX) --verify
	$(NRFJPROG) --reset

.PHONY: flash_sd
flash_sd:
	$(NRFJPROG) --program $(SOFTDEVICE_HEX) --verify
	$(NRFJPROG) --reset

