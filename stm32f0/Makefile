# clone libopencm3 into project root directory from https://github.com/libopencm3/libopencm3
OPENCM3_DIR = ../libopencm3
DEVICE = STM32F042C6
OBJS += \
	../software/Action.o \
	../software/Bitmap.o \
	../software/Device.o \
	../software/DeviceState.o \
	../software/EnOceanProtocol.o \
	../software/Font.o \
	../software/Storage.o \
	../software/String.o \
	../software/System.o \
	../software/tahoma_8pt.cpp \
	Clock.o \
	Flash.o \
	Serial.o \
	main.o

# see https://bitbashing.io/embedded-cpp.html
# -ggdb3
CFLAGS += -Os -I. -I../software
CPPFLAGS += -finline-small-functions -findirect-inlining -Os -MD -fno-exceptions -I. -I../software
LDFLAGS += -static -nostartfiles
LDLIBS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: clean all

all: binary.elf binary.bin

flash: binary.bin
	st-flash write binary.bin 0x8000000

clean:
	$(Q)$(RM) -rf binary.* *.o

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
