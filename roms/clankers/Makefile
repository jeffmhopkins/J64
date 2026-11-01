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
