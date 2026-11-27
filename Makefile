# Convenience wrapper. The real build is CMake.
BUILD_DIR ?= cmake-build-debug
BIN       ?= $(BUILD_DIR)/mos

S ?=
comma := ,
empty :=
space := $(empty) $(empty)
S_LIST = $(subst $(space),$(comma),$(strip $(subst $(comma),$(space),$(S))))

.PHONY: pa
pa:
	@command -v pahole >/dev/null || { echo "pahole not found: sudo apt install dwarves"; exit 1; }
	@test -n "$(S)" || { echo 'usage: make pa S="mlib_buffer [other_struct ...]" [BIN=path]'; exit 1; }
	@out=$$(cmake --build $(BUILD_DIR) --target $(notdir $(BIN)) 2>&1) || { echo "$$out"; exit 1; }
	@pahole -C $(S_LIST) $(BIN) 2>&1 | grep -v 'tag not supported' | grep . || \
		echo "nothing found for '$(S)' in $(BIN): is the struct actually used in code linked into it?"
