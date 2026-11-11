// launcher.cpp - tiny preconfiguration frontend for a game.
//
// Concepts
//   * Tracked Config: a file inside the game folder, identified by its path
//                    relative to that folder (e.g. "config/input.ini").
//   * Version      : a named snapshot or patch of a tracked config, or pre-launch command lists.
//   * Mode         : a resolution or a controller option. A mode is a list of
//                    (tracked config -> version) entries.
//   * Apply        : for the selected resolution mode and controller mode,
//                    copy or patch the chosen version of each file into the game folder.
//   * Originals    : the first time we ever touch a game file, the pristine
//                    copy is saved in originals/ and never overwritten.
//
// Build: see Makefile. Requires C++17 and FLTK 1.3.

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

// ================================================================ helpers ===

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

static void openPath(const fs::path& p) {
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", p.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    std::string q = "'";
    for (char ch : S(p)) { if (ch == '\'') q += "'\\''"; else q += ch; }
    q += "'";
    std::string cmd = "xdg-open " + q + " >/dev/null 2>&1 &";
    int r = system(cmd.c_str());
    (void)r;
#endif
}

static bool validName(const std::string& n) {
    if (n.empty() || n == "." || n == "..") return false;
    if (n.front() == ' ' || n.back() == ' ' || n.back() == '.') return false;
    return n.find_first_of("\\/:*?\"<>|=") == std::string::npos;
}

static bool cleanRel(std::string& rel) {
    fs::path p = P(rel);
    if (p.empty() || p.is_absolute() || p.has_root_name() || p.has_root_directory()) return false;
    p = p.lexically_normal();
    if (p.empty() || p.filename().empty() || p.filename() == fs::path(".")) return false;
    for (const auto& part : p)
        if (part == fs::path("..")) return false;
    rel = p.generic_u8string();
    return true;
}

static void selectByText(Fl_Hold_Browser* b, const std::string& t) {
    for (int i = 1; i <= b->size(); ++i)
        if (t == b->text(i)) { b->value(i); return; }
}

static void runModal(Fl_Window* w) {
    w->set_modal();
    w->show();
    while (w->shown()) Fl::wait();
}

static Fl_Box* mkLabel(int x, int y, int w, const char* text, bool bold = false) {
    Fl_Box* b = new Fl_Box(x, y, w, 20, text);
    b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    if (bold) b->labelfont(FL_BOLD);
    return b;
}

static Fl_Hold_Browser* mkBrowser(int x, int y, int w, int h) {
    Fl_Hold_Browser* b = new Fl_Hold_Browser(x, y, w, h);
    b->format_char(0);
    return b;
}

static Fl_Multi_Browser* mkMultiBrowser(int x, int y, int w, int h) {
    Fl_Multi_Browser* b = new Fl_Multi_Browser(x, y, w, h);
    b->format_char(0);
    return b;
}

// =================================================== file / patch utils ===

static std::vector<std::string> readLines(const fs::path& p) {
    std::vector<std::string> lines;
    std::ifstream in(p);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

static bool writeLines(const fs::path& p, const std::vector<std::string>& lines) {
    std::ofstream out(p, std::ios::trunc);
    if (!out) return false;
    for (auto& l : lines) out << l << "\n";
    return true;
}

static std::string extractKey(const std::string& line) {
    std::string trimmed = line;
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) 
        trimmed.erase(trimmed.begin());
    if (trimmed.rfind("//", 0) == 0 || trimmed.rfind("#", 0) == 0 || trimmed.rfind(";", 0) == 0)
        return "";

    auto pos = line.find_first_of(":=");
    if (pos == std::string::npos) return "";
    std::string key = line.substr(0, pos);
    while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
    while (!key.empty() && (key.front() == ' ' || key.front() == '\t')) key.erase(key.begin());
    return key;
}

static std::vector<std::string> applyPatch(const std::vector<std::string>& origLines, const std::vector<std::string>& patchLines) {
    std::vector<std::string> result = origLines;
    std::map<std::string, size_t> keyIndices;
    for (size_t i = 0; i < result.size(); ++i) {
        std::string k = extractKey(result[i]);
        if (!k.empty()) keyIndices[k] = i;
    }

    std::string lastAnchorKey = "";

    for (size_t p = 0; p < patchLines.size(); ++p) {
        const std::string& pl = patchLines[p];
        std::string pk = extractKey(pl);

        if (!pk.empty()) {
            lastAnchorKey = pk;
            auto it = keyIndices.find(pk);
            if (it != keyIndices.end()) {
                result[it->second] = pl;
            } else {
                result.push_back(pl);
                keyIndices[pk] = result.size() - 1;
            }
        } else {
            if (!lastAnchorKey.empty() && keyIndices.find(lastAnchorKey) != keyIndices.end()) {
                size_t anchorIdx = keyIndices[lastAnchorKey];
                size_t targetIdx = anchorIdx + 1;
                if (targetIdx < result.size() && extractKey(result[targetIdx]).empty()) {
                    result[targetIdx] = pl;
                } else {
                    result.insert(result.begin() + anchorIdx + 1, pl);
                    keyIndices.clear();
                    for (size_t i = 0; i < result.size(); ++i) {
                        std::string k = extractKey(result[i]);
                        if (!k.empty()) keyIndices[k] = i;
                    }
                }
            } else {
                result.push_back(pl);
            }
        }
    }
    return result;
}

static std::vector<std::string> computeDiffLines(const std::vector<std::string>& origLines, const std::vector<std::string>& extLines) {
    std::map<std::string, std::string> origMap;
    for (const auto& l : origLines) {
        std::string k = extractKey(l);
        if (!k.empty()) origMap[k] = l;
    }

    std::vector<std::string> diffs;
    for (const auto& l : extLines) {
        std::string k = extractKey(l);
        if (!k.empty()) {
            auto it = origMap.find(k);
            if (it == origMap.end() || it->second != l) {
                diffs.push_back(l);
            }
        } else {
            if (std::find(origLines.begin(), origLines.end(), l) == origLines.end()) {
                diffs.push_back(l);
            }
        }
    }
    return diffs;
}

// =================================================================== state ===

struct Category {
    const char* title;
    const char* subdir;
    const char* singular;
    fs::path dir;
    Fl_Hold_Browser* list = nullptr;
};

static Category g_cats[2] = {
    {"Resolution", "resolutions", "resolution", {}},
    {"Controller", "controllers", "controller", {}},
};

static fs::path g_base, g_cfgFile, g_modesDir, g_versionsDir, g_originalsDir;
static std::string g_gameFolder;
static std::string g_executablePath;
static Fl_Window* g_mainWindow = nullptr;
static Fl_Button* g_applyButton = nullptr;
static Fl_Box* g_gameLabel = nullptr;
static Fl_Box* g_status = nullptr;

static std::string selectedName(const Category& c);

static Fl_Color lightBlueColor() { return fl_rgb_color(226, 232, 240); }
static Fl_Color inactiveWindowColor() { return fl_rgb_color(194, 200, 208); }
static Fl_Color applyReadyColor() { return fl_rgb_color(238, 248, 210); }

static void setMainWindowActive(bool active) {
    if (!g_mainWindow) return;
    g_mainWindow->color(active ? lightBlueColor() : inactiveWindowColor());
    g_mainWindow->redraw();
}

static void updateApplyButton() {
    if (!g_applyButton) return;
    bool hasSelection = false;
    for (auto& c : g_cats)
        if (!selectedName(c).empty()) {
            hasSelection = true;
            break;
        }
    g_applyButton->color(hasSelection ? applyReadyColor() : FL_BACKGROUND_COLOR);
    g_applyButton->redraw();
}

static void debugLog(const std::string& message) {
    std::ofstream out(g_base / "launcher.log", std::ios::app);
    if (out) {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        out << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << " " << message << "\n";
    }
    std::clog << message << "\n";
}

static fs::path gamePath() { return P(g_gameFolder); }

static bool gameOk() {
    std::error_code ec;
    return !g_gameFolder.empty() && fs::is_directory(gamePath(), ec);
}

static void setStatus(const std::string& m) {
    g_status->copy_label(m.c_str());
    g_status->redraw();
}

static void updateGameLabel() {
    std::string t = "Game folder: " +
                    (g_gameFolder.empty() ? std::string("(not set - open Settings)") : g_gameFolder);
    g_gameLabel->copy_label(t.c_str());
    g_gameLabel->redraw();
}

static std::string selectedName(const Category& c) {
    int v = c.list->value();
    return v ? c.list->text(v) : "";
}

// ================================================================ config ====

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
    f << "game_path=" << g_gameFolder << "\n";
    f << "executable_path=" << g_executablePath << "\n";
    for (auto& c : g_cats) f << c.subdir << "=" << selectedName(c) << "\n";
}

// ========================================================== modes (disk) ====

struct Entry { std::string file, version; };

static fs::path modeFile(const Category& c, const std::string& name) {
    return c.dir / P(name + ".mode");
}

static std::vector<Entry> loadMode(const fs::path& f) {
    std::vector<Entry> v;
    std::ifstream in(f);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto eq = line.rfind('=');
        if (eq == std::string::npos || eq == 0) continue;
        v.push_back({line.substr(0, eq), line.substr(eq + 1)});
    }
    return v;
}

static bool saveMode(const fs::path& f, const std::vector<Entry>& v) {
    std::ofstream out(f, std::ios::trunc);
    if (!out) return false;
    for (auto& e : v) out << e.file << "=" << e.version << "\n";
    return (bool)out;
}

static std::vector<std::string> scanModes(const fs::path& dir) {
    std::vector<std::string> v;
    std::error_code ec;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        std::string n = S(it->path().filename());
        if (n.size() > 5 && n.compare(n.size() - 5, 5, ".mode") == 0 && it->is_regular_file(e2))
            v.push_back(n.substr(0, n.size() - 5));
    }
    std::sort(v.begin(), v.end());
    return v;
}

static void refresh(Category& c, const std::string& select = "") {
    c.list->clear();
    auto names = scanModes(c.dir);
    int sel = 0;
    for (size_t i = 0; i < names.size(); ++i) {
        c.list->add(names[i].c_str());
        if (names[i] == select) sel = (int)i + 1;
    }
    if (sel) c.list->value(sel);
    updateApplyButton();
}

// ======================================================== versions (disk) ===

using VersionMap = std::map<std::string, std::vector<std::string>>;

static fs::path versionPath(const std::string& rel, const std::string& ver, bool isPatch) {
    return g_versionsDir / P(rel) / P(ver + (isPatch ? ".patch" : ""));
}

static std::pair<std::string, bool> parseVersionDisplay(const std::string& itemText) {
    if (itemText.size() > 8 && itemText.compare(itemText.size() - 8, 8, " [patch]") == 0) {
        return {itemText.substr(0, itemText.size() - 8), true};
    }
    if (itemText.size() > 14 && itemText.compare(itemText.size() - 14, 14, " [pre-launch]") == 0) {
        return {itemText.substr(0, itemText.size() - 14), false};
    }
    return {itemText, false};
}

static VersionMap scanVersions() {
    VersionMap m;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(g_versionsDir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        if (!it->is_regular_file(e2)) continue;
        fs::path rel = it->path().parent_path().lexically_relative(g_versionsDir);
        if (rel.empty() || rel == fs::path(".")) continue;
        std::string fname = S(it->path().filename());
        bool isPatch = false;
        bool isCmdList = (rel.generic_u8string() == "__pre_launch_commands__");
        
        if (!isCmdList && fname.size() > 6 && fname.compare(fname.size() - 6, 6, ".patch") == 0) {
            fname = fname.substr(0, fname.size() - 6);
            isPatch = true;
        }
        std::string dispKey = isCmdList ? "Pre-launch Commands" : rel.generic_u8string();
        std::string suffix = isCmdList ? " [pre-launch]" : (isPatch ? " [patch]" : " [full]");
        m[dispKey].push_back(fname + suffix);
    }
    for (auto& kv : m) std::sort(kv.second.begin(), kv.second.end());
    return m;
}

static std::vector<std::string> modesUsing(const std::string& rel, const std::string& ver) {
    std::vector<std::string> out;
    for (auto& c : g_cats)
        for (auto& n : scanModes(c.dir))
            for (auto& e : loadMode(modeFile(c, n)))
                if (e.file == rel && (e.version == ver || e.version + " [patch]" == ver || e.version + " [full]" == ver || e.version + " [pre-launch]" == ver)) {
                    out.push_back(std::string(c.singular) + " " + n);
                    break;
                }
    return out;
}

static bool relFromGame(const fs::path& file, std::string& rel) {
    std::error_code ec;
    fs::path game = fs::weakly_canonical(gamePath(), ec);
    fs::path f = fs::weakly_canonical(file, ec);
    fs::path r = f.lexically_relative(game);
    if (r.empty()) return false;
    rel = r.generic_u8string();
    return cleanRel(rel);
}

// ====================================================== originals backup ====

static bool backupOriginal(const std::string& rel, std::string& err) {
    std::error_code ec;
    fs::path dst = g_originalsDir / P(rel);
    if (fs::exists(dst, ec)) return true;
    fs::path src = gamePath() / P(rel);
    if (!fs::is_regular_file(src, ec)) return true;
    fs::create_directories(dst.parent_path(), ec);
    if (ec) { err = ec.message(); return false; }
    fs::copy_file(src, dst, fs::copy_options::none, ec);
    if (ec) { err = ec.message(); return false; }
    return true;
}

static void onRestore(Fl_Widget*, void*) {
    if (!gameOk()) { setStatus("Game folder is not set or does not exist."); return; }
    std::vector<fs::path> rels;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(g_originalsDir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        if (it->is_regular_file(e2)) rels.push_back(it->path().lexically_relative(g_originalsDir));
    }
    if (rels.empty()) { setStatus("No originals have been saved yet."); return; }
    if (fl_choice("Restore %d original file(s) into the game folder?\n"
                  "This overwrites the files currently in the game.",
                  "Cancel", "Restore", nullptr, (int)rels.size()) != 1)
        return;
    int ok = 0;
    for (auto& r : rels) {
        fs::path dst = gamePath() / r;
        fs::create_directories(dst.parent_path(), ec);
        fs::copy_file(g_originalsDir / r, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) { setStatus("Error restoring " + r.generic_u8string() + ": " + ec.message()); return; }
        ++ok;
    }
    setStatus("Restored " + std::to_string(ok) + " original file(s).");
}

// ====================================================== dialog: add version ==

struct AddFormDlg {
    Fl_Window* win_ = nullptr;
    Fl_Choice* typeChoice = nullptr;
    Fl_Box *fileLabel = nullptr, *nameLabel = nullptr;
    Fl_Input *fileIn = nullptr, *nameIn = nullptr;
    Fl_Button *browseBtn = nullptr;
    Fl_Check_Button* patchCb_ = nullptr;
    Fl_Check_Button* extPatchCb_ = nullptr;
    Fl_Input* extFileIn = nullptr;
    Fl_Button* browseExtBtn = nullptr;
    Fl_Button* selectPatch_ = nullptr;
    Fl_Multi_Browser* cmdsBrowser = nullptr;
    Fl_Button *addCmdBtn = nullptr, *removeCmdBtn = nullptr;
    bool ok_ = false;
    std::string rel_, ver_;
    bool isPatch_ = false;
    bool isCmdList_ = false;
    std::string extFilePath_;
    std::vector<std::string> patchLines_;
    std::vector<std::string> cmdList_;
};

static void afToggleConfigType(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    bool isCmd = (f->typeChoice->value() == 1);
    if (isCmd) {
        f->fileIn->hide();
        if (f->browseBtn) f->browseBtn->hide();
        if (f->patchCb_) f->patchCb_->hide();
        if (f->extPatchCb_) f->extPatchCb_->hide();
        if (f->extFileIn) f->extFileIn->hide();
        if (f->browseExtBtn) f->browseExtBtn->hide();
        if (f->selectPatch_) f->selectPatch_->hide();

        if (f->cmdsBrowser) f->cmdsBrowser->show();
        if (f->addCmdBtn) f->addCmdBtn->show();
        if (f->removeCmdBtn) f->removeCmdBtn->show();

        f->fileLabel->copy_label("Pre-launch commands (executed sequentially before game launch):");
        f->nameLabel->copy_label("Command List Version Name (e.g., default, high_perf):");
    } else {
        if (f->cmdsBrowser) f->cmdsBrowser->hide();
        if (f->addCmdBtn) f->addCmdBtn->hide();
        if (f->removeCmdBtn) f->removeCmdBtn->hide();

        f->fileIn->show();
        if (f->browseBtn) f->browseBtn->show();
        if (f->patchCb_) f->patchCb_->show();
        if (f->extPatchCb_) f->extPatchCb_->show();
        if (f->extPatchCb_->value()) {
            if (f->extFileIn) f->extFileIn->show();
            if (f->browseExtBtn) f->browseExtBtn->show();
        }
        if (f->selectPatch_) f->selectPatch_->show();

        f->fileLabel->copy_label("File inside the game folder (e.g. config/input.ini):");
        f->nameLabel->copy_label("Version name (e.g. 1080p, xbox):");
    }
}

static void afBrowseFile(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    if (!gameOk()) { fl_alert("%s", "Set the game folder first (Settings)."); return; }
    Fl_Native_File_Chooser fc;
    fc.title("Select a file inside the game folder");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fc.directory(g_gameFolder.c_str());
    if (fc.show() != 0) return;
    std::string rel;
    if (!relFromGame(P(fc.filename()), rel))
        fl_alert("%s", "That file is not inside the game folder.");
    else {
        f->fileIn->value(rel.c_str());
        f->patchLines_.clear();
    }
}

struct PatchLinesDlg {
    Fl_Window* win = nullptr;
    Fl_Multi_Browser* lines = nullptr;
    Fl_Input* search = nullptr;
    bool ok = false;
    int searchStart = 0;
    std::vector<std::string> fileLines;
    std::vector<std::string> selected;
};

static bool containsIgnoreCase(const std::string& text, const std::string& query) {
    if (query.empty()) return false;
    auto it = std::search(text.begin(), text.end(), query.begin(), query.end(),
                          [](char a, char b) {
                              return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
                          });
    return it != text.end();
}

static void patchLinesFind(Fl_Widget*, void* p) {
    PatchLinesDlg* d = (PatchLinesDlg*)p;
    std::string query = d->search->value();
    if (query.empty() || d->fileLines.empty()) return;

    for (size_t offset = 0; offset < d->fileLines.size(); ++offset) {
        size_t index = (static_cast<size_t>(d->searchStart) + offset) % d->fileLines.size();
        if (containsIgnoreCase(d->fileLines[index], query)) {
            d->searchStart = static_cast<int>((index + 1) % d->fileLines.size());
            d->lines->middleline(static_cast<int>(index + 1));
            d->lines->redraw();
            return;
        }
    }
    fl_alert("No lines found for:\n%s", query.c_str());
}

static void patchLinesOk(Fl_Widget*, void* p) {
    PatchLinesDlg* d = (PatchLinesDlg*)p;
    for (int i = 1; i <= d->lines->size(); ++i)
        if (d->lines->selected(i))
            d->selected.push_back(d->fileLines[i - 1]);
    if (d->selected.empty()) {
        fl_alert("%s", "Select at least one line to include in the patch.");
        return;
    }
    d->ok = true;
    d->win->hide();
}

static void patchLinesCancel(Fl_Widget*, void* p) {
    ((PatchLinesDlg*)p)->win->hide();
}

static bool selectPatchLines(const fs::path& file, const std::vector<std::string>& initialSelected, std::vector<std::string>& out) {
    std::error_code ec;
    if (!fs::is_regular_file(file, ec)) {
        fl_alert("File not found:\n%s", S(file).c_str());
        return false;
    }

    PatchLinesDlg d;
    d.win = new Fl_Window(760, 530, "Select patch lines");
    mkLabel(10, 8, 740, "Select the lines to include in this patch:", true);
    d.search = new Fl_Input(10, 32, 580, 25, "Search:");
    d.search->when(FL_WHEN_ENTER_KEY);
    d.search->callback(patchLinesFind, &d);
    (new Fl_Button(600, 32, 150, 25, "Find next"))->callback(patchLinesFind, &d);
    d.lines = new Fl_Multi_Browser(10, 65, 740, 377);
    d.lines->format_char(0);
    d.fileLines = readLines(file);
    
    std::map<std::string, bool> initialMap;
    for (const auto& s : initialSelected) initialMap[s] = true;

    for (size_t i = 0; i < d.fileLines.size(); ++i) {
        std::string display = std::to_string(i + 1) + ": " + d.fileLines[i];
        d.lines->add(display.c_str());
        if (initialMap[d.fileLines[i]]) {
            d.lines->select(static_cast<int>(i + 1));
        }
    }
    (new Fl_Button(560, 490, 90, 30, "OK"))->callback(patchLinesOk, &d);
    (new Fl_Button(660, 490, 90, 30, "Cancel"))->callback(patchLinesCancel, &d);
    d.win->end();
    runModal(d.win);
    delete d.win;
    if (!d.ok) return false;
    out = std::move(d.selected);
    return true;
}

static void afSelectPatch(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    std::string rel = f->fileIn->value();
    if (!cleanRel(rel)) {
        fl_alert("%s", "Select a valid file inside the game folder first.");
        return;
    }
    std::vector<std::string> selected;
    if (selectPatchLines(gamePath() / P(rel), f->patchLines_, selected))
        f->patchLines_ = std::move(selected);
}

static void afFileChanged(Fl_Widget*, void* p) {
    ((AddFormDlg*)p)->patchLines_.clear();
}

static void afPatchChanged(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    f->selectPatch_->activate();
    if (!f->patchCb_->value() && !f->extPatchCb_->value()) {
        f->patchLines_.clear();
        f->selectPatch_->deactivate();
    }
}

static void afExtPatchCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    if (f->extPatchCb_->value()) {
        f->patchCb_->set();
        if (f->extFileIn) f->extFileIn->show();
        if (f->browseExtBtn) f->browseExtBtn->show();
        f->selectPatch_->activate();
    } else {
        if (f->extFileIn) f->extFileIn->hide();
        if (f->browseExtBtn) f->browseExtBtn->hide();
        f->extFilePath_.clear();
        if (f->extFileIn) f->extFileIn->value("");
    }
}

static void afBrowseExternal(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    Fl_Native_File_Chooser fc;
    fc.title("Select external file to create patch from");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (fc.show() != 0) return;
    f->extFilePath_ = fc.filename();
    if (f->extFileIn) f->extFileIn->value(f->extFilePath_.c_str());

    std::string rel = f->fileIn->value();
    if (cleanRel(rel)) {
        fs::path targetFile = gamePath() / P(rel);
        std::error_code ec;
        if (fs::is_regular_file(targetFile, ec)) {
            auto origLines = readLines(targetFile);
            auto extLines = readLines(P(f->extFilePath_));
            f->patchLines_ = computeDiffLines(origLines, extLines);
        }
    }
}

static void afAddCmdCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    const char* cmdIn = fl_input("Enter command to run before launch:", "");
    if (!cmdIn) return;
    std::string cmd = cmdIn;
    if (!cmd.empty()) {
        f->cmdsBrowser->add(cmd.c_str());
    }
}

static void afRemoveCmdCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    int idx = f->cmdsBrowser->value();
    if (!idx) {
        fl_alert("%s", "Select a command from the list to remove.");
        return;
    }
    f->cmdsBrowser->remove(idx);
}

static void afOk(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    if (!gameOk()) { fl_alert("%s", "Set the game folder first (Settings)."); return; }

    bool isCmds = (f->typeChoice->value() == 1);
    bool isFile = (f->typeChoice->value() == 0);

    std::string rel, ver;
    bool isPatch = false;

    if (isCmds) {
        rel = "__pre_launch_commands__";
        ver = f->nameIn->value();
        f->cmdList_.clear();
        for (int i = 1; i <= f->cmdsBrowser->size(); ++i) {
            std::string line = f->cmdsBrowser->text(i);
            if (!line.empty()) f->cmdList_.push_back(line);
        }
        if (f->cmdList_.empty()) {
            fl_alert("%s", "Add at least one pre-launch command.");
            return;
        }
        if (!validName(ver)) {
            fl_alert("%s", "Invalid version name.\nAvoid  \\ / : * ? \" < > | =  and leading/trailing spaces.");
            return;
        }
    } else {
        rel = f->fileIn->value();
        ver = f->nameIn->value();
        isPatch = f->patchCb_->value() != 0 || f->extPatchCb_->value() != 0;

        if (!cleanRel(rel)) {
            fl_alert("%s", "Invalid file path.\nUse a path relative to the game folder, e.g. config/input.ini");
            return;
        }
        if (!validName(ver)) {
            fl_alert("%s", "Invalid version name.\nAvoid  \\ / : * ? \" < > | =  and leading/trailing spaces.");
            return;
        }
        fs::path from = gamePath() / P(rel);
        std::error_code ec;
        if (!fs::is_regular_file(from, ec)) {
            fl_alert("Game file not found:\n%s", S(from).c_str());
            return;
        }

        std::string err;
        if (!backupOriginal(rel, err)) {
            fl_alert("Could not back up the original:\n%s", err.c_str());
            return;
        }
    }

    fs::path dst = versionPath(rel, ver, isPatch);
    std::error_code ec;
    if (fs::exists(dst, ec)) {
        fl_alert("Version '%s' of that config already exists.", ver.c_str());
        return;
    }
    fs::create_directories(dst.parent_path(), ec);

    if (isCmds) {
        if (!writeLines(dst, f->cmdList_)) {
            fl_alert("%s", "Could not save pre-launch commands version file.");
            return;
        }
    } else if (isPatch) {
        if (f->patchLines_.empty()) {
            fl_alert("%s", "Use 'Select patch lines...' to choose at least one line.");
            return;
        }
        if (!writeLines(dst, f->patchLines_)) {
            fl_alert("%s", "Could not save the patch version file.");
            return;
        }
    } else {
        fs::copy_file(gamePath() / P(rel), dst, fs::copy_options::none, ec);
        if (ec) {
            fl_alert("Could not save the version:\n%s", ec.message().c_str());
            return;
        }
    }

    f->ok_ = true;
    f->rel_ = rel;
    f->ver_ = ver;
    f->isPatch_ = isPatch;
    f->isCmdList_ = isCmds;
    f->win_->hide();
}

static void afCancel(Fl_Widget*, void* p) { ((AddFormDlg*)p)->win_->hide(); }

static bool addVersionForm(const std::string& prefillRel, std::string& outRel, std::string& outVer, bool& outIsPatch) {
    AddFormDlg f;
    f.win_ = new Fl_Window(560, 480, "Add Version");

    mkLabel(10, 8, 540, "Config Type:", true);
    f.typeChoice = new Fl_Choice(10, 30, 220, 25);
    f.typeChoice->add("Config file");
    f.typeChoice->add("Pre-launch commands");
    f.typeChoice->value(0);
    f.typeChoice->callback(afToggleConfigType, &f);

    f.fileLabel = mkLabel(10, 62, 540, "File inside the game folder (e.g. config/input.ini):");
    
    // Config file controls
    f.fileIn = new Fl_Input(10, 84, 440, 25);
    f.fileIn->callback(afFileChanged, &f);
    
    f.browseBtn = new Fl_Button(460, 84, 90, 25, "Browse...");
    f.browseBtn->callback(afBrowseFile, &f);

    f.patchCb_ = new Fl_Check_Button(10, 120, 250, 25, " Store as patch");
    f.patchCb_->callback(afPatchChanged, &f);

    f.extPatchCb_ = new Fl_Check_Button(10, 150, 300, 25, " Create patch from external file");
    f.extPatchCb_->callback(afExtPatchCb, &f);

    f.extFileIn = new Fl_Input(10, 180, 440, 25);
    f.extFileIn->hide();

    f.browseExtBtn = new Fl_Button(460, 180, 90, 25, "Browse ext...");
    f.browseExtBtn->callback(afBrowseExternal, &f);
    f.browseExtBtn->hide();
    
    Fl_Button* selectPatchLinesBtn = new Fl_Button(270, 120, 180, 25, "Select patch lines...");
    selectPatchLinesBtn->callback(afSelectPatch, &f);
    selectPatchLinesBtn->deactivate();
    f.selectPatch_ = selectPatchLinesBtn;

    // Pre-launch commands controls (shared layout area, initially hidden)
    f.cmdsBrowser = mkMultiBrowser(10, 84, 540, 100);
    f.cmdsBrowser->hide();

    f.addCmdBtn = new Fl_Button(10, 188, 110, 25, "Add command");
    f.addCmdBtn->callback(afAddCmdCb, &f);
    f.addCmdBtn->hide();

    f.removeCmdBtn = new Fl_Button(130, 188, 130, 25, "Remove command");
    f.removeCmdBtn->callback(afRemoveCmdCb, &f);
    f.removeCmdBtn->hide();

    f.nameLabel = mkLabel(10, 225, 540, "Version name (e.g. 1080p, xbox):");
    f.nameIn = new Fl_Input(10, 247, 540, 25);

    (new Fl_Button(340, 435, 130, 30, "Save version"))->callback(afOk, &f);
    (new Fl_Button(480, 435, 70, 30, "Cancel"))->callback(afCancel, &f);
    f.win_->end();

    if (!prefillRel.empty() && prefillRel != "Pre-launch Commands") {
        f.fileIn->value(prefillRel.c_str());
    }

    runModal(f.win_);
    delete f.win_;
    if (f.ok_) { outRel = f.rel_; outVer = f.ver_; outIsPatch = f.isPatch_; }
    return f.ok_;
}

// ======================================================== dialog: config versions ==

struct VerMgr {
    Fl_Window* win = nullptr;
    Fl_Hold_Browser *filesB = nullptr, *versB = nullptr;
    VersionMap vm;
};

static void vmFilesCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    m->versB->clear();
    int i = m->filesB->value();
    if (!i) return;
    auto it = m->vm.find(m->filesB->text(i));
    if (it == m->vm.end()) return;
    for (auto& v : it->second) m->versB->add(v.c_str());
}

static void vmReload(VerMgr* m, const std::string& selFile = "", const std::string& selVer = "") {
    m->vm = scanVersions();
    m->filesB->clear();
    for (auto& kv : m->vm) m->filesB->add(kv.first.c_str());
    if (!selFile.empty()) selectByText(m->filesB, selFile);
    vmFilesCb(nullptr, m);
    if (!selVer.empty()) selectByText(m->versB, selVer);
}

static void vmAddCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    int i = m->filesB->value();
    std::string pre = i ? m->filesB->text(i) : "";
    std::string rel, ver;
    bool isPatch = false;
    if (addVersionForm(pre, rel, ver, isPatch)) {
        std::string dispVer = ver + (rel == "__pre_launch_commands__" ? " [pre-launch]" : (isPatch ? " [patch]" : " [full]"));
        std::string dispKey = (rel == "__pre_launch_commands__") ? "Pre-launch Commands" : rel;
        vmReload(m, dispKey, dispVer);
        setStatus("Saved version '" + ver + "' for " + (rel == "__pre_launch_commands__" ? "Pre-launch Commands" : rel) + ".");
    }
}

static void vmDeleteCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    int fi = m->filesB->value(), vi = m->versB->value();
    if (!fi || !vi) { fl_alert("%s", "Select a tracked config and one of its versions first."); return; }
    std::string dispFile = m->filesB->text(fi);
    std::string rel = (dispFile == "Pre-launch Commands") ? "__pre_launch_commands__" : dispFile;
    auto [verClean, isPatch] = parseVersionDisplay(m->versB->text(vi));

    std::string used;
    for (auto& u : modesUsing(rel, m->versB->text(vi))) used += "\n  - " + u;
    std::string warn = used.empty() ? "" : "\n\nIt is used by:" + used + "\nThose entries will be flagged as missing.";
    if (fl_choice("Delete version '%s' of %s?%s", "Cancel", "Delete", nullptr, verClean.c_str(), dispFile.c_str(),
                  warn.c_str()) != 1)
        return;

    std::error_code ec;
    fs::remove(versionPath(rel, verClean, isPatch), ec);
    fs::path dir = versionPath(rel, verClean, isPatch).parent_path();
    while (dir != g_versionsDir && fs::is_empty(dir, ec) && !ec) {
        fs::path parent = dir.parent_path();
        fs::remove(dir, ec);
        dir = parent;
    }
    vmReload(m, dispFile);
}

static void vmCloseCb(Fl_Widget*, void* p) { ((VerMgr*)p)->win->hide(); }

static void onVersions(Fl_Widget*, void*) {
    VerMgr m;
    m.win = new Fl_Window(620, 390, "Config Versions");
    m.win->color(lightBlueColor());
    mkLabel(10, 10, 300, "Tracked Configs", true);
    mkLabel(320, 10, 290, "Versions", true);
    m.filesB = mkBrowser(10, 32, 300, 250);
    m.filesB->callback(vmFilesCb, &m);
    m.versB = mkBrowser(320, 32, 290, 250);
    (new Fl_Button(10, 287, 150, 28, "Add version..."))->callback(vmAddCb, &m);
    (new Fl_Button(320, 287, 150, 28, "Delete version"))->callback(vmDeleteCb, &m);

    std::string info = "Originals of touched game files are saved once in:\n" + S(g_originalsDir);
    Fl_Box* ib = new Fl_Box(10, 325, 490, 55);
    ib->copy_label(info.c_str());
    ib->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    (new Fl_Button(510, 350, 100, 30, "Close"))->callback(vmCloseCb, &m);
    m.win->end();

    vmReload(&m);
    setMainWindowActive(false);
    runModal(m.win);
    setMainWindowActive(true);
    delete m.win;
}

// ======================================================= dialog: edit mode ==

struct ModeEditor {
    Fl_Window* win = nullptr;
    Fl_Hold_Browser *entriesB = nullptr, *filesB = nullptr, *versB = nullptr;
    fs::path file;
    std::vector<Entry> entries;
    VersionMap vm;
};

static void meRefreshEntries(ModeEditor* m) {
    m->entriesB->clear();
    for (auto& e : m->entries) {
        std::error_code ec;
        bool isCmd = (e.file == "__pre_launch_commands__");
        std::string dispFile = isCmd ? "Pre-launch Commands" : e.file;
        bool ok = isCmd ? fs::is_regular_file(versionPath(e.file, e.version, false), ec) :
                          (fs::is_regular_file(versionPath(e.file, e.version, false), ec) ||
                           fs::is_regular_file(versionPath(e.file, e.version, true), ec));
        std::string t = dispFile + "   ->   " + e.version + (ok ? "" : "   [MISSING]");
        m->entriesB->add(t.c_str());
    }
}

static void meFilesCb(Fl_Widget*, void* p) {
    ModeEditor* m = (ModeEditor*)p;
    m->versB->clear();
    int i = m->filesB->value();
    if (!i) return;
    auto it = m->vm.find(m->filesB->text(i));
    if (it == m->vm.end()) return;
    for (auto& v : it->second) m->versB->add(v.c_str());
}

static void meEntryCb(Fl_Widget*, void* p) {
    ModeEditor* m = (ModeEditor*)p;
    int i = m->entriesB->value();
    if (!i) return;
    Entry e = m->entries[i - 1];
    std::string dispFile = (e.file == "__pre_launch_commands__") ? "Pre-launch Commands" : e.file;
    m->filesB->value(0);
    selectByText(m->filesB, dispFile);
    meFilesCb(nullptr, m);
    selectByText(m->versB, e.version);
    if (!m->versB->value()) {
        selectByText(m->versB, e.version + " [full]");
        if (!m->versB->value()) selectByText(m->versB, e.version + " [patch]");
        if (!m->versB->value()) selectByText(m->versB, e.version + " [pre-launch]");
    }
}

static void meSetCb(Fl_Widget*, void* p) {
    ModeEditor* m = (ModeEditor*)p;
    int fi = m->filesB->value(), vi = m->versB->value();
    if (!fi || !vi) { fl_alert("%s", "Select a tracked config and a version first."); return; }
    std::string dispFile = m->filesB->text(fi);
    std::string realFile = (dispFile == "Pre-launch Commands") ? "__pre_launch_commands__" : dispFile;
    auto [vClean, isPatch] = parseVersionDisplay(m->versB->text(vi));
    Entry ne{realFile, vClean};
    int idx = -1;
    for (size_t i = 0; i < m->entries.size(); ++i)
        if (m->entries[i].file == ne.file) { m->entries[i] = ne; idx = (int)i; }
    if (idx < 0) { m->entries.push_back(ne); idx = (int)m->entries.size() - 1; }
    if (!saveMode(m->file, m->entries)) fl_alert("%s", "Could not save the mode file.");
    meRefreshEntries(m);
    m->entriesB->value(idx + 1);
}

static void meRemoveCb(Fl_Widget*, void* p) {
    ModeEditor* m = (ModeEditor*)p;
    int i = m->entriesB->value();
    if (!i) { fl_alert("%s", "Select an entry to remove."); return; }
    m->entries.erase(m->entries.begin() + (i - 1));
    if (!saveMode(m->file, m->entries)) fl_alert("%s", "Could not save the mode file.");
    meRefreshEntries(m);
}

static void meCloseCb(Fl_Widget*, void* p) { ((ModeEditor*)p)->win->hide(); }

static void editMode(const Category& c, const std::string& name) {
    ModeEditor m;
    m.file = modeFile(c, name);
    m.entries = loadMode(m.file);
    m.vm = scanVersions();

    m.win = new Fl_Window(620, 475, "Edit Mode");
    std::string title = std::string(c.title) + ": " + name;
    mkLabel(10, 8, 600, "", true)->copy_label(title.c_str());

    mkLabel(10, 34, 600, "Configs this mode changes (config -> version):");
    m.entriesB = mkBrowser(10, 56, 600, 130);
    m.entriesB->callback(meEntryCb, &m);
    (new Fl_Button(10, 191, 130, 26, "Remove entry"))->callback(meRemoveCb, &m);

    mkLabel(10, 228, 300, "Tracked Configs");
    mkLabel(320, 228, 290, "Versions");
    m.filesB = mkBrowser(10, 249, 300, 140);
    m.filesB->callback(meFilesCb, &m);
    m.versB = mkBrowser(320, 249, 290, 140);
    (new Fl_Button(10, 395, 200, 28, "Set config -> version"))->callback(meSetCb, &m);
    (new Fl_Button(510, 435, 100, 30, "Close"))->callback(meCloseCb, &m);
    m.win->end();

    for (auto& kv : m.vm) m.filesB->add(kv.first.c_str());
    meRefreshEntries(&m);
    runModal(m.win);
    delete m.win;
}

// ======================================================== dialog: settings ===

struct CfgDlg {
    Fl_Window* win = nullptr;
    Fl_Input *game = nullptr, *executable = nullptr;
};

static void cfgBrowse(Fl_Widget*, void* p) {
    CfgDlg* d = (CfgDlg*)p;
    Fl_Native_File_Chooser fc;
    fc.title("Select the game's installation folder");
    fc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    if (d->game->value()[0]) fc.directory(d->game->value());
    if (fc.show() == 0) d->game->value(fc.filename());
}

static void cfgBrowseExecutable(Fl_Widget*, void* p) {
    CfgDlg* d = (CfgDlg*)p;
    Fl_Native_File_Chooser fc;
    fc.title("Select the executable to launch");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    std::string current = d->executable->value();
    if (!current.empty()) fc.directory(S(P(current).parent_path()).c_str());
    if (fc.show() == 0) d->executable->value(fc.filename());
}

static void cfgOpenData(Fl_Widget*, void*) { openPath(g_base); }

static void cfgOk(Fl_Widget*, void* p) {
    CfgDlg* d = (CfgDlg*)p;
    std::string path = d->game->value();
    std::error_code ec;
    if (!path.empty() && !fs::is_directory(P(path), ec)) {
        fl_alert("%s", "That folder does not exist.");
        return;
    }
    std::string executable = d->executable->value();
    if (!executable.empty() && !fs::is_regular_file(P(executable), ec)) {
        fl_alert("%s", "The configured executable does not exist or is not a file.");
        return;
    }
    g_gameFolder = path;
    g_executablePath = executable;
    updateGameLabel();
    saveConfig();
    d->win->hide();
}

static void cfgCancel(Fl_Widget*, void* p) { ((CfgDlg*)p)->win->hide(); }

static void onSettings(Fl_Widget*, void*) {
    CfgDlg d;
    d.win = new Fl_Window(520, 170, "Settings");
    d.win->color(lightBlueColor());
    mkLabel(10, 10, 500, "Game folder:");
    d.game = new Fl_Input(10, 32, 400, 25);
    d.game->value(g_gameFolder.c_str());
    (new Fl_Button(420, 32, 90, 25, "Browse..."))->callback(cfgBrowse, &d);
    mkLabel(10, 67, 500, "Executable to launch:");
    d.executable = new Fl_Input(10, 89, 400, 25);
    d.executable->value(g_executablePath.c_str());
    (new Fl_Button(420, 89, 90, 25, "Browse..."))->callback(cfgBrowseExecutable, &d);
    (new Fl_Button(10, 130, 140, 30, "Open data folder"))->callback(cfgOpenData);
    (new Fl_Button(350, 130, 80, 30, "OK"))->callback(cfgOk, &d);
    (new Fl_Button(440, 130, 70, 30, "Cancel"))->callback(cfgCancel, &d);
    d.win->end();
    setMainWindowActive(false);
    runModal(d.win);
    setMainWindowActive(true);
    delete d.win;
}

// ================================================== main window callbacks ===

static void onAddMode(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    const char* in = fl_input("New %s name:", "", c->singular);
    if (!in) return;
    std::string name = in;
    if (!validName(name)) {
        setStatus("Invalid name (avoid  \\ / : * ? \" < > | =  and leading/trailing spaces).");
        return;
    }
    fs::path f = modeFile(*c, name);
    std::error_code ec;
    if (fs::exists(f, ec)) { setStatus("'" + name + "' already exists."); return; }
    if (!saveMode(f, {})) { setStatus("Could not create the mode file."); return; }
    refresh(*c, name);
    saveConfig();
    editMode(*c, name);
}

static void onRemoveMode(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    std::string name = selectedName(*c);
    if (name.empty()) { setStatus("Select a " + std::string(c->singular) + " to remove."); return; }
    if (fl_choice("Delete %s '%s'?\n(Saved config versions are not affected.)", "Cancel", "Delete", nullptr,
                  c->singular, name.c_str()) != 1)
        return;
    std::error_code ec;
    fs::remove(modeFile(*c, name), ec);
    refresh(*c);
    setStatus(ec ? "Error: " + ec.message() : "Removed '" + name + "'.");
    saveConfig();
}

static void onEditMode(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    std::string name = selectedName(*c);
    if (name.empty()) { setStatus("Select a " + std::string(c->singular) + " to edit."); return; }
    editMode(*c, name);
}

static int runSynchronousCommand(const std::string& cmdLine, const fs::path& workDir) {
    if (cmdLine.empty()) return 0;
#ifdef _WIN32
    fs::path oldDir = fs::current_path();
    std::error_code ec;
    fs::current_path(workDir, ec);
    int r = std::system(cmdLine.c_str());
    if (!ec) fs::current_path(oldDir, ec);
    return r;
#else
    std::string fullCmd = "cd -- ";
    std::string qdir = "'";
    for (char ch : S(workDir)) { if (ch == '\'') qdir += "'\\''"; else qdir += ch; }
    qdir += "'";
    fullCmd += qdir + " && " + cmdLine;
    int status = std::system(fullCmd.c_str());
    if (status == -1) return -1;
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}

static void onLaunch(Fl_Widget*, void*) {
    if (g_executablePath.empty()) {
        setStatus("Configure the executable first in Settings.");
        return;
    }

    std::error_code ec;
    fs::path executable = fs::absolute(P(g_executablePath), ec);
    if (ec || !fs::is_regular_file(executable, ec)) {
        setStatus("Configured executable does not exist: " + g_executablePath);
        return;
    }

    for (auto& c : g_cats) {
        std::string modeName = selectedName(c);
        if (modeName.empty()) continue;
        for (auto& e : loadMode(modeFile(c, modeName))) {
            if (e.file == "__pre_launch_commands__") {
                fs::path cmdPath = versionPath(e.file, e.version, false);
                if (fs::is_regular_file(cmdPath, ec)) {
                    auto cmds = readLines(cmdPath);
                    for (auto& cmd : cmds) {
                        int exitCode = runSynchronousCommand(cmd, executable.parent_path());
                        if (exitCode != 0) {
                            std::string errMsg = "Pre-launch command failed (exit code " + std::to_string(exitCode) + "): " + cmd;
                            setStatus(errMsg);
                            debugLog(errMsg);
                        }
                    }
                }
            }
        }
    }

#ifdef _WIN32
    HINSTANCE result = ShellExecuteW(nullptr, L"open", executable.wstring().c_str(),
                                     nullptr, executable.parent_path().wstring().c_str(), SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        setStatus("Could not launch " + S(executable) + ".");
        return;
    }
#else
    pid_t pid = fork();
    if (pid < 0) {
        setStatus("Could not launch " + S(executable) + ".");
        return;
    }
    if (pid == 0) {
        if (chdir(executable.parent_path().c_str()) != 0) _exit(126);
        execl(executable.c_str(), executable.c_str(), (char*)nullptr);
        _exit(127);
    }
#endif
    setStatus("Launched " + S(executable) + ".");
}

static void onApply(Fl_Widget*, void*) {
    if (!gameOk()) { setStatus("Game folder is not set or does not exist. Open Settings."); return; }

    struct Item { std::string rel, ver, from; };
    std::vector<Item> plan;
    std::vector<std::string> overridden;
    std::string modes;

    for (auto& c : g_cats) {
        std::string name = selectedName(c);
        if (name.empty()) continue;
        std::string label = std::string(c.singular) + " '" + name + "'";
        modes += (modes.empty() ? "" : ", ") + label;
        for (auto& e : loadMode(modeFile(c, name))) {
            std::string rel = e.file;
            if (rel != "__pre_launch_commands__" && (!cleanRel(rel) || !validName(e.version))) {
                setStatus("Invalid entry '" + e.file + "=" + e.version + "' in " + label + ". Nothing was changed.");
                return;
            }
            bool found = false;
            for (auto& it : plan)
                if (it.rel == rel) {
                    overridden.push_back(rel);
                    it.ver = e.version;
                    it.from = label;
                    found = true;
                }
            if (!found) plan.push_back({rel, e.version, label});
        }
    }
    if (modes.empty()) { setStatus("Select a resolution and/or a controller first."); return; }
    if (plan.empty()) { setStatus("The selected modes have no config entries. Use Edit... to add some."); return; }

    std::error_code ec;
    for (auto& it : plan) {
        if (it.rel == "__pre_launch_commands__") {
            fs::path cmdPath = versionPath(it.rel, it.ver, false);
            if (!fs::is_regular_file(cmdPath, ec)) {
                setStatus("Pre-launch commands version '" + it.ver + "' is missing (used by " + it.from + ").");
                return;
            }
            continue;
        }
        fs::path patchPath = versionPath(it.rel, it.ver, true);
        fs::path fullPath = versionPath(it.rel, it.ver, false);
        if (!fs::is_regular_file(patchPath, ec) && !fs::is_regular_file(fullPath, ec)) {
            setStatus("Version '" + it.ver + "' of " + it.rel + " is missing (used by " + it.from +
                      "). Nothing was changed.");
            return;
        }
    }

    for (auto& it : plan) {
        if (it.rel == "__pre_launch_commands__") continue;
        std::string err;
        if (!backupOriginal(it.rel, err)) {
            setStatus("Could not back up " + it.rel + ": " + err + ". Nothing was changed.");
            return;
        }
    }

    int n = 0;
    for (auto& it : plan) {
        if (it.rel == "__pre_launch_commands__") {
            ++n;
            continue;
        }
        fs::path dst = gamePath() / P(it.rel);
        fs::create_directories(dst.parent_path(), ec);

        fs::path patchPath = versionPath(it.rel, it.ver, true);
        fs::path fullPath = versionPath(it.rel, it.ver, false);

        if (fs::is_regular_file(patchPath, ec)) {
            auto origLines = readLines(g_originalsDir / P(it.rel));
            auto patchLines = readLines(patchPath);
            auto mergedLines = applyPatch(origLines, patchLines);
            if (!writeLines(dst, mergedLines)) {
                setStatus("Error writing patched " + it.rel + ". Nothing was changed.");
                return;
            }
        } else if (fs::is_regular_file(fullPath, ec)) {
            fs::copy_file(fullPath, dst, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                setStatus("Error writing " + it.rel + ": " + ec.message() +
                          ". The game may be partly updated; originals are safe, use Restore originals.");
                return;
            }
        }
        ++n;
    }
    saveConfig();
    std::string msg = "Applied " + std::to_string(n) + " config(s) for " + modes + ".";
    if (!overridden.empty())
        msg += " Note: " + overridden[0] + (overridden.size() > 1 ? " (and others)" : "") +
               " was set by both modes; the later one (controller) won.";
    setStatus(msg);
}

// ====================================================================== GUI ==

static void onCategorySelection(Fl_Widget*, void*) {
    updateApplyButton();
}

static void buildCategory(Category& c, int x, int y, int w, int h) {
    mkLabel(x, y, w, c.title, true);
    c.list = mkBrowser(x, y + 22, w, h);
    c.list->callback(onCategorySelection);
    int bw = (w - 6) / 3, by = y + 22 + h + 5;
    (new Fl_Button(x, by, bw, 25, "Add"))->callback(onAddMode, &c);
    (new Fl_Button(x + bw + 3, by, bw, 25, "Remove"))->callback(onRemoveMode, &c);
    (new Fl_Button(x + 2 * (bw + 3), by, bw, 25, "Edit..."))->callback(onEditMode, &c);
}

int main(int argc, char** argv) {
    g_base = exeDir(argv);
    g_cfgFile = g_base / "launcher.cfg";
    g_modesDir = g_base / "modes";
    g_versionsDir = g_base / "versions";
    g_originalsDir = g_base / "originals";

    std::error_code ec;
    fs::create_directories(g_versionsDir, ec);
    fs::create_directories(g_originalsDir, ec);
    for (auto& c : g_cats) {
        c.dir = g_modesDir / c.subdir;
        fs::create_directories(c.dir, ec);
    }
    auto cfg = loadConfig();
    g_gameFolder = cfg["game_path"];
    g_executablePath = cfg["executable_path"];

    Fl::scheme("gtk+");
    Fl_Window* win = new Fl_Window(540, 500, "Game Launcher");
    g_mainWindow = win;
    setMainWindowActive(true);

    (new Fl_Button(10, 10, 140, 28, "Settings..."))->callback(onSettings);
    (new Fl_Button(160, 10, 140, 28, "Config Versions..."))->callback(onVersions);

    g_gameLabel = mkLabel(10, 45, 520, "");

    buildCategory(g_cats[0], 10, 75, 255, 190);
    buildCategory(g_cats[1], 275, 75, 255, 190);

    g_applyButton = new Fl_Button(10, 330, 360, 35, "Apply");
    g_applyButton->labelfont(FL_BOLD);
    g_applyButton->callback(onApply);
    (new Fl_Button(380, 330, 150, 35, "Restore originals"))->callback(onRestore);

    Fl_Button* launch = new Fl_Button(10, 375, 520, 55, "LAUNCH GAME");
    launch->color(FL_RED);
    launch->labelcolor(FL_WHITE);
    launch->labelfont(FL_BOLD);
    launch->labelsize(18);
    launch->callback(onLaunch);

    g_status = new Fl_Box(10, 440, 520, 45);
    g_status->box(FL_DOWN_BOX);
    g_status->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    win->end();

    updateGameLabel();
    for (auto& c : g_cats) refresh(c, cfg[c.subdir]);
    if (g_gameFolder.empty()) setStatus("First run: open Settings and set the game folder.");

    win->show(argc, argv);
    int rc = Fl::run();
    saveConfig();
    return rc;
}
