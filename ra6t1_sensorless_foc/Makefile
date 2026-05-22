# RA6T1 Bare-Metal Makefile
# Toolchain: arm-none-eabi-gcc (Arm GNU Toolchain)
# Target:    Cortex-M4 with FPv4-SP hard-float ABI

CROSS   := arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size

# ---- Build output directory -----------------------------------------------
BUILD_DIR := build

# ---- Source directories — add a new folder path here when creating one ----
SRC_DIRS := src

# ---- Target binary stem ---------------------------------------------------
TARGET_NAME := ra6t1_foc
TARGET      := $(BUILD_DIR)/$(TARGET_NAME)

# ---- Collect every .c file found in each source directory -----------------
SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))

# ---- Object files: mirror source paths under BUILD_DIR --------------------
#   e.g.  src/main.c        -> build/src/main.o
#         src/hal/hal_adc.c -> build/src/hal/hal_adc.o
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

# ---- Include paths: auto-derived from SRC_DIRS ----------------------------
INC_FLAGS := $(addprefix -I,$(SRC_DIRS))

# ---- CPU / FPU flags — Cortex-M4 + FPv4-SP-D16 ---------------------------
CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS := $(CPU_FLAGS) \
          $(INC_FLAGS) \
          -Wall -Wextra \
          -O0 -g3 \
          -ffunction-sections -fdata-sections \
          -std=c11

LDFLAGS := $(CPU_FLAGS) \
           -T ld/ra6t1.ld \
           -Wl,--gc-sections \
           -Wl,-Map=$(TARGET).map \
           -nostartfiles \
           -nostdlib

# ===========================================================================
.PHONY: all clean

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin
	@echo "--- Build complete ---"
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $@

# Compile rule: create the mirrored subdirectory under BUILD_DIR, then compile
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
