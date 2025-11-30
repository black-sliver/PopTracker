# input
# NOTE: we may use gifdec in the future to support animations
#       for now we use SDL2_image GIF support only
CONF ?= RELEASE # make CONF=DEBUG for debug, CONF=DIST for .zip
MACOS_DEPLOYMENT_TARGET ?= 10.15
SRC_DIR = src
TEST_DIR = test
BENCH_DIR = bench
LIB_DIR = lib
SRC = $(wildcard $(SRC_DIR)/*.cpp) \
      $(wildcard $(SRC_DIR)/uilib/*.cpp) \
      $(wildcard $(SRC_DIR)/ui/*.cpp) \
      $(wildcard $(SRC_DIR)/core/*.cpp) \
      $(wildcard $(SRC_DIR)/appupdater/*.cpp) \
      $(wildcard $(SRC_DIR)/gh/api/*.cpp) \
      $(wildcard $(SRC_DIR)/luasandbox/*.cpp) \
      $(wildcard $(SRC_DIR)/usb2snes/*.cpp) \
      $(wildcard $(SRC_DIR)/uat/*.cpp) \
      $(wildcard $(SRC_DIR)/ap/*.cpp) \
      $(wildcard $(SRC_DIR)/luaconnector/*.cpp) \
      $(wildcard $(SRC_DIR)/http/*.cpp) \
      $(wildcard $(SRC_DIR)/packmanager/*.cpp) \
      $(LIB_DIR)/fmt/src/format.cc \
      #$(LIB_DIR)/gifdec/gifdec.c)
HDR = $(wildcard $(SRC_DIR)/*.h) \
      $(wildcard $(SRC_DIR)/uilib/*.h) \
      $(wildcard $(SRC_DIR)/ui/*.h) \
      $(wildcard $(SRC_DIR)/core/*.h) \
      $(wildcard $(SRC_DIR)/appupdater/*.hpp) \
      $(wildcard $(SRC_DIR)/gh/api/*.hpp) \
      $(wildcard $(SRC_DIR)/luasandbox/*.h) \
      $(wildcard $(SRC_DIR)/usb2snes/*.h) \
      $(wildcard $(SRC_DIR)/uat/*.h) \
      $(wildcard $(SRC_DIR)/ap/*.h) \
      $(wildcard $(SRC_DIR)/luaconnector/*.h) \
      $(wildcard $(SRC_DIR)/http/*.h) \
      $(wildcard $(SRC_DIR)/packmanager/*.h) \
      $(wildcard $(LIB_DIR)/fmt/include/fmt/*.h) \
      $(wildcard $(LIB_DIR)/tinyfiledialogs/*)
TEST_SRC = $(filter-out $(SRC_DIR)/main.cpp,$(SRC)) \
      $(wildcard $(TEST_DIR)/*.cpp) \
      $(wildcard $(TEST_DIR)/*/*.cpp) \
      $(wildcard $(TEST_DIR)/*/*/*.cpp)
TEST_HDR = $(wildcard $(TEST_DIR)/*.h) \
      $(wildcard $(TEST_DIR)/*/*.h) \
      $(wildcard $(TEST_DIR)/*/*/*.hpp)
BENCH_SRC = $(filter-out $(SRC_DIR)/main.cpp,$(SRC)) \
      $(wildcard $(BENCH_DIR)/*.cpp) \
      $(wildcard $(BENCH_DIR)/*/*.cpp) \
      $(wildcard $(LIB_DIR)/sltbench/src/*.cpp)
INCLUDE_DIRS = -Ilib -Ilib/lua -Ilib/asio/include -DASIO_STANDALONE -Ilib/miniz -Ilib/json/include -Ilib/valijson/include -Ilib/tinyfiledialogs -Ilib/wswrap/include -Ilib/sltbench/include \
 -Ilib/fmt/include \
 #-Ilib/gifdec
WIN32_INCLUDE_DIRS =  -DWIN32_LEAN_AND_MEAN -Iwin32-lib/i686/include -Ilib/gmock-win32/include
WIN32_LIB_DIRS = -L./win32-lib/i686/bin -L./win32-lib/i686/lib
WIN64_INCLUDE_DIRS = -DWIN32_LEAN_AND_MEAN -Iwin32-lib/x86_64/include -Ilib/gmock-win32/include
WIN64_LIB_DIRS = -L./win32-lib/x86_64/bin -L./win32-lib/x86_64/lib
SSL_LIBS = -lssl -lcrypto
NIX_LIBS = -lSDL2_ttf -lSDL2_image $(SSL_LIBS) -lz
WIN32_LIBS = -D_WIN32_WINNT=0x0502 -lmingw32 -lSDL2main -lSDL2 -mwindows -lSDL2_image -lSDL2_ttf $(SSL_LIBS) -lm -lz -lwsock32 -lws2_32 -ldinput8 -ldxguid -ldxerr8 -luser32 -lusp10 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -lhid -lsetupapi -lfreetype -lbz2 -lpng -luuid -lrpcrt4 -lcrypt32 -lssp -lcrypt32 -static-libgcc
WIN64_LIBS = -D_WIN32_WINNT=0x0502 -lmingw32 -lSDL2main -lSDL2 -mwindows -Wl,--no-undefined -Wl,--dynamicbase -Wl,--nxcompat -lSDL2_image -lSDL2_ttf $(SSL_LIBS) -lm -lz -lwsock32 -lws2_32 -ldinput8 -ldxguid -ldxerr8 -luser32 -lusp10 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -lhid -lsetupapi -lfreetype -lbz2 -lpng -luuid -lrpcrt4 -lcrypt32 -lssp -lcrypt32 -static-libgcc -Wl,--high-entropy-va

CACERT_URL = https://curl.se/ca/cacert.pem
CACERT_SHA256_URL = https://curl.se/ca/cacert.pem.sha256

# extract version
VERSION_MAJOR := $(shell grep '.define APP_VERSION_MAJOR' $(SRC_DIR)/version.h | rev | cut -d' ' -f 1 | rev )
VERSION_MINOR := $(shell grep '.define APP_VERSION_MINOR' $(SRC_DIR)/version.h | rev | cut -d' ' -f 1 | rev )
VERSION_REVISION := $(shell grep '.define APP_VERSION_REVISION' $(SRC_DIR)/version.h | rev | cut -d' ' -f 1 | rev )
VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_REVISION)
VERSION_NO_EXTRA := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_REVISION)
VS := $(subst .,-,$(VERSION))
$(info Version   $(VERSION))
$(info w/o extra $(VERSION_NO_EXTRA))

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
  else
    IS_LINUX = yes
  endif
endif
ifneq '' '$(findstring clang,$(CC))'
  IS_LLVM = yes
endif

ifdef IS_WIN
  TEST_SRC += $(wildcard $(LIB_DIR)/gmock-win32/src/*.cpp)
  TEST_HDR += $(wildcard $(LIB_DIR)/gmock-win32/include/*.h)
endif

# output
ARCH = $(shell uname -m | tr -s ' ' '-' | tr A-Z a-z)
UNAME = $(shell uname -sm | tr -s ' ' '-' | tr A-Z a-z )
DIST_DIR ?= dist
BUILD_DIR ?= build
EXE_NAME = poptracker
TEST_EXE_NAME = poptracker-test
BENCH_EXE_NAME = poptracker-benchmark
NIX_BUILD_DIR = $(BUILD_DIR)/$(UNAME)
WIN32_BUILD_DIR = $(BUILD_DIR)/win32
WIN64_BUILD_DIR = $(BUILD_DIR)/win64
WASM_BUILD_DIR = $(BUILD_DIR)/wasm
WIN32_EXE = $(WIN32_BUILD_DIR)/$(EXE_NAME).exe
WIN32_TEST_EXE = $(WIN32_BUILD_DIR)/$(TEST_EXE_NAME).exe
WIN32_BENCH_EXE = $(WIN32_BUILD_DIR)/$(BENCH_EXE_NAME).exe
WIN64_EXE = $(WIN64_BUILD_DIR)/$(EXE_NAME).exe
WIN64_TEST_EXE = $(WIN64_BUILD_DIR)/$(TEST_EXE_NAME).exe
WIN64_BENCH_EXE = $(WIN64_BUILD_DIR)/$(BENCH_EXE_NAME).exe
NIX_EXE = $(NIX_BUILD_DIR)/$(EXE_NAME)
NIX_TEST_EXE = $(NIX_BUILD_DIR)/$(TEST_EXE_NAME)
NIX_BENCH_EXE = $(NIX_BUILD_DIR)/$(BENCH_EXE_NAME)
HTML = $(WASM_BUILD_DIR)/$(EXE_NAME).html

# dist/zip
ifeq ($(CONF), DIST)
ifdef IS_OSX
  OSX_APP := $(NIX_BUILD_DIR)/poptracker.app
  OSX_ZIP := $(DIST_DIR)/poptracker_$(VS)_macos.zip
else ifdef IS_LINUX
  DISTRO = $(shell lsb_release -si | tr -s ' ' '-' | tr A-Z a-z )
  DISTRO_VERSION = $(shell lsb_release -sr | tr -s '.' '-' | tr A-z a-z )
  NIX_XZ := $(DIST_DIR)/poptracker_$(VS)_$(DISTRO)-$(DISTRO_VERSION)-$(ARCH).tar.xz
endif
WIN32_ZIP := $(DIST_DIR)/poptracker_$(VS)_win32.zip
WIN64_ZIP := $(DIST_DIR)/poptracker_$(VS)_win64.zip
endif

# fragments
NIX_OBJ := $(patsubst %.cc, $(NIX_BUILD_DIR)/%.o, \
           $(patsubst %.cpp, $(NIX_BUILD_DIR)/%.o, $(SRC)))
NIX_TEST_OBJ := $(patsubst %.cc, $(NIX_BUILD_DIR)/%.o, \
                $(patsubst %.cpp, $(NIX_BUILD_DIR)/%.o, $(TEST_SRC)))
NIX_BENCH_OBJ := $(patsubst %.cc, $(NIX_BUILD_DIR)/%.o, \
                 $(patsubst %.cpp, $(NIX_BUILD_DIR)/%.o, $(BENCH_SRC)))
NIX_OBJ_DIRS := $(sort $(dir $(NIX_OBJ)) $(dir $(NIX_TEST_OBJ)) $(dir $(NIX_BENCH_OBJ)))

WIN32_OBJ := $(patsubst %.cc, $(WIN32_BUILD_DIR)/%.o, \
             $(patsubst %.cpp, $(WIN32_BUILD_DIR)/%.o, $(SRC)))
WIN32_TEST_OBJ := $(patsubst %.cc, $(WIN32_BUILD_DIR)/%.o, \
                  $(patsubst %.cpp, $(WIN32_BUILD_DIR)/%.o, $(TEST_SRC)))
WIN32_BENCH_OBJ := $(patsubst %.cc, $(WIN32_BUILD_DIR)/%.o, \
                   $(patsubst %.cpp, $(WIN32_BUILD_DIR)/%.o, $(BENCH_SRC)))
WIN32_OBJ_DIRS := $(sort $(dir $(WIN32_OBJ)) $(dir $(WIN32_TEST_OBJ)) $(dir $(WIN32_BENCH_OBJ)))

WIN64_OBJ := $(patsubst %.cc, $(WIN64_BUILD_DIR)/%.o, \
             $(patsubst %.cpp, $(WIN64_BUILD_DIR)/%.o, $(SRC)))
WIN64_TEST_OBJ := $(patsubst %.cc, $(WIN64_BUILD_DIR)/%.o, \
                  $(patsubst %.cpp, $(WIN64_BUILD_DIR)/%.o, $(TEST_SRC)))
WIN64_BENCH_OBJ := $(patsubst %.cc, $(WIN64_BUILD_DIR)/%.o, \
                   $(patsubst %.cpp, $(WIN64_BUILD_DIR)/%.o, $(BENCH_SRC)))
WIN64_OBJ_DIRS := $(sort $(dir $(WIN64_OBJ)) $(dir $(WIN64_TEST_OBJ)) $(dir $(WIN64_BENCH_OBJ)))

# tools
CC ?= gcc
CXX ?= g++
AR ?= ar

ifeq ($(CC), cc)
CC = gcc
CPP = $(CC) -E
endif
ifeq ($(CXX), cpp)
CXX = g++
endif
ifeq ($(AR), ar)
AR = ar
endif

$(info using CC=${CC})
$(info using CPP=${CPP})
$(info using CXX=${CXX})
$(info using AR=${AR})

# cross tools
EMCC ?= emcc
EMPP ?= em++
EMAR ?= emar
WIN32CC = i686-w64-mingw32-gcc
WIN32CPP = i686-w64-mingw32-g++
WIN32AR = i686-w64-mingw32-ar
WIN32STRIP = i686-w64-mingw32-strip
WIN32WINDRES = i686-w64-mingw32-windres
WIN64CC = x86_64-w64-mingw32-gcc
WIN64CPP = x86_64-w64-mingw32-g++
WIN64AR = x86_64-w64-mingw32-ar
WIN64STRIP = x86_64-w64-mingw32-strip
WIN64WINDRES = x86_64-w64-mingw32-windres

# tool config
LTO_JOBS ?= $(patsubst -j%,%,$(filter -j%,$(MAKEFLAGS)))
LTO_JOBS := $(if $(LTO_JOBS),$(LTO_JOBS),4)
COMMON_WARNING_FLAGS = \
	-Wall -Wextra -Werror # -Wshadow -Wconversion
C_FLAGS = $(COMMON_WARNING_FLAGS) -Wshadow -std=c99 -D_REENTRANT
LUA_C_FLAGS = $(COMMON_WARNING_FLAGS) -Wshadow \
	-D_REENTRANT -x c++ # we actually use C++ for Lua now
CPP_FLAGS = $(COMMON_WARNING_FLAGS) \
	-Wnon-virtual-dtor -Wno-unused-function -Wno-deprecated-declarations \
	-Wno-null-pointer-subtraction -Wno-shift-count-overflow  # TODO: fix those
ifeq ($(CONF), DEBUG) # DEBUG
  C_FLAGS += -Og -g -fno-omit-frame-pointer -fstack-protector-strong -fno-common -ftrapv
  LUA_C_FLAGS += -Og -g -fno-omit-frame-pointer -fstack-protector-strong -fno-common -DLUA_USE_APICHECK -DLUAI_ASSERT -ftrapv
  ifdef IS_LLVM # DEBUG with LLVM
    CPP_FLAGS += -fstack-protector-strong -g -Og -ffunction-sections -fdata-sections -pthread -fno-omit-frame-pointer
    LD_FLAGS = -Wl,-dead_strip -fstack-protector-strong -pthread -fno-omit-frame-pointer
    ifdef IS_LINUX # clang calls regular LD on linux
      LD_FLAGS = -Wl,--gc-sections -fstack-protector-strong -pthread -fno-omit-frame-pointer
    endif
  else # DEBUG with GCC
    CPP_FLAGS += -fstack-protector-strong -g -Og -ffunction-sections -fdata-sections -pthread -fno-omit-frame-pointer
    LD_FLAGS = -Wl,--gc-sections -fstack-protector-strong -pthread -fno-omit-frame-pointer
  endif
else # RELEASE or DIST
  C_FLAGS += -fstack-protector-strong -O2 -fno-common -DNDEBUG
  LUA_C_FLAGS += -fstack-protector-strong -O2 -fno-common -DNDEBUG
  ifdef IS_LLVM # RELEASE or DIST with LLVM
    CPP_FLAGS += -fstack-protector-strong -O2 -ffunction-sections -fdata-sections -DNDEBUG -flto -pthread -g
    LD_FLAGS = -Wl,-dead_strip -fstack-protector-strong -O2 -flto
    ifdef IS_LINUX # clang calls regular LD on linux
      LD_FLAGS = -Wl,--gc-sections -O2 -s -flto=$(LTO_JOBS) -pthread
    endif
  else # RELEASE or DIST with GCC
    CPP_FLAGS += -fstack-protector-strong -O2 -s -ffunction-sections -fdata-sections -DNDEBUG -flto=$(LTO_JOBS) -pthread
    LD_FLAGS = -Wl,--gc-sections -fstack-protector-strong -O2 -s -flto=$(LTO_JOBS) -pthread
  endif
endif

ifndef IS_LLVM
LUA_C_FLAGS += -Wno-error=maybe-uninitialized
endif

ifndef IS_OSX
C_FLAGS += -fstack-clash-protection
CPP_FLAGS += -fstack-clash-protection
LUA_C_FLAGS += -fstack-clash-protection
endif

ifdef WITH_ASAN
CPP_FLAGS += -fsanitize=address
LD_FLAGS += -fsanitize=address
endif

ifdef WITH_UBSAN
CPP_FLAGS += -fsanitize=undefined
LD_FLAGS += -fsanitize=undefined
endif

CPP_FLAGS += -DLUA_CPP

# os-specific tool config
WIN32_CPP_FLAGS = $(CPP_FLAGS) -D_WIN32_WINNT=0x0601
WIN64_CPP_FLAGS = $(CPP_FLAGS) -D_WIN32_WINNT=0x0601
NIX_CPP_FLAGS = $(CPP_FLAGS)
WIN32_LD_FLAGS = $(LD_FLAGS)
WIN64_LD_FLAGS = $(LD_FLAGS)
NIX_LD_FLAGS = $(LD_FLAGS)
NIX_C_FLAGS = $(C_FLAGS) -DLUA_USE_READLINE -DLUA_USE_LINUX
NIX_LUA_C_FLAGS = $(LUA_C_FLAGS) -DLUA_USE_READLINE -DLUA_USE_LINUX

ifdef IS_OSX
BREW_PREFIX := $(shell brew --prefix)
DEPLOYMENT_TARGET = $(MACOS_DEPLOYMENT_TARGET)
DEPLOYMENT_TARGET_MAJOR := $(shell echo $(DEPLOYMENT_TARGET) | cut -f1 -d.)
DEPLOYMENT_TARGET_MINOR := $(shell echo $(DEPLOYMENT_TARGET) | cut -f2 -d.)
# NOTE: on macos before 10.15 we have to use boost::filesystem instead of std::filesystem
#       This is currently not built automatically. Please open an issue if you really need a build for macos<10.15.
HAS_STD_FILESYSTEM := $(shell [ $(DEPLOYMENT_TARGET_MAJOR) -gt 10 -o \( $(DEPLOYMENT_TARGET_MAJOR) -eq 10 -a $(DEPLOYMENT_TARGET_MINOR) -ge 15 \) ] && echo true)
NIX_CPP_FLAGS += -mmacosx-version-min=$(DEPLOYMENT_TARGET) -I$(BREW_PREFIX)/opt/openssl@3.0/include -I$(BREW_PREFIX)/include
NIX_LD_FLAGS += -mmacosx-version-min=$(DEPLOYMENT_TARGET) -L$(BREW_PREFIX)/opt/openssl@3.0/lib -L$(BREW_PREFIX)/lib
NIX_C_FLAGS += -mmacosx-version-min=$(DEPLOYMENT_TARGET) -I$(BREW_PREFIX)/opt/openssl@3.0/include -I$(BREW_PREFIX)/include
NIX_LUA_C_FLAGS += -mmacosx-version-min=$(DEPLOYMENT_TARGET) -I$(BREW_PREFIX)/opt/openssl@3.0/include -I$(BREW_PREFIX)/include
else
HAS_STD_FILESYSTEM ?= true
endif

ifeq ($(HAS_STD_FILESYSTEM), true)
$(info using std::filesystem)
else
$(info using boost::filesystem)
NIX_LD_FLAGS += -lboost_filesystem
NIX_CPP_FLAGS += -DNO_STD_FILESYSTEM
endif

ifeq ($(CONF), DEBUG) # DEBUG
WINDRES_FLAGS =
else
WINDRES_FLAGS = -DNDEBUG
endif

# default target: "native"
ifdef IS_WIN32
  EXE = $(WIN32_EXE)
  TEST_EXE = $(WIN32_TEST_EXE)
  BENCH_EXE = $(WIN32_BENCH_EXE)
  WIN32CC = $(CC)
  WIN32CPP = $(CXX)
  WIN32AR = $(AR)
  WIN32STRIP = strip
  WIN32WINDRES = windres
  # MSYS' SDL_* configure is a bloat and links full static
  WIN32_LIBS += `pkg-config --libs SDL2_image libpng libjpeg libwebp SDL2_ttf freetype2 harfbuzz` -ldwrite -ltiff -lLerc -lbrotlidec -lbrotlicommon -lfreetype -lgraphite2 -llzma -lz -lwebp -lzstd -ldeflate -ljbig -ljpeg -lrpcrt4 -ljxl -lhwy -lsharpyuv -lavif -lyuv -ldav1d -lrav1e -lSvtAv1Enc -laom -lntdll -lwebpdemux
  ifeq ($(CONF), DIST)
    native: $(WIN32_ZIP)
  else
    native: $(WIN32_EXE)
  endif
else ifdef IS_WIN64
  EXE = $(WIN64_EXE)
  TEST_EXE = $(WIN64_TEST_EXE)
  BENCH_EXE = $(WIN64_BENCH_EXE)
  WIN64CC = $(CC)
  WIN64CPP = $(CXX)
  WIN64AR = $(AR)
  WIN64STRIP = strip
  WIN64WINDRES = windres
  # MSYS' SDL_* configure is a bloat and links full static
  WIN64_LIBS += `pkg-config --libs SDL2_image libpng libjpeg libwebp SDL2_ttf freetype2 harfbuzz` -ldwrite -ltiff -lLerc -lbrotlidec -lbrotlicommon -lfreetype -lgraphite2 -llzma -lz -lwebp -lzstd -ldeflate -ljbig -ljpeg -lrpcrt4 -ljxl -lhwy -lsharpyuv -lavif -lyuv -ldav1d -lrav1e -lSvtAv1Enc -laom -lntdll -lwebpdemux
  ifeq ($(CONF), DIST)
    native: $(WIN64_ZIP)
  else
    native: $(WIN64_EXE)
  endif
else ifdef IS_OSX
  EXE = $(NIX_EXE)
  TEST_EXE = $(NIX_TEST_EXE)
  BENCH_EXE = $(NIX_BENCH_EXE)
  ifeq ($(CONF), DIST) # TODO dmg?
    native: $(OSX_APP) $(OSX_ZIP) test_osx_app
  else
    native: $(NIX_EXE)
  endif
else
  WIN32_LIBS += -lbrotlidec-static -lbrotlicommon-static # brotli is a mess
  WIN64_LIBS +=  -lbrotlidec-static -lbrotlicommon-static
  EXE = $(NIX_EXE)
  TEST_EXE = $(NIX_TEST_EXE)
  BENCH_EXE = $(NIX_BENCH_EXE)
  ifeq ($(CONF), DIST) # TODO deb?
    native: $(NIX_XZ)
  else
    native: $(NIX_EXE)
  endif
endif

.PHONY: all native cross wasm clean test_osx_app commandline update-cacert assets/cacert.pem bench
all: native cross wasm commandline
wasm: $(HTML)
commandline: doc/commandline.txt
update-cacert: assets/cacert.pem

ifeq ($(CONF), DIST)
cross: $(WIN32_ZIP) $(WIN64_ZIP)
else
cross: $(WIN32_EXE) $(WIN64_EXE)
endif
cross-test: $(WIN32_TEST_EXE) $(WIN64_TEST_EXE)
	# TODO: run tests with wine

# Project Targets
assets/cacert.pem:
	(cd assets && \
	    if [ -x "`which curl`" ]; then \
	        curl --etag-compare ../.cacert.sha256.etag \
	             --etag-save ../.cacert.sha256.etag \
	             -o '$(notdir $@).sha256' '$(CACERT_SHA256_URL)' && \
	        if [ -f '$(notdir $@).sha256' ]; then \
	            sha256sum -c '$(notdir $@).sha256' || ( \
	                curl -o '$(notdir $@)' '$(CACERT_URL)' && \
	                sha256sum -c '$(notdir $@).sha256' \
	            ) \
	        fi \
	    else \
	        wget -O '$(notdir $@).sha256' '$(CACERT_SHA256_URL)' && \
	        sha256sum -c '$(notdir $@).sha256' || ( \
	            wget -O '$(notdir $@)' '$(CACERT_URL)' && \
	            sha256sum -c '$(notdir $@).sha256' \
	        ) \
	    fi \
	)
	rm -f '$@.sha256'

doc/commandline.txt: $(EXE)
	$(EXE) --help | sed 's@$(EXE)@poptracker@' > $@

$(HTML): $(SRC) $(WASM_BUILD_DIR)/liblua.a $(HDR) | $(WASM_BUILD_DIR)
    # TODO: add preloads as dependencies
	# -s ASSERTIONS=1 
	# -fexceptions is required for fixing up jsonc->json
	$(EMPP) $(SRC) $(WASM_BUILD_DIR)/liblua.a -std=c++17 -fexceptions $(INCLUDE_DIRS) -Os -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s SDL2_IMAGE_FORMATS='["png","gif"]' -s ALLOW_MEMORY_GROWTH=1 --preload-file assets --preload-file packs -o $@

$(NIX_EXE): $(NIX_OBJ) $(NIX_BUILD_DIR)/liblua.a $(HDR) | $(NIX_BUILD_DIR)
	$(CXX) -std=c++1z $(NIX_OBJ) $(NIX_BUILD_DIR)/liblua.a -ldl $(NIX_LD_FLAGS) `sdl2-config --libs` $(NIX_LIBS) -o $@

$(NIX_TEST_EXE): $(NIX_TEST_OBJ) $(NIX_BUILD_DIR)/liblua.a $(HDR) | $(NIX_BUILD_DIR)
	$(CXX) -std=c++1z $(NIX_TEST_OBJ) -l gtest -l gmock $(NIX_BUILD_DIR)/liblua.a -ldl $(NIX_LD_FLAGS) `sdl2-config --libs` $(NIX_LIBS) -o $@

$(NIX_BENCH_EXE): $(NIX_BENCH_OBJ) $(NIX_BUILD_DIR)/liblua.a $(HDR) | $(NIX_BUILD_DIR)
	$(CXX) -std=c++1z $(NIX_BENCH_OBJ) $(NIX_BUILD_DIR)/liblua.a -ldl $(NIX_LD_FLAGS) `sdl2-config --libs` $(NIX_LIBS) -o $@

$(WIN32_EXE): $(WIN32_OBJ) $(WIN32_BUILD_DIR)/app.res $(WIN32_BUILD_DIR)/liblua.a $(HDR) | $(WIN32_BUILD_DIR)
# FIXME: static 32bit exe does not work for some reason
	$(WIN32CPP) -o $@ -std=c++17 -static -Wl,-Bstatic $(WIN32_OBJ) $(WIN32_BUILD_DIR)/app.res $(WIN32_BUILD_DIR)/liblua.a  $(WIN32_LIB_DIRS) $(WIN32_LD_FLAGS) $(WIN32_LIBS)
ifneq ($(CONF), DEBUG)
	$(WIN32STRIP) $@
endif

$(WIN32_TEST_EXE): $(WIN32_TEST_OBJ) $(WIN32_BUILD_DIR)/app.res $(WIN32_BUILD_DIR)/liblua.a $(HDR) | $(WIN32_BUILD_DIR)
	$(WIN32CPP) -o $@ -std=c++17 $(WIN32_TEST_OBJ) -l gtest -l gmock $(WIN32_BUILD_DIR)/liblua.a  $(WIN32_LIB_DIRS) $(WIN32_LD_FLAGS) $(WIN32_LIBS)

$(WIN32_BENCH_EXE): $(WIN32_BENCH_OBJ) $(WIN32_BUILD_DIR)/app.res $(WIN32_BUILD_DIR)/liblua.a $(HDR) | $(WIN32_BUILD_DIR)
	$(WIN32CPP) -o $@ -std=c++17 $(WIN32_BENCH_OBJ) $(WIN32_BUILD_DIR)/liblua.a  $(WIN32_LIB_DIRS) $(WIN32_LD_FLAGS) $(WIN32_LIBS)

$(WIN64_EXE): $(WIN64_OBJ) $(WIN64_BUILD_DIR)/app.res $(WIN64_BUILD_DIR)/liblua.a $(HDR) | $(WIN64_BUILD_DIR)
	$(WIN64CPP) -o $@ -std=c++17 -static -Wl,-Bstatic $(WIN64_OBJ) $(WIN64_BUILD_DIR)/app.res $(WIN64_BUILD_DIR)/liblua.a  $(WIN64_LIB_DIRS) $(WIN64_LD_FLAGS) $(WIN64_LIBS)
ifneq ($(CONF), DEBUG)
	$(WIN64STRIP) $@
endif

$(WIN64_TEST_EXE): $(WIN64_TEST_OBJ) $(WIN64_BUILD_DIR)/app.res $(WIN64_BUILD_DIR)/liblua.a $(HDR) | $(WIN64_BUILD_DIR)
	$(WIN64CPP) -o $@ -std=c++17 $(WIN64_TEST_OBJ) -l gtest -l gmock $(WIN64_BUILD_DIR)/liblua.a  $(WIN64_LIB_DIRS) $(WIN64_LD_FLAGS) $(WIN64_LIBS)

$(WIN64_BENCH_EXE): $(WIN64_BENCH_OBJ) $(WIN64_BUILD_DIR)/app.res $(WIN64_BUILD_DIR)/liblua.a $(HDR) | $(WIN64_BUILD_DIR)
	$(WIN64CPP) -o $@ -std=c++17 $(WIN64_BENCH_OBJ) -l gtest -l gmock $(WIN64_BUILD_DIR)/liblua.a  $(WIN64_LIB_DIRS) $(WIN64_LD_FLAGS) $(WIN64_LIBS)

$(WIN32_ZIP): $(WIN32_EXE) | $(DIST_DIR)
$(WIN64_ZIP): $(WIN64_EXE) | $(DIST_DIR)
$(WIN32_ZIP) $(WIN64_ZIP):
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
	./macosx/bundle_macosx_app.sh --version=$(VERSION) --deployment-target=$(DEPLOYMENT_TARGET) "$(NIX_EXE)"

test_osx_app: $(OSX_APP)
	# test that the app bundle is correctly build
	cd ./$(OSX_APP)/Contents/MacOS; ./poptracker --version

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

# Targets' dependencies
$(WASM_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WASM_BUILD_DIR)
	mkdir -p $(WASM_BUILD_DIR)/lib
	cp -R lib/lua $(WASM_BUILD_DIR)/lib/
	(cd $(WASM_BUILD_DIR)/lib/lua && make -f makefile a CC=$(EMPP) AR="$(EMAR) rc" CFLAGS="$(LUA_C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(WASM_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(WASM_BUILD_DIR)/lib/lua
$(NIX_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(NIX_BUILD_DIR)
	mkdir -p $(NIX_BUILD_DIR)/lib
	cp -R lib/lua $(NIX_BUILD_DIR)/lib/
	(cd $(NIX_BUILD_DIR)/lib/lua && make -f makefile a CC=$(CXX) AR="$(AR) rc" CFLAGS="$(NIX_LUA_C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(NIX_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(NIX_BUILD_DIR)/lib/lua
$(WIN32_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WIN32_BUILD_DIR)
	mkdir -p $(WIN32_BUILD_DIR)/lib
	cp -R lib/lua $(WIN32_BUILD_DIR)/lib/
	(cd $(WIN32_BUILD_DIR)/lib/lua && make -f makefile a CC=$(WIN32CPP) AR="$(WIN32AR) rc" CFLAGS="$(LUA_C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(WIN32_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(WIN32_BUILD_DIR)/lib/lua
$(WIN64_BUILD_DIR)/liblua.a: lib/lua/makefile lib/lua/luaconf.h | $(WIN64_BUILD_DIR)
	mkdir -p $(WIN64_BUILD_DIR)/lib
	cp -R lib/lua $(WIN64_BUILD_DIR)/lib/
	(cd $(WIN64_BUILD_DIR)/lib/lua && make -f makefile a CC=$(WIN64CPP) AR="$(WIN64AR) rc" CFLAGS="$(LUA_C_FLAGS)" MYCFLAGS="" MYLIBS="")
	mv $(WIN64_BUILD_DIR)/lib/lua/$(notdir $@) $@
	rm -rf $(WIN64_BUILD_DIR)/lib/lua

$(BUILD_DIR)/poptracker.exe.manifest: win32/exe.manifest.template.xml | $(BUILD_DIR)
	sed 's/{{VERSION}}/$(VERSION_NO_EXTRA).0/g;s/{{NAME}}/poptracker/g;s/{{DESCRIPTION}}/PopTracker/g' $< > $@

$(WIN32_BUILD_DIR)/app.res: $(SRC_DIR)/app.rc $(SRC_DIR)/version.h assets/icon.ico $(BUILD_DIR)/poptracker.exe.manifest | $(WIN32_BUILD_DIR)
	$(WIN32WINDRES) $(WINDRES_FLAGS) $< -O coff $@
$(WIN64_BUILD_DIR)/app.res: $(SRC_DIR)/app.rc $(SRC_DIR)/version.h assets/icon.ico $(BUILD_DIR)/poptracker.exe.manifest | $(WIN64_BUILD_DIR)
	$(WIN64WINDRES) $(WINDRES_FLAGS) $< -O coff $@

# Build dirs
$(BUILD_DIR):
	mkdir -p $@
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
$(NIX_BUILD_DIR)/test/%.o: %.c* $(HDR) $(TEST_HDR) | $(NIX_OBJ_DIRS)
	$(CXX) -std=c++1z $(INCLUDE_DIRS) $(NIX_CPP_FLAGS) `sdl2-config --cflags` -c $< -o $@
$(NIX_BUILD_DIR)/%.o: %.c* $(HDR) | $(NIX_OBJ_DIRS)
	$(CXX) -std=c++1z $(INCLUDE_DIRS) $(NIX_CPP_FLAGS) `sdl2-config --cflags` -c $< -o $@
$(WIN32_OBJ_DIRS): | $(WIN32_BUILD_DIR)
	mkdir -p $@
$(WIN32_BUILD_DIR)/test/%.o: %.c* $(HDR) $(TEST_HDR) | $(WIN32_OBJ_DIRS)
	$(WIN32CPP) -std=c++17 $(INCLUDE_DIRS) $(WIN32_INCLUDE_DIRS) $(WIN32_CPP_FLAGS) -D_REENTRANT -c $< -o $@
$(WIN32_BUILD_DIR)/%.o: %.c* $(HDR) | $(WIN32_OBJ_DIRS)
	$(WIN32CPP) -std=c++17 $(INCLUDE_DIRS) $(WIN32_INCLUDE_DIRS) $(WIN32_CPP_FLAGS) -D_REENTRANT -c $< -o $@
$(WIN64_OBJ_DIRS): | $(WIN64_BUILD_DIR)
	mkdir -p $@
$(WIN64_BUILD_DIR)/test/%.o: %.c* $(HDR) $(TEST_HDR) | $(WIN64_OBJ_DIRS)
	$(WIN64CPP) -std=c++17 $(INCLUDE_DIRS) $(WIN64_INCLUDE_DIRS) $(WIN64_CPP_FLAGS) -D_REENTRANT -c $< -o $@
$(WIN64_BUILD_DIR)/%.o: %.c* $(HDR) | $(WIN64_OBJ_DIRS)
	$(WIN64CPP) -std=c++17 $(INCLUDE_DIRS) $(WIN64_INCLUDE_DIRS) $(WIN64_CPP_FLAGS) -D_REENTRANT -c $< -o $@

# Avoid detection/auto-cleanup of intermediates
.OBJ_DIRS: $(NIX_OBJ_DIRS) $(WIN32_OBJ_DIRS) $(WIN64_OBJ_DIRS)

test: $(EXE) $(TEST_EXE)
	@echo "Running $(TEST_EXE)"
	@$(TEST_EXE)
	@echo "Checking $(EXE)"
ifdef IS_WIN
	@echo "VersionInfo: "
	-@powershell -NoLogo -NoProfile -Command "(Get-Item -Path '$(EXE)').VersionInfo | Format-List -Force"
endif
	@echo -n "Size: "
	@du -h $(EXE) | cut -f -1
	@echo -n "Version: "
	@timeout 5 $(EXE) --version
	@echo "HTTP Test ..."
	@timeout 9 $(EXE) --list-packs > /dev/null

bench: $(EXE) $(BENCH_EXE)
	@echo "Running $(BENCH_EXE)"
	@$(BENCH_EXE)

clean:
	(cd lib/lua && make -f makefile clean)
	rm -rf $(WASM_BUILD_DIR)/$(EXE_NAME){,.exe,.html,.js,.wasm,.data} $(WASM_BUILD_DIR)/*.a $(WASM_BUILD_DIR)/$(SRC_DIR) $(WASM_BUILD_DIR)/$(LIB_DIR) $(WASM_BUILD_DIR)/test $(WASM_BUILD_DIR)/bench
	rm -rf $(WIN32_EXE) $(WIN32_TEST_EXE) $(WIN32_BUILD_DIR)/app.res $(WIN32_BUILD_DIR)/*.a $(WIN32_BUILD_DIR)/$(SRC_DIR) $(WIN32_BUILD_DIR)/$(LIB_DIR) $(WIN32_BUILD_DIR)/test $(WIN32_BUILD_DIR)/bench
	rm -rf $(WIN64_EXE) $(WIN64_TEST_EXE) $(WIN64_BUILD_DIR)/app.res $(WIN64_BUILD_DIR)/*.a $(WIN64_BUILD_DIR)/$(SRC_DIR) $(WIN64_BUILD_DIR)/$(LIB_DIR) $(WIN64_BUILD_DIR)/test $(WIN64_BUILD_DIR)/bench
	rm -rf $(NIX_EXE) $(NEX_TEST_EXE) $(NIX_BUILD_DIR)/*.a $(NIX_BUILD_DIR)/$(SRC_DIR) $(NIX_BUILD_DIR)/$(LIB_DIR) $(NIX_BUILD_DIR)/test $(NIX_BUILD_DIR)/bench
	[ -d $(NIX_BUILD_DIR) ] && [ -z "${ls -A $(NIX_BUILD_DIR)}" ] && rmdir $(NIX_BUILD_DIR) || true
	[ -d $(WASM_BUILD_DIR) ] && [ -z "${ls -A $(WASM_BUILD_DIR)}" ] && rmdir $(WASM_BUILD_DIR) || true
	[ -d $(WIN32_BUILD_DIR) ] && [ -z "${ls -A $(WIN32_BUILD_DIR)}" ] && rmdir $(WIN32_BUILD_DIR) || true
	[ -d $(WIN64_BUILD_DIR) ] && [ -z "${ls -A $(WIN64_BUILD_DIR)}" ] && rmdir $(WIN64_BUILD_DIR) || true
	[ -d $(BUILD_DIR) ] && [ -z "${ls -A $(BUILD_DIR)}" ] && rmdir $(BUILD_DIR) || true
ifdef IS_OSX
	./macosx/bundle_macosx_app.sh --clear-thirdparty-dirs
endif

