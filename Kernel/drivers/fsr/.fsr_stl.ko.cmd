cmd_drivers/fsr/fsr_stl.ko := /opt/toolchains/arm-2009q3/bin/arm-none-linux-gnueabi-ld -EL -r  -T /home/james/android/indkernel/Kernel/scripts/module-common.lds --build-id -o drivers/fsr/fsr_stl.ko drivers/fsr/fsr_stl.o drivers/fsr/fsr_stl.mod.o