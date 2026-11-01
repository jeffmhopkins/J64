# Host unit tests: compile common/ and tests/host/ with the native compiler and run.
# Runs anywhere with a C compiler; `make test` at the repo root runs it in the container.
CC      ?= cc
CFLAGS  ?= -std=gnu17 -Wall -Wextra -Werror -O1 -g
ROOT    := ../..
BUILD   := build

SRCS := $(wildcard *.c) $(wildcard $(ROOT)/common/src/*.c)

test: $(BUILD)/host-tests
	./$(BUILD)/host-tests
.PHONY: test

$(BUILD)/host-tests: $(SRCS) $(wildcard *.h) $(wildcard $(ROOT)/common/include/j64/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -I$(ROOT)/common/include -o $@ $(SRCS)

clean:
	$(RM) -r $(BUILD)
.PHONY: clean
