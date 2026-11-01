ROMNAME          := clankers
N64_ROM_TITLE    := "Clankers"
N64_ROM_SAVETYPE := eeprom4k

# The level, world and music text files go into the ROM as they are.
ROM_DATA := data

# Titan One at the sizes the game uses, with a 1 px outline.
FONT_SIZES  := 8 10 14 18
ASSET_EXTRA := $(foreach s,$(FONT_SIZES),filesystem/titan$(s).font64)

include ../../mk/rom.mk

N64_CFLAGS += -Isrc/core -Isrc/n64

define font_rule
filesystem/titan$(1).font64: assets/TitanOne.ttf
	@mkdir -p filesystem/.font$(1)
	@echo "    [FONT] $$@"
	$$(N64_MKFONT) --size $(1) --outline 1 --range 20-7F --range A0-FF -o filesystem/.font$(1) "$$<"
	mv filesystem/.font$(1)/TitanOne.font64 $$@
	rmdir filesystem/.font$(1)
endef
$(foreach s,$(FONT_SIZES),$(eval $(call font_rule,$(s))))

# The self-playing test build (clankers-test.z64): the same game and filesystem, with main.c
# compiled with CK_AUTOTEST so a scripted tour drives it and logs PASS or FAIL.
# tools/win-test.ps1 runs it in ares.
TEST_ROM  := $(ROMNAME)-test.z64
TEST_OBJS := $(filter-out $(BUILD_DIR)/src/n64/main.o,$(OBJS)) $(BUILD_DIR)/autotest/main.o

all: $(TEST_ROM)

$(BUILD_DIR)/autotest/main.o: src/n64/main.c
	@mkdir -p $(dir $@)
	@echo "    [CC] $< (autotest)"
	$(CC) -c $(CFLAGS) -DCK_AUTOTEST -o $@ $<

$(BUILD_DIR)/$(ROMNAME)-test.elf: $(TEST_OBJS)
$(TEST_ROM): N64_ROM_TITLE := "Clankers test"
$(TEST_ROM): $(BUILD_DIR)/$(ROMNAME).dfs

clean-test:
	$(RM) $(TEST_ROM)
clean: clean-test
.PHONY: clean-test
