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
	OBJCOPY  := 'C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 5.68\gcc\arm-none-eabi\bin\objcopy'
	RMDIR    := del /f /s /q
else
	EM_BUILD := /Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 5.68/bin/emBuild
	SIZE     := /Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 5.68/gcc/arm-none-eabi/bin/arm-none-eabi-size
	OBJCOPY  := /Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 5.68/gcc/arm-none-eabi/bin/objcopy
	RMDIR    := rm -rf
endif

NRFJPROG := nrfjprog -f nrf52

DWM3001CDK_PROJ_DIR  := ./Projects/QANI/FreeRTOS/DWM3001CDK/ses
DWM3001CDK_PROJ_XML  := $(DWM3001CDK_PROJ_DIR)/DWM3001CDK-QANI-FreeRTOS.emProject
DWM3001CDK_BUILD_DIR := $(DWM3001CDK_PROJ_DIR)/Output

TARGET_BIN_DIR := $(DWM3001CDK_BUILD_DIR)/Common/Exe
TARGET_HEX     := $(TARGET_BIN_DIR)/DWM3001CDK-QANI-FreeRTOS_full.hex
TARGET_ELF     := $(TARGET_BIN_DIR)/DWM3001CDK-QANI-FreeRTOS.elf
TARGET_BIN     := $(TARGET_BIN_DIR)/DWM3001CDK-QANI-FreeRTOS.bin

SOFTDEVICE_HEX = ./SDK_BSP/Nordic/NORDIC_SDK_17_1_0/components/softdevice/s113/hex/s113_nrf52_7.2.0_softdevice.hex

CONFIG_BIN_DIR := ./Config_Images

define get_constants_from_headers
	@./Scripts/get_constants_from_headers.sh > /dev/null 2>&1
endef

# fuck windows
ifeq ($(HOST_OS),Windows)
	DWM3001CDK_BUILD_DIR := .\\Projects\\QANI\\FreeRTOS\\DWM3001CDK\\ses\\Output
endif

define reset_mcu
	$(NRFJPROG) --reset
endef

################################################################################################
.PHONY: all
all:
	@$(EM_BUILD) -echo -config "Common" $(DWM3001CDK_PROJ_XML) 2>&1
	@$(SIZE) $(TARGET_ELF)
	@$(OBJCOPY) -O binary $(TARGET_ELF) $(TARGET_BIN)
	$(call get_constants_from_headers)

.PHONY: clean
clean:
	$(RMDIR) $(DWM3001CDK_BUILD_DIR)

.PHONY: __flash
__flash: all
	$(NRFJPROG) --program $(TARGET_HEX) --sectorerase --verify

.PHONY: flash
flash: __flash
	$(call reset_mcu)

################################################################################################

################################################################################################
.PHONY: bl
bl:
	cd ./Bootloader && make
	$(call get_constants_from_headers)

.PHONY: clean_bl
clean_bl:
	cd ./Bootloader && make clean

.PHONY: __flash_bl
__flash_bl:
	cd ./Bootloader && make flash

.PHONY: flash_bl
flash_bl: __flash_bl
	$(call reset_mcu)

################################################################################################

################################################################################################
$(CONFIG_BIN_DIR):
	mkdir $(CONFIG_BIN_DIR)

.PHONY: config_imgs
config_imgs: $(CONFIG_BIN_DIR)
	python3 Scripts/generate_config_imgs.py

.PHONY: clean_config_imgs
clean_config_imgs:
	rm -f $(CONFIG_BIN_DIR)/*.bin

.PHONY: __flash_config_img
__flash_config_img: config_imgs
	$(call check_defined, ANCHOR_ID, which anchod id to flash?)
	$(NRFJPROG) --program $(CONFIG_BIN_DIR)/a$(ANCHOR_ID).hex --sectorerase --verify

.PHONY: flash_config_img
flash_config_img: __flash_config_img
	$(call reset_mcu)

################################################################################################

################################################################################################
.PHONY: everything
everything: all bl config_imgs

.PHONY: clean_everything
clean_everything: clean clean_bl clean_config_imgs

.PHONY: flash_everything
flash_everything: flash_erase __flash __flash_bl __flash_config_img
	$(call reset_mcu)

################################################################################################

################################################################################################
.PHONY: flash_erase
flash_erase:
	$(NRFJPROG) --eraseall

.PHONY: flash_sd
flash_sd:
	$(NRFJPROG) --program $(SOFTDEVICE_HEX) --verify
	$(NRFJPROG) --reset
################################################################################################

################################################################################################
.PHONY: help
help:
	@echo "make                     Build the application"
	@echo "make clean               Clean the application"
	@echo "make flash               Flash the application"
	@echo ""
	@echo "make bl                  Build the bootloader"
	@echo "make clean_bl            Clean the bootloader"
	@echo "make flash_bl            Flash the bootloader"
	@echo ""
	@echo "make config_imgs         Generate config images"
	@echo "make clean_config_imgs   Clean config images"
	@echo "make flash_config_img    Flash config image"
	@echo ""
	@echo "make everything          Build everything"
	@echo "make clean_everything    Clean everything"
	@echo "make flash_everything    Flash everything"
	@echo ""
	@echo "make flash_erase         Erase all flash"
	@echo "make flash_sd            Flash softdevice"
################################################################################################
