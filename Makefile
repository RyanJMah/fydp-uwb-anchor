check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))

__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $2, ($2))))

ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    HOST_OS := Windows
else
    HOST_OS := $(shell uname)  # same as "uname -s"
endif

ifeq ($(HOST_OS),Windows)
	EM_BUILD := 'C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 5.68\bin\emBuild.exe'
	SIZE     := 'C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 5.68\gcc\arm-none-eabi\bin\arm-none-eabi-size'
	RMDIR    := del /f /s /q
else
	EM_BUILD := /Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 5.68/bin/emBuild
	SIZE     := /Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 5.68/gcc/arm-none-eabi/bin/arm-none-eabi-size
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

PROVISIONING_BIN_DIR := ./Provisioning_Images

define generate_provisioning_image
	@python3 Scripts/generate_flash_config_bin.py $(1)
endef

# fuck windows
ifeq ($(HOST_OS),Windows)
	DWM3001CDK_BUILD_DIR := .\\Projects\\QANI\\FreeRTOS\\DWM3001CDK\\ses\\Output
endif

.PHONY: all
all:
	@$(EM_BUILD) -D MAKEFILE_ANCHOR_ID=$(ANCHOR_ID) -echo -config "Common" $(DWM3001CDK_PROJ_XML) 2>&1
	@$(SIZE) $(TARGET_ELF)

.PHONY: clean
clean:
	$(RMDIR) $(DWM3001CDK_BUILD_DIR)

.PHONY: provision
provision:
	$(call check_defined, ANCHOR_ID, anchor id must be provided)
	$(call generate_provisioning_image, $(ANCHOR_ID))
	$(NRFJPROG) --program $(PROVISIONING_BIN_DIR)/a$(ANCHOR_ID).hex --sectorerase --verify
	$(NRFJPROG) --reset

.PHONY: flash_erase
flash_erase:
	$(NRFJPROG) --eraseall

.PHONY: flash
flash: all
	$(NRFJPROG) --eraseall
	$(NRFJPROG) --program $(TARGET_HEX) --verify
	$(NRFJPROG) --reset

.PHONY: flash_sd
flash_sd:
	$(NRFJPROG) --program $(SOFTDEVICE_HEX) --verify
	$(NRFJPROG) --reset

