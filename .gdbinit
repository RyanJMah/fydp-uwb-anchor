target extended-remote :2331

# file ./Projects/QANI/FreeRTOS/DWM3001CDK/ses/Output/Common/Exe/DWM3001CDK-QANI-FreeRTOS.elf
file ./Bootloader/build/gl_bootloader.out
# file ./Bootloader/Output/Common/Exe/gl_bootloader.elf

# load

mon reset 0

b main


