PLUGIN_NAME := RimellAnamorphic
PLUGIN_VERSION := 0.1.0
OPENFX_REV := 1b74e4369c02327012bf2abdd4f39319f52b8cbe

CXX ?= clang++
BUILD_DIR := build
DEPS_DIR := deps
OPENFX_DIR := $(DEPS_DIR)/openfx
BUNDLE := $(BUILD_DIR)/$(PLUGIN_NAME).ofx.bundle
CONTENTS := $(BUNDLE)/Contents

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARCH_DIR := MacOS
  INSTALL_DIR ?= $(HOME)/Library/OFX/Plugins
  LDFLAGS_PLUGIN := -bundle -undefined dynamic_lookup
else ifeq ($(OS),Windows_NT)
  ARCH_DIR := Win64
  INSTALL_DIR ?= C:/Program Files/Common Files/OFX/Plugins
  LDFLAGS_PLUGIN := -shared
else
  ARCH_DIR := Linux-x86-64
  INSTALL_DIR ?= $(HOME)/.OFX/Plugins
  LDFLAGS_PLUGIN := -shared
endif

BIN_DIR := $(CONTENTS)/$(ARCH_DIR)
PLUGIN_BINARY := $(BIN_DIR)/$(PLUGIN_NAME).ofx
INFO_PLIST := $(CONTENTS)/Info.plist

CPPFLAGS += -I$(OPENFX_DIR)/include -DRIMELL_ANAMORPHIC_VERSION=\"$(PLUGIN_VERSION)\"
CXXFLAGS += -std=c++17 -O3 -fPIC -fvisibility=hidden -Wall -Wextra -Wpedantic

.PHONY: all clean deps install package

all: $(PLUGIN_BINARY) $(INFO_PLIST)

deps:
	@if [ ! -d "$(OPENFX_DIR)/.git" ]; then \
		mkdir -p "$(DEPS_DIR)"; \
		git clone https://github.com/AcademySoftwareFoundation/openfx.git "$(OPENFX_DIR)"; \
	fi
	@git -C "$(OPENFX_DIR)" fetch --depth 1 origin "$(OPENFX_REV)"
	@git -C "$(OPENFX_DIR)" checkout --detach "$(OPENFX_REV)"

$(PLUGIN_BINARY): src/RimellAnamorphic.cpp | deps
	@mkdir -p "$(BIN_DIR)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS_PLUGIN) "$<" -o "$@"

$(INFO_PLIST): resources/Info.plist.in
	@mkdir -p "$(CONTENTS)"
	@sed \
		-e 's/@OFX_PLUGIN_NAME@/$(PLUGIN_NAME)/g' \
		-e 's/@PROJECT_VERSION@/$(PLUGIN_VERSION)/g' \
		"$<" > "$@"

install: all
	@mkdir -p "$(INSTALL_DIR)"
	cp -R "$(BUNDLE)" "$(INSTALL_DIR)/"

package: all
	@tar -czf "$(BUILD_DIR)/$(PLUGIN_NAME)-$(PLUGIN_VERSION).tar.gz" -C "$(BUILD_DIR)" "$(PLUGIN_NAME).ofx.bundle"

clean:
	rm -rf "$(BUILD_DIR)"
