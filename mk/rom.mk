# Shared build rules for every ROM under roms/.
#
# A ROM's Makefile sets ROMNAME and N64_ROM_TITLE, then includes this file:
#
#     ROMNAME       := hello
#     N64_ROM_TITLE := "J64 Hello"
#     include ../../mk/rom.mk
#
# Sources: every .c file under the ROM's src/, plus common/src/ (shared code).
# Assets:  files under the ROM's assets/ are converted into its filesystem/ and
#          packed into a DFS image when the ROM has an assets/ folder.

ifeq ($(N64_INST),)
  $(error N64_INST is not set: build through the repo-root Makefile or tools/dev.sh)
endif

J64_ROOT  := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
BUILD_DIR := build
COMMON    := $(J64_ROOT)/common

include $(N64_INST)/include/n64.mk

# Version string baked into the ROM: J64_VERSION from CI, else git describe.
J64_VERSION ?=
ifeq ($(strip $(J64_VERSION)),)
  J64_VERSION := $(shell git -C $(J64_ROOT) describe --tags --always --dirty 2>/dev/null || echo dev)
endif

N64_CFLAGS += -I$(COMMON)/include '-DJ64_VERSION="$(J64_VERSION)"'

SRC_FILES    := $(shell find src -name '*.c')
COMMON_FILES := $(shell find $(COMMON)/src -name '*.c' 2>/dev/null)
OBJS := $(addprefix $(BUILD_DIR)/,$(SRC_FILES:.c=.o)) \
        $(patsubst $(COMMON)/src/%.c,$(BUILD_DIR)/common/%.o,$(COMMON_FILES))

# Assets (optional): assets/*.png -> filesystem/*.sprite, packed into the ROM.
ASSET_PNGS := $(wildcard assets/*.png)
ASSET_OUT  := $(patsubst assets/%.png,filesystem/%.sprite,$(ASSET_PNGS))

all: $(ROMNAME).z64
.PHONY: all

$(BUILD_DIR)/common/%.o: $(COMMON)/src/%.c
	@mkdir -p $(dir $@)
	@echo "    [CC] common/$*.c"
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)

ifneq ($(ASSET_PNGS),)
filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o $(dir $@) "$<"

$(BUILD_DIR)/$(ROMNAME).dfs: $(ASSET_OUT)
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs
endif

clean:
	$(RM) -r $(BUILD_DIR) filesystem $(ROMNAME).z64
.PHONY: clean

ifneq ($(wildcard $(BUILD_DIR)),)
  -include $(shell find $(BUILD_DIR) -name '*.d')
endif
