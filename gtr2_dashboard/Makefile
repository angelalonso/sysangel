CXX      := g++
WIN_CXX  := x86_64-w64-mingw32-g++
CXXFLAGS := -std=c++17 -Os -Wall -Wextra -ffunction-sections -fdata-sections
PREFIX   := /home/aaf/Software/Dev/sysangel/gtr2_dashboard/build-win/prefix

SRC      := launcher.cpp
OBJ      := launcher.o
WIN_OBJ  := launcher.win.o
TEST_BIN := launcher_tests

# Windows system libraries needed by static FLTK (GDI, COM, common controls, winsock).
# Order matters: they must come after -lfltk.
WIN_SYS_LIBS := -lgdi32 -lole32 -luuid -lcomctl32 -lwsock32

all: gamelauncher

gamelauncher: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lfltk

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

win: gamelauncher.exe

gamelauncher.exe: $(WIN_OBJ)
	$(WIN_CXX) $(CXXFLAGS) -mwindows -static-libgcc -static-libstdc++ $^ -o $@ -L$(PREFIX)/lib -lfltk $(WIN_SYS_LIBS)

$(WIN_OBJ): $(SRC)
	$(WIN_CXX) $(CXXFLAGS) -DWIN32 -I$(PREFIX)/include -c $< -o $@

$(TEST_BIN): tests.cpp launcher.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -lfltk

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(WIN_OBJ) gamelauncher gamelauncher.exe $(TEST_BIN)

.PHONY: all win test clean
