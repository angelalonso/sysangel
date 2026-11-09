# Makefile for gamelauncher
#
#   make            -> native Linux binary  (needs libfltk1.3-dev / fltk-devel)
#   make win        -> gamelauncher.exe     (cross-compiled, fully static)
#   make clean      -> remove binaries
#   make distclean  -> also remove the downloaded/built FLTK for Windows
#
# Debian/Ubuntu prerequisites:
#   sudo apt install g++ make cmake curl mingw-w64 libfltk1.3-dev
# Arch:
#   sudo pacman -S gcc make cmake curl mingw-w64-gcc fltk

APP      := gamelauncher
SRC      := launcher.cpp

CXXFLAGS := -std=c++17 -Os -Wall -Wextra -ffunction-sections -fdata-sections

# ---------------------------------------------------------------- Linux ----
.PHONY: all linux win clean distclean
all: linux

linux: $(APP)

$(APP): $(SRC)
	$(CXX) $(CXXFLAGS) $$(fltk-config --cxxflags) $< -o $@ \
	    $$(fltk-config --ldflags) -Wl,--gc-sections -s

# --------------------------------------------------------------- Windows ---
MINGW      := x86_64-w64-mingw32
WCXX       := $(MINGW)-g++
FLTK_VER   := 1.3.9
FLTK_URL   := https://www.fltk.org/pub/fltk/$(FLTK_VER)/fltk-$(FLTK_VER)-source.tar.gz
WDEPS      := build-win
WPREFIX    := $(abspath $(WDEPS)/prefix)
WFLTK_LIB  := $(WPREFIX)/lib/libfltk.a

win: $(APP).exe

$(APP).exe: $(SRC) $(WFLTK_LIB)
	$(WCXX) $(CXXFLAGS) -DWIN32 -I$(WPREFIX)/include $< -o $@ \
	    -L$(WPREFIX)/lib -lfltk \
	    -lole32 -luuid -lcomctl32 -lgdi32 -lcomdlg32 -lshell32 -lws2_32 \
	    -static -mwindows -Wl,--gc-sections -s

# Downloads FLTK and builds it once for MinGW (static libs only).
$(WFLTK_LIB):
	mkdir -p $(WDEPS)
	[ -f $(WDEPS)/fltk.tar.gz ] || curl -L -o $(WDEPS)/fltk.tar.gz $(FLTK_URL)
	[ -d $(WDEPS)/fltk-$(FLTK_VER) ] || tar -xzf $(WDEPS)/fltk.tar.gz -C $(WDEPS)
	cmake -S $(WDEPS)/fltk-$(FLTK_VER) -B $(WDEPS)/cmake \
	    -DCMAKE_SYSTEM_NAME=Windows \
	    -DCMAKE_C_COMPILER=$(MINGW)-gcc \
	    -DCMAKE_CXX_COMPILER=$(MINGW)-g++ \
	    -DCMAKE_RC_COMPILER=$(MINGW)-windres \
	    -DCMAKE_BUILD_TYPE=MinSizeRel \
	    -DCMAKE_INSTALL_PREFIX=$(WPREFIX) \
	    -DOPTION_BUILD_SHARED_LIBS=OFF \
	    -DOPTION_BUILD_EXAMPLES=OFF \
	    -DFLTK_BUILD_TEST=OFF \
	    -DFLTK_BUILD_EXAMPLES=OFF \
	    -DOPTION_USE_GL=OFF
	cmake --build $(WDEPS)/cmake -j$$(nproc)
	cmake --install $(WDEPS)/cmake

# ----------------------------------------------------------------- clean ---
clean:
	rm -f $(APP) $(APP).exe

distclean: clean
	rm -rf $(WDEPS)
