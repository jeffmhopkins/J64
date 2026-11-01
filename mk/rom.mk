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
#          packed into a DFS image (the ROM reads them as rom:/<name>):
#            assets/*.png         -> <name>.sprite (mksprite picks the pixel format)
#            assets/rgba16/*.png  -> <name>.sprite packed as RGBA16 (big pictures, less memory)
#          Optional, set before the include:
#            ROM_DATA    := data       folders copied as they are (rom:/data/...)
#            ASSET_EXTRA := filesystem/x.font64 ...   more outputs, with rules in the ROM's Makefile

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

# Assets (optional), packed into the ROM's filesystem.
ASSET_PNGS   := $(wildcard assets/*.png)
ASSET_PNGS16 := $(wildcard assets/rgba16/*.png)
DATA_FILES   := $(foreach d,$(ROM_DATA),$(shell find $(d) -type f ! -name '*.md'))
ASSET_OUT    := $(patsubst assets/%.png,filesystem/%.sprite,$(ASSET_PNGS)) \
                $(patsubst assets/rgba16/%.png,filesystem/%.sprite,$(ASSET_PNGS16)) \
                $(addprefix filesystem/,$(DATA_FILES)) $(ASSET_EXTRA)

all: $(ROMNAME).z64
.PHONY: all

$(BUILD_DIR)/common/%.o: $(COMMON)/src/%.c
	@mkdir -p $(dir $@)
	@echo "    [CC] common/$*.c"
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)

ifneq ($(strip $(ASSET_OUT)),)
filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o $(dir $@) "$<"

filesystem/%.sprite: assets/rgba16/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@ (RGBA16)"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) --format RGBA16 -o $(dir $@) "$<"

$(addprefix filesystem/,$(DATA_FILES)): filesystem/%: %
	@mkdir -p $(dir $@)
	@echo "    [DATA] $@"
	cp "$<" "$@"

$(BUILD_DIR)/$(ROMNAME).dfs: $(ASSET_OUT)
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs
endif

clean:
	$(RM) -r $(BUILD_DIR) filesystem $(ROMNAME).z64
.PHONY: clean

ifneq ($(wildcard $(BUILD_DIR)),)
  -include $(shell find $(BUILD_DIR) -name '*.d')
endif
