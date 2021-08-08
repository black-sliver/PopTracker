# input
# NOTE: we may use gifdec in the future to support animations
#       for now we use SDL2_image GIF support only
CONF ?= RELEASE # make CONF=DEBUG for debug, CONF=DIST for .zip
SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.cpp) \
      $(wildcard $(SRC_DIR)/uilib/*.cpp) \
      $(wildcard $(SRC_DIR)/ui/*.cpp) \
      $(wildcard $(SRC_DIR)/core/*.cpp) \
      $(wildcard $(SRC_DIR)/usb2snes/*.cpp) #lib/gifdec/gifdec.c
HDR = $(wildcard $(SRC_DIR)/*.h) \
      $(wildcard $(SRC_DIR)/uilib/*.h) \
      $(wildcard $(SRC_DIR)/ui/*.h) \
      $(wildcard $(SRC_DIR)/core/*.h) \
      $(wildcard $(SRC_DIR)/usb2snes/*.h)
INCLUDE_DIRS = -Ilib -Ilib/lua -Ilib/asio/include -Ilib/miniz -Ilib/json/include #-Ilib/gifdec
WIN32_INCLUDE_DIRS = -Iwin32-lib/i686/include
WIN32_LIB_DIRS = -L./win32-lib/i686/bin -L./win32-lib/i686/lib
WIN64_INCLUDE_DIRS = -Iwin32-lib/x86_64/include
WIN64_LIB_DIRS = -L./win32-lib/x86_64/bin -L./win32-lib/x86_64/lib
WIN32_LIBS = -lmingw32 -lSDL2main -lSDL2 -mwindows -lm -lSDL2_image -lz -lwsock32 -lws2_32 -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -lhid -lsetupapi -lfreetype -lbz2 -lpng -lSDL2_ttf
WIN64_LIBS = -lmingw32 -lSDL2main -lSDL2 -mwindows -Wl,--no-undefined -Wl,--dynamicbase -Wl,--nxcompat -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -lSDL2_ttf -lSDL2_image -lwsock32 -lws2_32 -lfreetype -lpng -lz -lbz2 -lssp -static-libgcc -Wl,--high-entropy-va
# output
DISTRO = $(shell lsb_release -si | tr -s ' ' '-' | tr A-Z a-z )
ARCH = $(shell uname -m | tr -s ' ' '-' | tr A-Z a-z)
UNAME = $(shell uname -sm | tr -s ' ' '-' | tr A-Z a-z )
DIST_DIR ?= dist
BUILD_DIR ?= build
EXE_NAME = poptracker
NIX_BUILD_DIR = $(BUILD_DIR)/$(UNAME)
WIN32_BUILD_DIR = $(BUILD_DIR)/win32
WIN64_BUILD_DIR = $(BUILD_DIR)/win64
WASM_BUILD_DIR = $(BUILD_DIR)/wasm
WIN32_EXE = $(WIN32_BUILD_DIR)/$(EXE_NAME).exe
WIN64_EXE = $(WIN64_BUILD_DIR)/$(EXE_NAME).exe
NIX_EXE = $(NIX_BUILD_DIR)/$(EXE_NAME)
HTML = $(WASM_BUILD_DIR)/$(EXE_NAME).html
# dist/zip
ifeq ($(CONF), DIST)
VERSION := $(shell grep VERSION_STRING $(SRC_DIR)/poptracker.h | cut -d'"' -f 2 )
VS := $(subst .,-,$(VERSION))
WIN32_ZIP := $(DIST_DIR)/poptracker_$(VS)_win32.zip
WIN64_ZIP := $(DIST_DIR)/poptracker_$(VS)_win64.zip
OSX_APP := $(NIX_BUILD_DIR)/poptracker.app
OSX_ZIP := $(DIST_DIR)/poptracker_$(VS)_macos.zip
NIX_XZ := $(DIST_DIR)/poptracker_$(VS)_$(DISTRO)-$(ARCH).tar.xz
endif
# fragments
NIX_OBJ := $(patsubst %.cpp, $(NIX_BUILD_DIR)/%.o, $(SRC))
NIX_OBJ_DIRS := $(sort $(dir $(NIX_OBJ)))
WIN32_OBJ := $(patsubst %.cpp, $(WIN32_BUILD_DIR)/%.o, $(SRC))
WIN32_OBJ_DIRS := $(sort $(dir $(WIN32_OBJ)))
WIN64_OBJ := $(patsubst %.cpp, $(WIN64_BUILD_DIR)/%.o, $(SRC))
WIN64_OBJ_DIRS := $(sort $(dir $(WIN64_OBJ)))
# tools
CC = gcc # TODO: use ?=
CPP = g++ # TODO: use ?=
AR = ar # TODO: use ?=
# cross tools
EMCC ?= emcc
EMPP ?= em++
EMAR ?= emar
WIN32CC = i686-w64-mingw32-gcc
WIN32CPP = i686-w64-mingw32-g++
WIN32AR = i686-w64-mingw32-ar
WIN32STRIP = i686-w64-mingw32-strip
WIN64CC = x86_64-w64-mingw32-gcc
WIN64CPP = x86_64-w64-mingw32-g++
WIN64AR = x86_64-w64-mingw32-ar
WIN64STRIP = x86_64-w64-mingw32-strip

# detect OS and compiler
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
  endif
endif
ifneq '' '$(findstring clang,$(CC))'
  IS_LLVM = yes
endif

# tool config
#TODO: -fsanitize=address -fno-omit-frame-pointer ?
C_FLAGS = -Wall -std=c99 -D_REENTRANT
ifeq ($(CONF), DEBUG) # DEBUG
C_FLAGS += -Og -g -fno-omit-frame-pointer -fstack-protector-all -fno-common -DLUA_USE_APICHECK -DLUAI_ASSERT -ftrapv
ifdef IS_LLVM # DEBUG with LLVM
CPP_FLAGS = -Wall -Wnon-virtual-dtor -Wno-unused-function -fstack-protector-all -g -Og -ffunction-sections -fdata-sections -pthread -fno-omit-frame-pointer
LD_FLAGS = -Wl,-dead_strip -fstack-protector-all -pthread -fno-omit-frame-pointer
else # DEBUG with GCC
CPP_FLAGS = -Wall -Wnon-virtual-dtor -Wno-unused-function -fstack-protector-all -g -Og -ffunction-sections -fdata-sections -pthread -fno-omit-frame-pointer
LD_FLAGS = -Wl,--gc-sections -fstack-protector-all -pthread -fno-omit-frame-pointer
endif
else
C_FLAGS += -O2 -fno-stack-protector -fno-common
ifdef IS_LLVM # RELEASE or DIST with LLVM
CPP_FLAGS = -O2 -ffunction-sections -fdata-sections -DNDEBUG -flto -pthread
LD_FLAGS = -Wl,-dead_strip -O2 -s -flto
else # RELEASE or DIST with GCC
CPP_FLAGS = -O2 -s -ffunction-sections -fdata-sections -DNDEBUG -flto=8 -pthread
LD_FLAGS = -Wl,--gc-sections -O2 -s -flto=8 -pthread
endif
endif

# os-specific tool config
WIN32_CPP_FLAGS = $(CPP_FLAGS)
WIN64_CPP_FLAGS = $(CPP_FLAGS)
NIX_CPP_FLAGS = $(CPP_FLAGS)
WIN32_LD_FLAGS = $(LD_FLAGS)
WIN64_LD_FLAGS = $(LD_FLAGS)
NIX_LD_FLAGS = $(LD_FLAGS)
NIX_C_FLAGS = $(C_FLAGS) -DLUA_USE_READLINE -DLUA_USE_LINUX
ifdef IS_OSX
NIX_CPP_FLAGS += -mmacosx-version-min=10.12
NIX_LD_FLAGS += -mmacosx-version-min=10.12
NIX_C_FLAGS += -mmacosx-version-min=10.12
endif

# default target: "native"
ifdef IS_WIN32
  EXE = $(WIN32_EXE)
  WIN32CC = $(CC)
  WIN32CPP = $(CPP)
  WIN32AR = $(AR)
  WIN32STRIP = strip
  # MSYS' SDL_* configure is a bloat and links full static
  WIN32_LIBS += `pkg-config --libs SDL2_image libpng libjpeg libwebp SDL2_ttf freetype2 harfbuzz` -ltiff -lbrotlidec-static -lbrotlicommon-static -lfreetype -lgraphite2 -llzma -lz -lwebp -lzstd -ldeflate -ljbig -ljpeg -lrpcrt4
  ifeq ($(CONF), DIST)
    native: $(WIN32_ZIP)
  else
    native: $(WIN32_EXE)
  endif
else ifdef IS_WIN64
  EXE = $(WIN64_EXE)
  WIN64CC = $(CC)
  WIN64CPP = $(CPP)
  WIN64AR = $(AR)
  WIN64STRIP = strip
  # MSYS' SDL_* configure is a bloat and links full static
  WIN64_LIBS += `pkg-config --libs SDL2_image libpng libjpeg libwebp SDL2_ttf freetype2 harfbuzz` -ltiff -lbrotlidec-static -lbrotlicommon-static -lfreetype -lgraphite2 -llzma -lz -lwebp -lzstd -ldeflate -ljbig -ljpeg -lrpcrt4
  ifeq ($(CONF), DIST)
    native: $(WIN64_ZIP)
  else
    native: $(WIN64_EXE)
  endif
else ifdef IS_OSX
  EXE = $(NIX_EXE)
  ifeq ($(CONF), DIST) # TODO dmg?
    native: $(OSX_APP) $(OSX_ZIP)
  else
    native: $(NIX_EXE)
  endif
else
  EXE = $(NIX_EXE)
  ifeq ($(CONF), DIST) # TODO deb?
    native: $(NIX_XZ)
  else
    native: $(NIX_EXE)
  endif
endif

.PHONY: all native cross wasm clean
all: native cross wasm
wasm: $(HTML)

ifeq ($(CONF), DIST)
cross: $(WIN32_ZIP) $(WIN64_ZIP)
else
cross: $(WIN32_EXE) $(WIN64_EXE)
endif

# Project Targets
$(HTML): $(SRC) $(WASM_BUILD_DIR)/liblua.a $(HDR) | $(WASM_BUILD_DIR)
    # TODO: add preloads as dependencies
	# -s ASSERTIONS=1 
	# -fexceptions is required for fixing up jsonc->json
	$(EMPP) $(SRC) $(WASM_BUILD_DIR)/liblua.a -std=c++17 -fexceptions $(INCLUDE_DIRS) -Os -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s SDL2_IMAGE_FORMATS='["png","gif"]' -s ALLOW_MEMORY_GROWTH=1 --preload-file assets --preload-file packs -o $@

$(NIX_EXE): $(NIX_OBJ) $(NIX_BUILD_DIR)/liblua.a $(HDR) | $(NIX_BUILD_DIR)
	$(CPP) -std=c++1z $(NIX_OBJ) $(NIX_BUILD_DIR)/liblua.a -ldl $(NIX_LD_FLAGS) `sdl2-config --libs` -lSDL2_ttf -lSDL2_image -o $@

$(WIN32_EXE): $(WIN32_OBJ) $(WIN32_BUILD_DIR)/liblua.a $(HDR) | $(WIN32_BUILD_DIR)
# FIXME: static 32bit exe does not work for some reason
	$(WIN32CPP) -o $@ -std=c++17 $(WIN32_OBJ) $(WIN32_BUILD_DIR)/liblua.a  $(WIN32_LIB_DIRS) $(WIN32_LD_FLAGS) $(WIN32_LIBS)
ifneq ($(CONF), DEBUG)
	$(WIN32STRIP) $@
endif

$(WIN64_EXE): $(WIN64_OBJ) $(WIN64_BUILD_DIR)/liblua.a $(HDR) | $(WIN64_BUILD_DIR)
	$(WIN64CPP) -o $@ -std=c++17 -static -Wl,-Bstatic $(WIN64_OBJ) $(WIN64_BUILD_DIR)/liblua.a  $(WIN64_LIB_DIRS) $(WIN64_LD_FLAGS) $(WIN64_LIBS)
ifneq ($(CONF), DEBUG)
	$(WIN64STRIP) $@
endif

$(WIN32_ZIP): $(WIN32_EXE) | $(DIST_DIR)
$(WIN64_ZIP): $(WIN64_EXE) | $(DIST_DIR)
$(WIN32_ZIP) $(WIN64_ZIP):
	$(eval TGT = $(shell echo "$@" | rev | cut -d'_' -f 1 | rev | cut -d'.' -f 1))
	$(eval TMP_DIR = $(DIST_DIR)/.tmp-$(TGT))
	rm -rf $(TMP_DIR)
	mkdir -p $(TMP_DIR)/poptracker/packs
	cp -r assets $(TMP_DIR)/poptracker/
	cp LICENSE README.md CHANGELOG.md CREDITS.md $(TMP_DIR)/poptracker/
	cp $(dir $<)*.exe $(TMP_DIR)/poptracker/
	cp $(dir $<)*.dll $(TMP_DIR)/poptracker/ || true
	rm -f $@
	(cd $(TMP_DIR) && \
	    if [ -x "`which 7z`" ]; then 7z a -mx=9 ../$(notdir $@) poptracker ; \
	    else zip -9 -r ../$(notdir $@) poptracker ; fi && \
	    if [ -x "`which advzip`" ]; then advzip --recompress -4 ../$(notdir $@) ; fi \
	)
	rm -rf $(TMP_DIR)

$(OSX_APP): $(NIX_EXE)
	./macosx/bundle_macosx_app.sh --version=$(VERSION) "$(NIX_EXE)"
	
$(OSX_ZIP): $(OSX_APP) | $(DIST_DIR)
	rm -f $@
	(cd $(dir $<) && \
	    if [ -x "`which 7z`" ]; then 7z a -mx=9 ../../dist/$(notdir $@) $(notdir $<) -x!.DS_Store; \
	    else zip -9 -r ../../dist/$(notdir $@) $(notdir $<) -x "*.DS_Store" ; fi && \
	    if [ -x "`which advzip`" ]; then advzip --recompress -4 ../$(notdir $@) ; fi \
	)

$(NIX_XZ): $(NIX_EXE) | $(DIST_DIR)
	$(eval TMP_DIR = $(DIST_DIR)/.tmp-nix)
	rm -rf $(TMP_DIR)
	mkdir -p $(TMP_DIR)/poptracker/packs
	cp -r assets $(TMP_DIR)/poptracker/
	cp LICENSE README.md CHANGELOG.md CREDITS.md $(TMP_DIR)/poptracker/
	cp $(NIX_EXE) $(TMP_DIR)/poptracker/
	rm -f $@
	(cd $(TMP_DIR) && \
	    tar -cJf ../$(notdir $@) poptracker \
	)
	rm -rf $(TMP_DIR)

# Targets' dependencies
$(WASM_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WASM_BUILD_DIR)
	mkdir -p $(WASM_BUILD_DIR)/lib
	cp -R lib/lua $(WASM_BUILD_DIR)/lib/
	(cd $(WASM_BUILD_DIR)/lib/lua && make -f makefile a CC=$(EMCC) AR="$(EMAR) rc" CFLAGS="$(C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(WASM_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(WASM_BUILD_DIR)/lib
$(NIX_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(NIX_BUILD_DIR)
	mkdir -p $(NIX_BUILD_DIR)/lib
	cp -R lib/lua $(NIX_BUILD_DIR)/lib/
	(cd $(NIX_BUILD_DIR)/lib/lua && make -f makefile a CC=$(CC) AR="$(AR) rc" CFLAGS="$(NIX_C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(NIX_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(NIX_BUILD_DIR)/lib
$(WIN32_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WIN64_BUILD_DIR)
	mkdir -p $(WIN32_BUILD_DIR)/lib
	cp -R lib/lua $(WIN32_BUILD_DIR)/lib/
	(cd $(WIN32_BUILD_DIR)/lib/lua && make -f makefile a CC=$(WIN32CC) AR="$(WIN32AR) rc" CFLAGS="$(C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(WIN32_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(WIN32_BUILD_DIR)/lib
$(WIN64_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WIN64_BUILD_DIR)
	mkdir -p $(WIN64_BUILD_DIR)/lib
	cp -R lib/lua $(WIN64_BUILD_DIR)/lib/
	(cd $(WIN64_BUILD_DIR)/lib/lua && make -f makefile a CC=$(WIN64CC) AR="$(WIN64AR) rc" CFLAGS="$(C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(WIN64_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(WIN64_BUILD_DIR)/lib

# Build dirs
$(NIX_BUILD_DIR):
	mkdir -p $@
$(WASM_BUILD_DIR):
	mkdir -p $@
$(WIN32_BUILD_DIR):
	mkdir -p $@
$(WIN64_BUILD_DIR):
	mkdir -p $@

# Dist dir
$(DIST_DIR):
	mkdir -p $@

# Fragments
$(NIX_OBJ_DIRS): | $(NIX_BUILD_DIR)
	mkdir -p $@
$(NIX_BUILD_DIR)/%.o: %.cpp $(HDR) | $(NIX_OBJ_DIRS)
	$(CPP) -std=c++1z $(INCLUDE_DIRS) $(NIX_CPP_FLAGS) `sdl2-config --cflags` -c $< -o $@
$(WIN32_OBJ_DIRS): | $(WIN32_BUILD_DIR)
	mkdir -p $@
$(WIN32_BUILD_DIR)/%.o: %.cpp $(HDR) | $(WIN32_OBJ_DIRS)
	$(WIN32CPP) -std=c++17 $(INCLUDE_DIRS) $(WIN32_INCLUDE_DIRS) $(WIN32_CPP_FLAGS) -D_REENTRANT -c $< -o $@
$(WIN64_OBJ_DIRS): | $(WIN64_BUILD_DIR)
	mkdir -p $@
$(WIN64_BUILD_DIR)/%.o: %.cpp $(HDR) | $(WIN64_OBJ_DIRS)
	$(WIN64CPP) -std=c++17 $(INCLUDE_DIRS) $(WIN64_INCLUDE_DIRS) $(WIN64_CPP_FLAGS) -D_REENTRANT -c $< -o $@

# Avoid detection/auto-cleanup of intermediates
.OBJ_DIRS: $(NIX_OBJ_DIRS) $(WIN32_OBJ_DIRS) $(WIN64_OBJ_DIRS)

test: $(EXE)
# TODO: implement and run actual tests
	@echo "Testing $(EXE)"
	@du -h $(EXE) | cut -f -1
	@timeout 5 $(EXE) --version

clean:
	(cd lib/lua && make -f makefile clean)
	rm -rf $(WASM_BUILD_DIR)/$(EXE_NAME){,.exe,.html,.js,.wasm,.data} $(WASM_BUILD_DIR)/*.a
	rm -rf $(WIN32_EXE) $(WIN32_BUILD_DIR)/*.a $(WIN32_BUILD_DIR)/$(SRC_DIR)
	rm -rf $(WIN64_EXE) $(WIN64_BUILD_DIR)/*.a $(WIN64_BUILD_DIR)/$(SRC_DIR)
	rm -rf $(NIX_EXE) $(NIX_BUILD_DIR)/*.a $(NIX_BUILD_DIR)/$(SRC_DIR)
	if [ -d $(NIX_BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(NIX_BUILD_DIR) ; fi
	if [ -d $(WASM_BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(WASM_BUILD_DIR) ; fi
	if [ -d $(WIN32_BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(WIN32_BUILD_DIR) ; fi
	if [ -d $(WIN64_BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(WIN64_BUILD_DIR) ; fi
	if [ -d $(BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(BUILD_DIR) ; fi
