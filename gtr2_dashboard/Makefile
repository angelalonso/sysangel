CXX      := g++
WIN_CXX  := x86_64-w64-mingw32-g++
CXXFLAGS := -std=c++17 -Os -Wall -Wextra -ffunction-sections -fdata-sections
PREFIX   := /home/aaf/Software/Dev/sysangel/gtr2_dashboard/build-win/prefix
WIN_CXXFLAGS := $(CXXFLAGS) -DWIN32 -isystem $(PREFIX)/include

SRC      := launcher.cpp
OBJ      := launcher.o
WIN_OBJ  := launcher.win.o
TEST_BIN := launcher_tests
CLEANUP_SRC := cleanup.cpp
CLEANUP_BIN := cleanup
CLEANUP_WIN_BIN := cleanup.exe

# Windows system libraries needed by static FLTK (GDI, COM, common controls, winsock).
# Order matters: they must come after -lfltk.
WIN_SYS_LIBS := -lgdi32 -lole32 -luuid -lcomctl32 -lwsock32

all: gamelauncher

$(CLEANUP_BIN): $(CLEANUP_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

win-cleanup: $(CLEANUP_WIN_BIN)

$(CLEANUP_WIN_BIN): $(CLEANUP_SRC)
	$(WIN_CXX) $(CXXFLAGS) -static-libgcc -static-libstdc++ $< -o $@

gamelauncher: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lfltk

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

win: gamelauncher.exe

gamelauncher.exe: $(WIN_OBJ)
	$(WIN_CXX) $(CXXFLAGS) -mwindows -static-libgcc -static-libstdc++ $^ -o $@ -L$(PREFIX)/lib -lfltk $(WIN_SYS_LIBS)

$(WIN_OBJ): $(SRC)
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@

$(TEST_BIN): tests.cpp launcher.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -lfltk

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(WIN_OBJ) gamelauncher gamelauncher.exe $(TEST_BIN) $(CLEANUP_BIN) $(CLEANUP_WIN_BIN)

.PHONY: all win cleanup win-cleanup test clean
