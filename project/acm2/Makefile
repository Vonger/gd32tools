NAME = $(notdir $(CURDIR))
CMSIS = $(CURDIR)/../../GD32F1x0_Firmware_Library_v3.1.0/Firmware/CMSIS
PERIP = $(CURDIR)/../../GD32F1x0_Firmware_Library_v3.1.0/Firmware/GD32F1x0_standard_peripheral
USBD = $(CURDIR)/../../GD32F1x0_Firmware_Library_v3.1.0/Firmware/GD32F1x0_usbd_driver
TOOLCHAIN = $(CURDIR)/../../toolchain/mac/bin/arm-none-eabi

CC = $(TOOLCHAIN)-gcc
CP = $(TOOLCHAIN)-objcopy

DEFINES = -DGD32F130_150 -DUSE_STDPERIPH_DRIVER

INCLUDES = \
	-I$(CURDIR)/../core \
	-I$(CMSIS)/GD/GD32F1x0/Include \
	-I$(PERIP)/Include \
	-I$(USBD)/Include \
	-I$(CURDIR)

SOURCES = \
	$(CURDIR)/../core/core_cm3.c \
	$(CURDIR)/../core/startup_gd32f1x0.s \
	$(CMSIS)/GD/GD32F1x0/Source/system_gd32f1x0.c \
	$(wildcard $(PERIP)/Source/*.c) \
	$(wildcard $(USBD)/Source/*.c) \
	$(wildcard $(CURDIR)/*.c)

OBJECTS = $(SOURCES:%.c=%.o)

CFLAGS = \
	-mcpu=cortex-m3 -mthumb -mlittle-endian -mthumb-interwork \
	-ffast-math -fdata-sections -ffunction-sections \
	-Wl,-T,$(CURDIR)/../core/gd32f150g8.ld,-Map,$(NAME).map,--gc-sections \
	-Wall -std=gnu99 -O2 $(DEFINES) $(INCLUDES)

$(NAME): $(SOURCES)
	@$(CC) $(CFLAGS) $^ -lm -lnosys -o $(CURDIR)/$@
	@$(CP) -O ihex $(CURDIR)/$@ $(CURDIR)/$@.hex

test:
	@echo $(OBJECTS)

clean:
	@rm -f $(CURDIR)/$(NAME)
	@rm -f $(CURDIR)/$(NAME).map

