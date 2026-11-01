# Host unit tests: compile common/, every ROM's src/core/ (pure C game rules), and tests/host/
# with the native compiler and run. Runs anywhere with a C compiler; `make test` at the repo
# root runs it in the container.
CC      ?= cc
CFLAGS  ?= -std=gnu17 -Wall -Wextra -Werror -O1 -g
ROOT    := ../..
BUILD   := build

CORES := $(wildcard $(ROOT)/roms/*/src/core)
SRCS  := $(wildcard *.c) $(wildcard $(ROOT)/common/src/*.c) $(foreach d,$(CORES),$(wildcard $(d)/*.c))
HDRS  := $(wildcard *.h) $(wildcard $(ROOT)/common/include/j64/*.h) $(foreach d,$(CORES),$(wildcard $(d)/*.h))

test: $(BUILD)/host-tests
	cd $(ROOT) && tests/host/$(BUILD)/host-tests
.PHONY: test

$(BUILD)/host-tests: $(SRCS) $(HDRS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -I$(ROOT)/common/include $(addprefix -I,$(CORES)) -o $@ $(SRCS) -lm

clean:
	$(RM) -r $(BUILD)
.PHONY: clean
