// launcher.cpp - tiny preconfiguration frontend for a game.
//
// Layout (next to the executable):
//   launcher.cfg                  <- saved state (game path, last selections)
//   resolutions/<name>/...files   <- one folder per resolution option
//   controllers/<name>/...files   <- one folder per controller option
//
// "Apply" copies the contents of the selected resolution folder and the
// selected controller folder (recursively, overwriting) into the game folder.
// Subfolders inside an option folder are preserved, so you can ship files
// such as  config/input.ini  and they land in <game>/config/input.ini.
//
// Build: see Makefile. Requires C++17 and FLTK 1.3.

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// ---------------------------------------------------------------- helpers --

// FLTK talks UTF-8; make sure paths are interpreted that way on Windows too.
static fs::path P(const std::string& s) { return fs::u8path(s); }
static std::string S(const fs::path& p) { return p.u8string(); }

static fs::path exeDir(char** argv) {
#ifdef _WIN32
    wchar_t buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, buf, sizeof(buf) / sizeof(buf[0]));
    if (n > 0) return fs::path(buf).parent_path();
#else
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) { buf[n] = 0; return fs::path(buf).parent_path(); }
#endif
    std::error_code ec;
    return fs::absolute(argv[0], ec).parent_path();
}

static std::vector<std::string> scan(const fs::path& dir) {
    std::vector<std::string> v;
    std::error_code ec;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
        if (it->is_directory(ec)) v.push_back(S(it->path().filename()));
    std::sort(v.begin(), v.end());
    return v;
}

static bool validName(const std::string& n) {
    if (n.empty() || n == "." || n == "..") return false;
    if (n.front() == ' ' || n.back() == ' ' || n.back() == '.') return false;
    return n.find_first_of("\\/:*?\"<>|") == std::string::npos;
}

// ----------------------------------------------------------------- state ---

struct Category {
    const char* title;    // shown above the list
    const char* subdir;   // folder name on disk, also config key
    const char* singular; // used in dialogs
    fs::path dir;
    Fl_Hold_Browser* list = nullptr;
};

static Category g_cats[2] = {
    {"Resolution", "resolutions", "resolution"},
    {"Controller", "controllers", "controller"},
};

static fs::path g_base;
static fs::path g_cfgFile;
static Fl_Input* g_gameInput = nullptr;
static Fl_Box* g_status = nullptr;

static void setStatus(const std::string& m) {
    std::string e;  // '&' is a shortcut marker in FLTK labels; escape it
    for (char ch : m) { e += ch; if (ch == '&') e += '&'; }
    g_status->copy_label(e.c_str());
    g_status->redraw();
}

static std::string selectedName(const Category& c) {
    int v = c.list->value();
    return v ? c.list->text(v) : "";
}

static void refresh(Category& c, const std::string& select = "") {
    c.list->clear();
    auto names = scan(c.dir);
    int sel = 0;
    for (size_t i = 0; i < names.size(); ++i) {
        c.list->add(names[i].c_str());
        if (names[i] == select) sel = (int)i + 1;
    }
    if (sel) c.list->value(sel);
}

// ---------------------------------------------------------------- config ---

static std::map<std::string, std::string> loadConfig() {
    std::map<std::string, std::string> m;
    std::ifstream f(g_cfgFile);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        m[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return m;
}

static void saveConfig() {
    std::ofstream f(g_cfgFile, std::ios::trunc);
    if (!f) return;
    f << "game_path=" << g_gameInput->value() << "\n";
    for (auto& c : g_cats) f << c.subdir << "=" << selectedName(c) << "\n";
}

// ------------------------------------------------------------- callbacks ---

static void onBrowse(Fl_Widget*, void*) {
    Fl_Native_File_Chooser fc;
    fc.title("Select the game's installation folder");
    fc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    if (g_gameInput->value()[0]) fc.directory(g_gameInput->value());
    if (fc.show() == 0) {
        g_gameInput->value(fc.filename());
        saveConfig();
    }
}

static void onAdd(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    const char* in = fl_input("New %s name:", "", c->singular);
    if (!in) return;
    std::string name = in;
    if (!validName(name)) { setStatus("Invalid name."); return; }

    fs::path dir = c->dir / P(name);
    std::error_code ec;
    if (fs::exists(dir, ec)) { setStatus("'" + name + "' already exists."); return; }
    fs::create_directories(dir, ec);
    if (ec) { setStatus("Could not create folder: " + ec.message()); return; }

    // Optionally import files right away.
    Fl_Native_File_Chooser fc;
    fc.title("Select the file(s) for this option (Cancel to skip)");
    fc.type(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);
    int copied = 0;
    if (fc.show() == 0) {
        for (int i = 0; i < fc.count(); ++i) {
            fs::path src = P(fc.filename(i));
            fs::copy_file(src, dir / src.filename(),
                          fs::copy_options::overwrite_existing, ec);
            if (!ec) ++copied;
        }
    }
    refresh(*c, name);
    setStatus(copied ? "Added '" + name + "' with " + std::to_string(copied) + " file(s)."
                     : "Added '" + name + "' (empty). Use \"Open folder\" to put files in it.");
    saveConfig();
}

static void onRemove(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    std::string name = selectedName(*c);
    if (name.empty()) { setStatus("Select a " + std::string(c->singular) + " to remove."); return; }
    if (fl_choice("Delete '%s' and all its files?", "Cancel", "Delete", nullptr, name.c_str()) != 1)
        return;
    std::error_code ec;
    fs::remove_all(c->dir / P(name), ec);
    refresh(*c);
    setStatus(ec ? "Error: " + ec.message() : "Removed '" + name + "'.");
    saveConfig();
}

static void onOpen(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    std::string name = selectedName(*c);
    fs::path dir = name.empty() ? c->dir : c->dir / P(name);
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    std::string q = "'";
    for (char ch : S(dir)) { if (ch == '\'') q += "'\\''"; else q += ch; }
    q += "'";
    std::string cmd = "xdg-open " + q + " >/dev/null 2>&1 &";
    int r = system(cmd.c_str());
    (void)r;
#endif
}

static void onApply(Fl_Widget*, void*) {
    std::error_code ec;
    fs::path game = P(g_gameInput->value());
    if (game.empty() || !fs::is_directory(game, ec)) {
        setStatus("Game folder does not exist. Set it first.");
        return;
    }
    std::string summary;
    for (auto& c : g_cats) {
        std::string name = selectedName(c);
        if (name.empty()) continue;
        fs::copy(c.dir / P(name), game,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        if (ec) { setStatus("Error applying '" + name + "': " + ec.message()); return; }
        if (!summary.empty()) summary += " + ";
        summary += name;
    }
    if (summary.empty()) { setStatus("Select a resolution and/or a controller first."); return; }
    saveConfig();
    setStatus("Applied: " + summary);
}

// ------------------------------------------------------------------- GUI ---

static void buildCategory(Category& c, int x, int y, int w, int h) {
    Fl_Box* t = new Fl_Box(x, y, w, 20, c.title);
    t->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    t->labelfont(FL_BOLD);

    c.list = new Fl_Hold_Browser(x, y + 22, w, h);
    c.list->format_char(0);  // names are plain text, '@' has no special meaning

    int bw = (w - 6) / 3, by = y + 22 + h + 5;
    (new Fl_Button(x, by, bw, 25, "Add"))->callback(onAdd, &c);
    (new Fl_Button(x + bw + 3, by, bw, 25, "Remove"))->callback(onRemove, &c);
    (new Fl_Button(x + 2 * (bw + 3), by, bw, 25, "Open folder"))->callback(onOpen, &c);
}

int main(int argc, char** argv) {
    g_base = exeDir(argv);
    g_cfgFile = g_base / "launcher.cfg";
    for (auto& c : g_cats) {
        c.dir = g_base / c.subdir;
        std::error_code ec;
        fs::create_directories(c.dir, ec);
    }
    auto cfg = loadConfig();

    Fl::scheme("gtk+");
    Fl_Window* win = new Fl_Window(540, 395, "Game launcher");

    Fl_Box* lbl = new Fl_Box(10, 10, 80, 25, "Game folder:");
    lbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    g_gameInput = new Fl_Input(95, 10, 325, 25);
    g_gameInput->value(cfg["game_path"].c_str());
    (new Fl_Button(430, 10, 100, 25, "Browse..."))->callback(onBrowse);

    buildCategory(g_cats[0], 10, 48, 255, 190);
    buildCategory(g_cats[1], 275, 48, 255, 190);

    Fl_Button* apply = new Fl_Button(10, 305, 520, 35, "Apply");
    apply->labelfont(FL_BOLD);
    apply->callback(onApply);

    g_status = new Fl_Box(10, 348, 520, 37);
    g_status->box(FL_DOWN_BOX);
    g_status->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    win->end();

    for (auto& c : g_cats) refresh(c, cfg[c.subdir]);

    win->show(argc, argv);
    int rc = Fl::run();
    saveConfig();
    return rc;
}
