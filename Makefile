# input
# NOTE: we may use gifdec in the future to support animations
#       for now we use SDL2_image GIF support only
CONF ?= RELEASE # make CONF=DEBUG for debug, CONF=DIST for .zip
SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.cpp) \
      $(wildcard $(SRC_DIR)/uilib/*.cpp) \
      $(wildcard $(SRC_DIR)/ui/*.cpp) \
      $(wildcard $(SRC_DIR)/core/*.cpp) \
      $(wildcard $(SRC_DIR)/sd2snes/*.cpp) #lib/gifdec/gifdec.c
HDR = $(wildcard $(SRC_DIR)/*.h) \
      $(wildcard $(SRC_DIR)/uilib/*.h) \
      $(wildcard $(SRC_DIR)/ui/*.h) \
      $(wildcard $(SRC_DIR)/core/*.h) \
      $(wildcard $(SRC_DIR)/sd2snes/*.h)
INCLUDE_DIRS = -Ilib -Ilib/lua -Ilib/asio/include #-Ilib/gifdec
WIN32_INCLUDE_DIRS = -Iwin32-lib/i686/include
WIN32_LIB_DIRS = -L./win32-lib/i686/bin -L./win32-lib/i686/lib
WIN64_INCLUDE_DIRS = -Iwin32-lib/x86_64/include
WIN64_LIB_DIRS = -L./win32-lib/x86_64/bin -L./win32-lib/x86_64/lib
WIN32_LIBS = -lmingw32 -lSDL2main -lSDL2 -mwindows -lm -lSDL2_image -lz -lwsock32 -lws2_32 -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -lhid -lsetupapi -lfreetype -lbz2 -lpng -lSDL2_ttf
WIN64_LIBS = -lmingw32 -lSDL2main -lSDL2 -mwindows -Wl,--no-undefined -Wl,--dynamicbase -Wl,--nxcompat -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -lSDL2_ttf -lSDL2_image -lwsock32 -lws2_32 -lfreetype -lpng -lz -lbz2 -lssp -static-libgcc -Wl,--high-entropy-va
# output
DIST_DIR ?= dist
BUILD_DIR ?= build
EXE_NAME = poptracker
NIX_BUILD_DIR = $(BUILD_DIR)/$(shell uname -s -m | tr -s ' ' '-' | tr A-Z a-z )
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
# TODO OSX_APP := ...
WIN32_ZIP := $(DIST_DIR)/poptracker_$(VS)_win32.zip
WIN64_ZIP := $(DIST_DIR)/poptracker_$(VS)_win64.zip
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
# (default) tool config
ifeq ($(CONF), DEBUG) # DEBUG
ifdef IS_LLVM # DEBUG with LLVM
CPP_FLAGS = -Wall -Wno-unused-function -fstack-protector-all -g -Og -ffunction-sections -fdata-sections -pthread
LD_FLAGS = -Wl,-dead_strip -fstack-protector-all -pthread
else # DEBUG with GCC
CPP_FLAGS = -Wall -Wno-unused-function -fstack-protector-all -g -Og -ffunction-sections -fdata-sections -pthread
LD_FLAGS = -Wl,--gc-sections -fstack-protector-all -pthread
endif
else ifdef IS_LLVM # RELEASE, DIST with LLVM
CPP_FLAGS = -O2 -ffunction-sections -fdata-sections -DNDEBUG -flto -pthread
LD_FLAGS = -Wl,-dead_strip -O2 -s -flto
else # RELEASE, DIST with GCC
CPP_FLAGS = -O2 -s -ffunction-sections -fdata-sections -DNDEBUG -flto=8 -pthread
LD_FLAGS = -Wl,--gc-sections -O2 -s -flto=8 -pthread
endif
WIN32_CPP_FLAGS = $(CPP_FLAGS)
WIN64_CPP_FLAGS = $(CPP_FLAGS)
NIX_CPP_FLAGS = $(CPP_FLAGS)
WIN32_LD_FLAGS = $(LD_FLAGS)
WIN64_LD_FLAGS = $(LD_FLAGS)
NIX_LD_FLAGS = $(LD_FLAGS)

# default target: "native"
ifdef IS_WIN32
  EXE = $(WIN32_EXE)
  ifeq ($(CONF), DIST)
    native: $(WIN32_ZIP)
  else
    native: $(WIN32_EXE)
  endif
else ifdef IS_WIN64
  EXE = $(WIN64_EXE)
  ifeq ($(CONF), DIST)
    native: $(WIN64_ZIP)
  else
    native: $(WIN64_EXE)
  endif
else # TODO OSX .app
  EXE = $(NIX_EXE)
  native: $(NIX_EXE)
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
	$(CPP) -std=c++17 $(NIX_OBJ) $(NIX_BUILD_DIR)/liblua.a -ldl $(NIX_LD_FLAGS) `sdl2-config --libs` -lSDL2_ttf -lSDL2_image -o $@

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

$(WIN32_ZIP): $(WIN32_EXE)
	mkdir -p $(DIST_DIR)/tmp-win32/poptracker/packs
	cp -r assets $(DIST_DIR)/tmp-win32/poptracker/
	cp LICENSE README.md CHANGELOG.md $(DIST_DIR)/tmp-win32/poptracker/
	cp $(WIN32_BUILD_DIR)/*.exe $(WIN32_BUILD_DIR)/*.dll $(DIST_DIR)/tmp-win32/poptracker/
	rm -f $@
	(cd $(DIST_DIR)/tmp-win32 && \
	    if [ -x "`which 7z`" ]; then 7z a -mx=9 ../$(notdir $@) poptracker ; \
	    else zip -9 -r ../$(notdir $@) poptracker ; fi ; \
	    if [ -x "`which advzip`" ]; then advzip --recompress -4 ../$(notdir $@) ; fi \
	)
	rm -rf dist/tmp-win32

$(WIN64_ZIP): $(WIN64_EXE)
	mkdir -p $(DIST_DIR)/tmp-win64/poptracker/packs
	cp -r assets $(DIST_DIR)/tmp-win64/poptracker/
	cp LICENSE README.md CHANGELOG.md $(DIST_DIR)/tmp-win64/poptracker/
	cp $(WIN64_BUILD_DIR)/*.exe $(WIN64_BUILD_DIR)/*.dll $(DIST_DIR)/tmp-win64/poptracker/
	rm -f $@
	(cd $(DIST_DIR)/tmp-win64 && \
	    if [ -x "`which 7z`" ]; then 7z a -mx=9 ../$(notdir $@) poptracker ; \
	    else zip -9 -r ../$(notdir $@) poptracker ; fi ; \
	    if [ -x "`which advzip`" ]; then advzip --recompress -4 ../$(notdir $@) ; fi \
	)
	rm -rf dist/tmp-win64


# Targets' dependencies
$(WASM_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WASM_BUILD_DIR)
# TODO: set build path so we can build multiple configurations at once
	(cd lib/lua && make -f makefile a CC=$(EMCC) AR="$(EMAR) rc" MYCFLAGS="-Wall -std=c99" MYLIBS="")
	mv lib/lua/$(notdir $@) $@
	(cd lib/lua && make -f makefile clean)
$(NIX_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(NIX_BUILD_DIR)
# TODO: set build path so we can build multiple configurations at once
	(cd lib/lua && make -f makefile a CC=$(CC) AR="$(AR) rc")
	mv lib/lua/$(notdir $@) $@
	(cd lib/lua && make -f makefile clean)
$(WIN32_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WIN64_BUILD_DIR)
# TODO: set build path so we can build multiple configurations at once
	(cd lib/lua && make -f makefile a CC=$(WIN32CC) AR="$(WIN32AR) rc" MYCFLAGS="-Wall -std=c99" MYLIBS="")
	mv lib/lua/$(notdir $@) $@
	(cd lib/lua && make -f makefile clean)	
$(WIN64_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WIN64_BUILD_DIR)
# TODO: set build path so we can build multiple configurations at once
	(cd lib/lua && make -f makefile a CC=$(WIN64CC) AR="$(WIN64AR) rc" MYCFLAGS="-Wall -std=c99" MYLIBS="")
	mv lib/lua/$(notdir $@) $@
	(cd lib/lua && make -f makefile clean)	

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
	$(CPP) -std=c++17 $(INCLUDE_DIRS) $(NIX_CPP_FLAGS) `sdl2-config --cflags` -c $< -o $@
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
	timeout 5 $(EXE) --version

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
