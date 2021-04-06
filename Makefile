# input
# NOTE: we may use gifdec in the future to support animations
#       for now we use SDL2_image GIF support only
CONF ?= RELEASE
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
WIN32_INCLUDE_DIRS = -Iwin32-lib/include
WIN32_LIB_DIRS = -L./win32-lib/bin -L./win32-lib/lib
WIN32_LIBS = -lmingw32 -lSDL2main -lSDL2 -mwindows -Wl,--no-undefined -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -static-libgcc -lSDL2_ttf -lSDL2_image -lwsock32 -lws2_32 -lfreetype -lz -lpng -lbz2 -lssp
# output
BUILD_DIR ?= build
EXE_NAME = poptracker
NIX_BUILD_DIR = $(BUILD_DIR)/native
WIN32_BUILD_DIR = $(BUILD_DIR)/win32
WASM_BUILD_DIR = $(BUILD_DIR)/wasm
WIN32_EXE = $(WIN32_BUILD_DIR)/$(EXE_NAME).exe
NIX_EXE = $(NIX_BUILD_DIR)/$(EXE_NAME)
HTML = $(WASM_BUILD_DIR)/$(EXE_NAME).html
# fragments
NIX_OBJ := $(patsubst %.cpp, $(NIX_BUILD_DIR)/%.o, $(SRC))
NIX_OBJ_DIRS := $(dir $(NIX_OBJ))
WIN32_OBJ := $(patsubst %.cpp, $(WIN32_BUILD_DIR)/%.o, $(SRC))
WIN32_OBJ_DIRS := $(dir $(WIN32_OBJ))
# tools
CC = gcc # TODO: use ?=
CPP = g++ # TODO: use ?=
AR = ar # TODO: use ?=
EMCC ?= emcc
EMPP ?= em++
EMAR ?= emar
WIN32CC = x86_64-w64-mingw32-gcc
WIN32CPP = x86_64-w64-mingw32-g++
WIN32AR = x86_64-w64-mingw32-ar
WIN32STRIP = x86_64-w64-mingw32-strip
# tool config
ifeq ($(CONF), DEBUG)
CPP_FLAGS = -Wall -Wno-unused-function -Og -g -ffunction-sections -fdata-sections -flto=8
LD_FLAGS = -Wl,-gc-sections
else # RELEASE
CPP_FLAGS = -O2 -s -ffunction-sections -fdata-sections -flto=8 -DNDEBUG
LD_FLAGS = -Wl,-gc-sections
endif

all: native cross wasm
.PHONY: all native cross wasm clean

wasm: $(HTML)

native: $(NIX_EXE)

cross: $(WIN32_EXE)

# Project Targets
$(HTML): $(SRC) $(WASM_BUILD_DIR)/liblua.a $(HDR) | $(WASM_BUILD_DIR)
    # TODO: add preloads as dependencies
	# -s ASSERTIONS=1 
	# -fexceptions is required for fixing up jsonc->json
	$(EMPP) $(SRC) $(WASM_BUILD_DIR)/liblua.a -std=c++17 -fexceptions $(INCLUDE_DIRS) -Os -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s SDL2_IMAGE_FORMATS='["png","gif"]' -s ALLOW_MEMORY_GROWTH=1 --preload-file assets --preload-file packs -o $@

$(NIX_EXE): $(NIX_OBJ) $(NIX_BUILD_DIR)/liblua.a $(HDR) | $(NIX_BUILD_DIR)
	$(CPP) -std=c++17 $(NIX_OBJ) $(NIX_BUILD_DIR)/liblua.a -ldl $(CPP_FLAGS) $(LD_FLAGS) `sdl2-config --libs` -lSDL2_ttf -lSDL2_image -pthread -o $@

$(WIN32_EXE): $(WIN32_OBJ) $(WIN32_BUILD_DIR)/liblua.a $(HDR) | $(WIN32_BUILD_DIR)
    # TODO: make zip a separate target
	$(WIN32CPP) -o $@ -std=c++17 -static -Wl,-Bstatic $(WIN32_OBJ) $(WIN32_BUILD_DIR)/liblua.a  $(WIN32_LIB_DIRS) $(CPP_FLAGS) $(LD_FLAGS) $(WIN32_LIBS)
	$(WIN32STRIP) $@
	mkdir -p dist/tmp-win32/poptracker/packs
	cp -r assets dist/tmp-win32/poptracker/
	cp LICENSE README.md dist/tmp-win32/poptracker/
	cp $(WIN32_BUILD_DIR)/*.exe $(WIN32_BUILD_DIR)/*.dll dist/tmp-win32/poptracker/
	rm -f dist/poptracker_win32_x86-64.zip
	(cd dist/tmp-win32 && \
	    if [ -x "`which 7z`" ]; then 7z a -mx=9 ../poptracker_win32_x86-64.zip poptracker ; \
	    else zip -9 -r ../poptracker_win32_x86-64.zip poptracker ; fi ; \
	    if [ -x "`which advzip`" ]; then advzip --recompress -4 ../poptracker_win32_x86-64.zip ; fi \
	)
	rm -rf dist/tmp-win32


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
$(WIN32_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WIN32_BUILD_DIR)
# TODO: set build path so we can build multiple configurations at once
	(cd lib/lua && make -f makefile a CC=$(WIN32CC) AR="$(WIN32AR) rc" MYCFLAGS="-Wall -std=c99" MYLIBS="")
	mv lib/lua/$(notdir $@) $@
	(cd lib/lua && make -f makefile clean)	

# Build dirs
$(NIX_BUILD_DIR):
	mkdir -p $@
$(WASM_BUILD_DIR):
	mkdir -p $@
$(WIN32_BUILD_DIR):
	mkdir -p $@

# Fragments
$(NIX_BUILD_DIR)/%/:
	mkdir -p $@
$(NIX_BUILD_DIR)/%.o: %.cpp $(HDR) | $(NIX_OBJ_DIRS)
	$(CPP) -std=c++17 -ldl $(INCLUDE_DIRS) $(CPP_FLAGS) `sdl2-config --cflags` -c $< -o $@
$(WIN32_BUILD_DIR)/%/:
	mkdir -p $@
$(WIN32_BUILD_DIR)/%.o: %.cpp $(HDR) | $(WIN32_OBJ_DIRS)
	$(WIN32CPP) -std=c++17 $(INCLUDE_DIRS) $(WIN32_INCLUDE_DIRS) $(CPP_FLAGS) -D_REENTRANT -c $< -o $@

# Avoid detection/auto-cleanup of intermediates
.OBJ_DIRS: $(NIX_OBJ_DIRS) $(WIN32_OBJ_DIRS)

clean:
	(cd lib/lua && make -f makefile clean)
	rm -rf $(WASM_BUILD_DIR)/$(EXE_NAME){,.exe,.html,.js,.wasm,.data} $(WASM_BUILD_DIR)/*.a
	rm -rf $(WIN32_EXE) $(WIN32_BUILD_DIR)/*.a $(WIN32_BUILD_DIR)/$(SRC_DIR)
	rm -rf $(NIX_EXE) $(NIX_BUILD_DIR)/*.a $(NIX_BUILD_DIR)/$(SRC_DIR)
	if [ -d $(NIX_BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(NIX_BUILD_DIR) ; fi
	if [ -d $(WASM_BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(WASM_BUILD_DIR) ; fi
	if [ -d $(WIN32_BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(WIN32_BUILD_DIR) ; fi
	if [ -d $(BUILD_DIR) ]; then rmdir --ignore-fail-on-non-empty $(BUILD_DIR) ; fi
