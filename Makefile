# J64 top-level Makefile.
#
#   make                  build every ROM into dist/ (inside the dev container)
#   make hello            build one ROM
#   make test             host unit tests
#   make run ROM=hello    open a ROM in ares            (runs on your machine)
#   make deploy ROM=hello upload to SummerCart64 + log  (runs on your machine)
#   make shell            interactive shell in the dev container
#   make clean
#
# Build steps run through tools/dev.sh, which uses Docker unless you are already
# inside the container (devcontainer, CI).

ROMS := $(notdir $(patsubst %/Makefile,%,$(wildcard roms/*/Makefile)))
ROM  ?= $(firstword $(ROMS))
DEV  := tools/dev.sh

all: $(ROMS)
.PHONY: all

# Build a ROM in the container and copy it to dist/.
$(ROMS):
	$(DEV) make --no-print-directory -C roms/$@
	@mkdir -p dist
	cp roms/$@/$@.z64 dist/$@.z64
	@if [ -f roms/$@/$@-test.z64 ]; then cp roms/$@/$@-test.z64 dist/$@-test.z64; fi
.PHONY: $(ROMS)

test:
	$(DEV) make --no-print-directory -C tests/host
.PHONY: test

run: $(ROM)
	tools/run-ares.sh dist/$(ROM).z64
.PHONY: run

deploy: $(ROM)
	tools/deploy-sc64.sh dist/$(ROM).z64
.PHONY: deploy

shell:
	$(DEV) bash
.PHONY: shell

clean:
	$(DEV) sh -c 'for r in $(ROMS); do make --no-print-directory -C roms/$$r clean; done; make --no-print-directory -C tests/host clean'
	rm -rf dist
.PHONY: clean
