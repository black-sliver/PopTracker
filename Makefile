$(warning We switched to the Meson build system. See BUILD.md. This Makefile is only provided for back compat.)

CONF ?= RELEASE # make CONF=DEBUG for debug, CONF=DIST for .zip

ifeq (, $(shell which meson))
    $(error Meson not found. Please install it to build. See BUILD.md)
endif

TMP_BUILD_BASE_DIR ?= build-tmp
TMP_NATIVE_BUILD_DIR := $(TMP_BUILD_BASE_DIR)/native
TMP_CROSS_WIN64_BUILD_DIR := $(TMP_BUILD_BASE_DIR)/cross-win64

# extract version
VERSION := $(shell meson introspect meson.build --projectinfo | jq -r '.version' )
VS := $(subst .,-,$(VERSION))
$(info Version   $(VERSION))

# detect OS
ifeq ($(OS),Windows_NT)
  IS_WIN = yes
  ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
    IS_WIN64 = yes
  else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
    IS_WIN64 = yes
  else
    IS_WIN32 = yes
  endif
else
  ifeq ($(shell uname -s),Darwin)
    IS_OSX = yes
    IS_LLVM = yes
  else
    IS_LINUX = yes
  endif
endif
ifneq '' '$(findstring clang,$(CC))'
  IS_LLVM = yes
endif

# output
ARCH = $(shell uname -m | tr -s ' ' '-' | tr A-Z a-z)
UNAME = $(shell uname -sm | tr -s ' ' '-' | tr A-Z a-z)
DIST_DIR ?= dist
BUILD_DIR ?= build
EXE_NAME = poptracker
FUZZER_EXE_NAME = fuzzer
TEST_EXE_NAME = poptracker-test
BENCH_EXE_NAME = poptracker-benchmark
NIX_BUILD_DIR = $(BUILD_DIR)/$(UNAME)
WIN32_BUILD_DIR = $(BUILD_DIR)/win32
WIN64_BUILD_DIR = $(BUILD_DIR)/win64
WASM_BUILD_DIR = $(BUILD_DIR)/wasm
WIN32_EXE = $(WIN32_BUILD_DIR)/$(EXE_NAME).exe
WIN32_TEST_EXE = $(WIN32_BUILD_DIR)/$(TEST_EXE_NAME).exe
WIN32_FUZZER_EXE = $(WIN32_BUILD_DIR)/$(FUZZER_EXE_NAME).exe
WIN32_BENCH_EXE = $(WIN32_BUILD_DIR)/$(BENCH_EXE_NAME).exe
WIN64_EXE = $(WIN64_BUILD_DIR)/$(EXE_NAME).exe
WIN64_TEST_EXE = $(WIN64_BUILD_DIR)/$(TEST_EXE_NAME).exe
WIN64_FUZZER_EXE = $(WIN64_BUILD_DIR)/$(FUZZER_EXE_NAME).exe
WIN64_BENCH_EXE = $(WIN64_BUILD_DIR)/$(BENCH_EXE_NAME).exe
NIX_EXE = $(NIX_BUILD_DIR)/$(EXE_NAME)
NIX_TEST_EXE = $(NIX_BUILD_DIR)/$(TEST_EXE_NAME)
NIX_FUZZER_EXE = $(NIX_BUILD_DIR)/$(FUZZER_EXE_NAME)
NIX_BENCH_EXE = $(NIX_BUILD_DIR)/$(BENCH_EXE_NAME)
HTML = $(WASM_BUILD_DIR)/$(EXE_NAME).html

# dist/zip
ifeq ($(CONF), DIST)
ifdef IS_OSX
  OSX_APP := $(NIX_BUILD_DIR)/poptracker.app
  OSX_ZIP := $(DIST_DIR)/poptracker_$(VS)_macos_$(ARCH).zip
else ifdef IS_LINUX
  DISTRO = $(shell lsb_release -si | tr -s ' ' '-' | tr A-Z a-z )
  DISTRO_VERSION = $(shell lsb_release -sr | tr -s '.' '-' | tr A-Z a-z )
  NIX_XZ := $(DIST_DIR)/poptracker_$(VS)_$(DISTRO)-$(DISTRO_VERSION)-$(ARCH).tar.xz
endif
WIN32_ZIP := $(DIST_DIR)/poptracker_$(VS)_win32.zip
WIN64_ZIP := $(DIST_DIR)/poptracker_$(VS)_win64.zip
endif

# tool config
MESON_FLAGS = ""

ifeq ($(CONF), DEBUG) # DEBUG
MESON_FLAGS += --buildtype debugoptimized -Doptimization=g
endif

ifdef WITH_ASAN
MESON_FLAGS +=  -Db_sanitize=address
endif

ifdef WITH_UBSAN
MESON_FLAGS +=  -Db_sanitize=undefined
endif

# default target: "native"
ifdef IS_WIN32
  EXE = $(WIN32_EXE)
  TEST_EXE = $(WIN32_TEST_EXE)
  FUZZER_EXE = $(WIN32_FUZZER_EXE)
  BENCH_EXE = $(WIN32_BENCH_EXE)
  ifeq ($(CONF), DIST)
    native: $(WIN32_ZIP)
  else
    native: $(WIN32_EXE)
  endif
else ifdef IS_WIN64
  EXE = $(WIN64_EXE)
  TEST_EXE = $(WIN64_TEST_EXE)
  FUZZER_EXE = $(WIN64_FUZZER_EXE)
  BENCH_EXE = $(WIN64_BENCH_EXE)
  ifeq ($(CONF), DIST)
    native: $(WIN64_ZIP)
  else
    native: $(WIN64_EXE)
  endif
else ifdef IS_OSX
  EXE = $(NIX_EXE)
  TEST_EXE = $(NIX_TEST_EXE)
  FUZZER_EXE = $(NIX_FUZZER_EXE)
  BENCH_EXE = $(NIX_BENCH_EXE)
  ifeq ($(CONF), DIST) # TODO dmg?
    native: $(OSX_APP) $(OSX_ZIP) test_osx_app
  else
    native: $(NIX_EXE)
  endif
else
  EXE = $(NIX_EXE)
  TEST_EXE = $(NIX_TEST_EXE)
  FUZZER_EXE = $(NIX_FUZZER_EXE)
  BENCH_EXE = $(NIX_BENCH_EXE)
  ifeq ($(CONF), DIST) # TODO deb?
    native: $(NIX_XZ)
  else
    native: $(NIX_EXE)
  endif
endif

.PHONY: all native cross wasm clean test_osx_app commandline update-cacert assets/cacert.pem bench $(NIX_EXE) $(NIX_XZ) $(OSX_APP) $(OSX_ZIP) $(WIN64_EXE) $(WIN64_ZIP) $(WIN32_EXE) $(WIN32_ZIP) native_build_impl native_test_impl native_fuzzer_impl native_bench_impl cross_setup_impl
all: native cross wasm commandline
wasm: $(HTML)
commandline: doc/commandline.txt
update-cacert: assets/cacert.pem

ifeq ($(CONF), DIST)
cross: $(WIN32_ZIP) $(WIN64_ZIP)
cross64: $(WIN64_ZIP)
else
cross: $(WIN32_EXE) $(WIN64_EXE)
cross64: $(WIN64_EXE)
endif
cross-test: $(WIN32_TEST_EXE) $(WIN64_TEST_EXE)
cross64-test: $(WIN64_TEST_EXE)

# Project Targets
assets/cacert.pem:
	./scripts/update-cacert.sh

doc/commandline.txt: $(EXE)
	./scripts/update-commandline.sh

native_build_impl:
	mkdir -p "$(TMP_BUILD_BASE_DIR)"
	meson setup --reconfigure $(MESON_FLAGS) "$(TMP_NATIVE_BUILD_DIR)"
	meson compile -v -C "$(TMP_NATIVE_BUILD_DIR)"
native_test_impl:
	mkdir -p "$(TMP_BUILD_BASE_DIR)"
	meson setup --reconfigure $(MESON_FLAGS) "$(TMP_NATIVE_BUILD_DIR)"
	meson test -v -C "$(TMP_NATIVE_BUILD_DIR)"
native_fuzzer_impl:
	mkdir -p "$(TMP_BUILD_BASE_DIR)"
	meson setup --reconfigure $(MESON_FLAGS) "$(TMP_NATIVE_BUILD_DIR)"
	meson compile -v -C "$(TMP_NATIVE_BUILD_DIR)" fuzzer
native_bench_impl:
	mkdir -p "$(TMP_BUILD_BASE_DIR)"
	meson setup --reconfigure $(MESON_FLAGS) "$(TMP_NATIVE_BUILD_DIR)"
	meson test --benchmark -v -C "$(TMP_NATIVE_BUILD_DIR)"

$(NIX_EXE): native_build_impl
	mkdir -p "$(NIX_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/$(EXE_NAME)" $@
$(NIX_TEST_EXE): native_test_impl
	mkdir -p "$(NIX_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/test/$(TEST_EXE_NAME)" $@
$(NIX_FUZZER_EXE): native_fuzzer_impl
	mkdir -p "$(NIX_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/test/$(FUZZER_EXE_NAME)" $@
$(NIX_BENCH_EXE): native_bench_impl
	mkdir -p "$(NIX_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/bench/$(BENCH_EXE_NAME)" $@

ifdef IS_WIN32
# native 32bit Window build
$(WIN32_EXE): native_build_impl
	mkdir -p "$(WIN32_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/$(EXE_NAME).exe" $@
$(WIN32_TEST_EXE): native_test_impl
	mkdir -p "$(WIN32_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/test/$(TEST_EXE_NAME).exe" $@
$(WIN32_FUZZER_EXE): native_fuzzer_impl
	mkdir -p "$(WIN32_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/test/$(FUZZER_EXE_NAME).exe" $@
$(WIN32_BENCH_EXE): native_bench_impl
	mkdir -p "$(WIN32_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/bench/$(BENCH_EXE_NAME).exe" $@
else
# cross 32bit Window build - not implemented
$(WIN32_EXE):
	$(error Cross compiling 32bit Windows not supported anymore. Run `make cross64` for 64bit)
$(WIN32_TEST_EXE):
	$(error Cross compiling 32bit Windows not supported anymore. Run `make cross64-test` for 64bit)
$(WIN32_FUZZER_EXE):
	$(error Cross compiling 32bit Windows not supported anymore)
$(WIN32_BENCH_EXE):
	$(error Cross compiling 32bit Windows not supported anymore)
endif

ifdef IS_WIN64
# natiove 64bit Window build
$(WIN64_EXE): native_build_impl
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/$(EXE_NAME).exe" $@
$(WIN64_TEST_EXE): native_test_impl
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/test/$(TEST_EXE_NAME).exe" $@
$(WIN64_FUZZER_EXE): native_fuzzer_impl
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/test/$(FUZZER_EXE_NAME).exe" $@
$(WIN64_BENCH_EXE): native_bench_impl
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_NATIVE_BUILD_DIR)/bench/$(BENCH_EXE_NAME).exe" $@
else
# cross 64bit Window build
cross_setup_impl:
	mkdir -p "$(TMP_BUILD_BASE_DIR)"
	meson setup --reconfigure $(MESON_FLAGS) --cross-file win32/x86_64-w64-mingw32.ini "$(TMP_CROSS_WIN64_BUILD_DIR)"
$(WIN64_EXE): cross_setup_impl
	meson compile -v -C "$(TMP_CROSS_WIN64_BUILD_DIR)"
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_CROSS_WIN64_BUILD_DIR)/$(EXE_NAME).exe" $@
$(WIN64_TEST_EXE): cross_setup_impl
	meson test -v -C "$(TMP_CROSS_WIN64_BUILD_DIR)"
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_CROSS_WIN64_BUILD_DIR)/test/$(TEST_EXE_NAME).exe" $@
$(WIN64_FUZZER_EXE): cross_setup_impl
	meson compile -v -C "$(TMP_CROSS_WIN64_BUILD_DIR)" fuzzer
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_CROSS_WIN64_BUILD_DIR)/test/$(FUZZER_EXE_NAME).exe" $@
$(WIN64_BENCH_EXE): cross_setup_impl
	meson test --benchmark -v -C "$(TMP_CROSS_WIN64_BUILD_DIR)"
	mkdir -p "$(WIN64_BUILD_DIR)"
	cp "$(TMP_CROSS_WIN64_BUILD_DIR)/bench/$(BENCH_EXE_NAME).exe" $@
endif

$(HTML):
	$(error wasm target not supported at the moment)

$(NIX_XZ): $(NIX_EXE)
	# TODO: package

$(WIN32_ZIP): $(WIN32_EXE)
$(WIN64_ZIP): $(WIN64_EXE)
$(WIN32_ZIP) $(WIN64_ZIP):
	mkdir -p "$(DIST_DIR)"
	$(eval TGT = $(shell echo "$@" | rev | cut -d'_' -f 1 | rev | cut -d'.' -f 1))
	$(eval TMP_DIR = $(DIST_DIR)/.tmp-$(TGT))
	rm -rf $(TMP_DIR)
	mkdir -p $(TMP_DIR)/poptracker/packs
	cp -r api $(TMP_DIR)/poptracker/
	cp -r schema $(TMP_DIR)/poptracker/
	cp -r assets $(TMP_DIR)/poptracker/
	cp -r key $(TMP_DIR)/poptracker/
	cp LICENSE README.md CHANGELOG.md CREDITS.md $(TMP_DIR)/poptracker/
	cp $(dir $<)*.exe $(TMP_DIR)/poptracker/
	cp $(dir $<)*.dll $(TMP_DIR)/poptracker/ || true
	rm $(TMP_DIR)/poptracker/*test.exe || true
	rm $(TMP_DIR)/poptracker/*benchmark.exe || true
	rm -f $@
	(cd $(TMP_DIR) && \
	    if [ -x "`which 7z`" ]; then 7z a -mx=9 ../$(notdir $@) poptracker ; \
	    else zip -9 -r ../$(notdir $@) poptracker ; fi && \
	    if [ -x "`which advzip`" ]; then advzip --recompress -4 ../$(notdir $@) ; fi \
	)
	rm -rf $(TMP_DIR)

$(OSX_APP): $(NIX_EXE)
	# TODO: switch to meson install
	./macosx/bundle_macosx_app.sh --version=$(VERSION) --deployment-target=$(DEPLOYMENT_TARGET) "$(NIX_EXE)"

test_osx_app: $(OSX_APP)
	# test that the app bundle is correctly build
	cd ./$(OSX_APP)/Contents/MacOS; ./poptracker --version

$(OSX_ZIP): $(OSX_APP) | $(DIST_DIR)
	rm -f $@
	# Use ditto, not zip/7z: the app's resources live under Contents/MacOS, so
	# codesign signs them as nested code via extended attributes. Plain zip
	# (Info-ZIP) drops xattrs, which breaks the signature and makes a
	# quarantined copy report as "damaged and can't be opened". ditto is Apple's
	# archiver, is always present on macOS/CI runners, and preserves the
	# signing metadata so the .app stays validly signed after extraction.
	ditto -c -k --keepParent --sequesterRsrc --zlibCompressionLevel 9 "$<" "$@"

$(NIX_XZ): $(NIX_EXE)
	# TODO: switch to meson install
	mkdir -p "$(DIST_DIR)"
	$(eval TMP_DIR = $(DIST_DIR)/.tmp-nix)
	rm -rf $(TMP_DIR)
	mkdir -p $(TMP_DIR)/poptracker/packs
	cp -r api $(TMP_DIR)/poptracker/
	cp -r schema $(TMP_DIR)/poptracker/
	cp -r assets $(TMP_DIR)/poptracker/
	cp -r key $(TMP_DIR)/poptracker/
	cp LICENSE README.md CHANGELOG.md CREDITS.md $(TMP_DIR)/poptracker/
	cp $(NIX_EXE) $(TMP_DIR)/poptracker/
	rm -f $@
	(cd $(TMP_DIR) && \
	    tar -cJf ../$(notdir $@) poptracker \
	)
	rm -rf $(TMP_DIR)

test: $(TEST_EXE)  # automatically runs tests via Meson
bench: $(BENCH_EXE)  # automatically runs benchmarks via Meson
fuzzer: $(FUZZER_EXE)

clean:
	rm -rf "$(TMP_CROSS_WIN64_BUILD_DIR)"
	rm -rf "$(TMP_NATIVE_BUILD_DIR)"
	-rmdir "$(TMP_BUILD_BASE_DIR)"
	rm -rf "$(WIN32_EXE)" "$(WIN32_TEST_EXE)" "$(WIN32_FUZZER_EXE)" "$(WIN32_BENCH_EXE)"
	rm -rf "$(WIN64_EXE)" "$(WIN64_TEST_EXE)" "$(WIN64_FUZZER_EXE)" "$(WIN64_BENCH_EXE)"
	rm -rf "$(NIX_EXE)" "$(NIX_TEST_EXE)" "$(NIX_FUZZER_EXE)" "$(NIX_BENCH_EXE)"
	-rmdir "$(NIX_BUILD_DIR)"
	-rmdir "$(WIN32_BUILD_DIR)"
	-rmdir "$(WIN64_BUILD_DIR)"
	-rmdir "$(BUILD_DIR)"
	-rmdir "$(DIST_DIR)"
