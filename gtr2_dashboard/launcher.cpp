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
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
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

static std::string trimCopy(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

// Splits editor text into lines (handles \r\n; a final newline does not add an empty line).
static std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string cur;
    for (char ch : text) {
        if (ch == '\n') { lines.push_back(cur); cur.clear(); }
        else if (ch != '\r') cur += ch;
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

static std::string joinLines(const std::vector<std::string>& lines) {
    std::string out;
    for (auto& l : lines) out += l + "\n";
    return out;
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

// 1-based index of the first selected row of a multi-select browser, or 0.
static int selectedRow(Fl_Multi_Browser* b) {
    for (int i = 1; i <= b->size(); ++i)
        if (b->selected(i)) return i;
    return b->value();
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

static std::vector<size_t> computeDiffIndices(const std::vector<std::string>& origLines,
                                              const std::vector<std::string>& extLines);

static std::vector<std::string> computeDiffLines(const std::vector<std::string>& origLines, const std::vector<std::string>& extLines) {
    std::vector<std::string> diffs;
    for (size_t index : computeDiffIndices(origLines, extLines))
        diffs.push_back(extLines[index]);
    return diffs;
}

static std::vector<size_t> computeDiffIndices(const std::vector<std::string>& origLines,
                                              const std::vector<std::string>& extLines) {
    std::map<std::string, std::string> origMap;
    for (const auto& l : origLines) {
        std::string k = extractKey(l);
        if (!k.empty()) origMap[k] = l;
    }

    std::vector<size_t> diffs;
    for (size_t i = 0; i < extLines.size(); ++i) {
        const auto& l = extLines[i];
        std::string k = extractKey(l);
        if (!k.empty()) {
            auto it = origMap.find(k);
            if (it == origMap.end() || it->second != l) diffs.push_back(i);
        } else if (std::find(origLines.begin(), origLines.end(), l) == origLines.end()) {
            diffs.push_back(i);
        }
    }
    return diffs;
}

static int nextSelectedIndex(const std::vector<bool>& selected, int current, int direction) {
    if (selected.empty() || direction == 0) return -1;
    const int count = static_cast<int>(selected.size());
    int index = current;
    for (int step = 0; step < count; ++step) {
        index = (index + direction + count) % count;
        if (selected[index]) return index;
    }
    return -1;
}

static bool undoSelection(std::vector<bool>& current, std::vector<std::vector<bool>>& history) {
    if (history.empty()) return false;
    current = std::move(history.back());
    history.pop_back();
    return true;
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

static bool hasModeSelection() {
    for (auto& c : g_cats)
        if (c.list && !selectedName(c).empty()) return true;
    return false;
}

static void updateApplyButton() {
    if (!g_applyButton) return;
    g_applyButton->color(hasModeSelection() ? applyReadyColor() : FL_BACKGROUND_COLOR);
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

enum class VerKind { Full, Patch, Commands };

static const char* const kPreLaunchRel = "__pre_launch_commands__";
static const char* const kPreLaunchDisplay = "Pre-launch Commands";

static std::string kindSuffix(VerKind k) {
    switch (k) {
        case VerKind::Patch: return " [patch]";
        case VerKind::Commands: return " [pre-launch]";
        default: return " [full]";
    }
}

struct ParsedVersion {
    std::string name;
    VerKind kind = VerKind::Full;
    bool hadSuffix = false;
};

// Splits a Versions-list row such as "xbox [patch]" into name and kind. The suffix length
// always comes from the suffix string itself (a hard-coded length once broke "[pre-launch]").
static ParsedVersion parseVersionDisplayEx(const std::string& text) {
    for (VerKind k : {VerKind::Full, VerKind::Patch, VerKind::Commands}) {
        const std::string suffix = kindSuffix(k);
        if (text.size() > suffix.size() &&
            text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0)
            return {text.substr(0, text.size() - suffix.size()), k, true};
    }
    return {text, VerKind::Full, false};
}

[[maybe_unused]] static std::pair<std::string, bool> parseVersionDisplay(const std::string& itemText) {
    ParsedVersion p = parseVersionDisplayEx(itemText);
    return {p.name, p.kind == VerKind::Patch};
}

static std::string versionDisplay(const std::string& name, VerKind k) { return name + kindSuffix(k); }
static std::string displayRel(const std::string& rel) { return rel == kPreLaunchRel ? kPreLaunchDisplay : rel; }
static std::string realRel(const std::string& disp) { return disp == kPreLaunchDisplay ? kPreLaunchRel : disp; }

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

static bool versionExists(const std::string& rel, const std::string& ver) {
    std::error_code ec;
    if (rel == kPreLaunchRel) return fs::is_regular_file(versionPath(rel, ver, false), ec);
    return fs::is_regular_file(versionPath(rel, ver, false), ec) ||
           fs::is_regular_file(versionPath(rel, ver, true), ec);
}

// Mode files written by older builds may contain the list suffix ("default [pre-launch]").
// If the clean name exists, use it so such entries are not reported as missing.
static std::string healVersionName(const std::string& rel, const std::string& ver) {
    if (versionExists(rel, ver)) return ver;
    ParsedVersion p = parseVersionDisplayEx(ver);
    if (p.hadSuffix && versionExists(rel, p.name)) return p.name;
    return ver;
}

static std::vector<Entry> loadModeResolved(const fs::path& f) {
    auto v = loadMode(f);
    for (auto& e : v) e.version = healVersionName(e.file, e.version);
    return v;
}

static std::vector<std::string> modesUsing(const std::string& rel, const std::string& ver) {
    std::vector<std::string> out;
    for (auto& c : g_cats)
        for (auto& n : scanModes(c.dir))
            for (auto& e : loadModeResolved(modeFile(c, n)))
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

static std::vector<fs::path> listOriginals() {
    std::vector<fs::path> rels;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(g_originalsDir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        if (it->is_regular_file(e2)) rels.push_back(it->path().lexically_relative(g_originalsDir));
    }
    std::sort(rels.begin(), rels.end());
    return rels;
}

static bool restoreOriginals(const std::vector<fs::path>& rels, int& restored, std::string& error) {
    restored = 0;
    for (auto& r : rels) {
        std::error_code ec;
        fs::path dst = gamePath() / r;
        fs::create_directories(dst.parent_path(), ec);
        fs::copy_file(g_originalsDir / r, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) { error = "Error restoring " + r.generic_u8string() + ": " + ec.message(); return false; }
        ++restored;
    }
    return true;
}

static void onRestore(Fl_Widget*, void*) {
    if (!gameOk()) { setStatus("Game folder is not set or does not exist."); return; }
    std::vector<fs::path> rels = listOriginals();
    if (rels.empty()) { setStatus("No originals have been saved yet."); return; }
    if (fl_choice("Restore %d original file(s) into the game folder?\n"
                  "This overwrites the files currently in the game.",
                  "Cancel", "Restore", nullptr, (int)rels.size()) != 1)
        return;
    int ok = 0;
    std::string err;
    if (!restoreOriginals(rels, ok, err)) { setStatus(err); return; }
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
    Fl_Check_Button* blockingCb = nullptr;
    Fl_Button *addCmdBtn = nullptr, *editCmdBtn = nullptr, *removeCmdBtn = nullptr;
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
        if (f->editCmdBtn) f->editCmdBtn->show();
        if (f->removeCmdBtn) f->removeCmdBtn->show();

        if (f->blockingCb) f->blockingCb->show();
        f->fileLabel->copy_label("Pre-launch commands (commands run in parallel; sleep/timeout commands wait):");
        f->nameLabel->copy_label("Command List Version Name (e.g., default, high_perf):");
    } else {
        if (f->cmdsBrowser) f->cmdsBrowser->hide();
        if (f->addCmdBtn) f->addCmdBtn->hide();
        if (f->editCmdBtn) f->editCmdBtn->hide();
        if (f->removeCmdBtn) f->removeCmdBtn->hide();
        if (f->blockingCb) f->blockingCb->hide();

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
    std::map<std::string, std::string> comparisonByKey;
    std::vector<std::string> comparisonByIndex;
    std::vector<bool> currentSelection;
    std::vector<std::vector<bool>> selectionHistory;
    bool updating = false;
    int searchMatch = -1;
};

static void patchLinesRefreshStyles(PatchLinesDlg* d);

static std::string patchLineContent(const PatchLinesDlg* d, size_t index) {
    if (!d->currentSelection[index]) return d->fileLines[index];

    std::string key = extractKey(d->fileLines[index]);
    if (!key.empty()) {
        auto comparison = d->comparisonByKey.find(key);
        if (comparison != d->comparisonByKey.end()) return comparison->second;
    }
    if (index < d->comparisonByIndex.size() && !d->comparisonByIndex[index].empty())
        return d->comparisonByIndex[index];
    return d->fileLines[index];
}

// Output lines of the patch in file order; selected lines show the comparison-file version.
static std::vector<std::string> patchSelectionOutput(const PatchLinesDlg* d) {
    std::vector<std::string> out;
    for (size_t i = 0; i < d->currentSelection.size(); ++i) {
        if (!d->currentSelection[i]) continue;
        std::string key = extractKey(d->fileLines[i]);
        auto comparison = d->comparisonByKey.find(key);
        if (!key.empty() && comparison != d->comparisonByKey.end()) {
            out.push_back(comparison->second);
        } else if (i < d->comparisonByIndex.size() && !d->comparisonByIndex[i].empty()) {
            out.push_back(d->comparisonByIndex[i]);
        } else {
            out.push_back(d->fileLines[i]);
        }
    }
    return out;
}

// Default selection: lines previously chosen, plus (for an external file) every keyed line
// whose value differs from the comparison file.
static void initPatchDialogState(PatchLinesDlg& d, const std::vector<std::string>& initialSelected,
                                 const std::vector<std::string>& comparisonLines) {
    std::map<std::string, bool> initialMap;
    for (const auto& s : initialSelected) initialMap[s] = true;

    d.currentSelection.assign(d.fileLines.size(), false);
    d.comparisonByKey.clear();
    d.comparisonByIndex.assign(d.fileLines.size(), "");
    std::map<std::string, bool> changedComparisonKeys;
    if (!comparisonLines.empty()) {
        auto changed = computeDiffIndices(d.fileLines, comparisonLines);
        for (size_t index : changed) {
            std::string key = extractKey(comparisonLines[index]);
            if (!key.empty()) {
                d.comparisonByKey[key] = comparisonLines[index];
                changedComparisonKeys[key] = true;
            }
        }
        for (size_t i = 0; i < d.fileLines.size() && i < comparisonLines.size(); ++i) {
            if (extractKey(d.fileLines[i]).empty() && d.fileLines[i] != comparisonLines[i])
                d.comparisonByIndex[i] = comparisonLines[i];
        }
    }
    for (size_t i = 0; i < d.fileLines.size(); ++i) {
        std::string key = extractKey(d.fileLines[i]);
        d.currentSelection[i] = initialMap[d.fileLines[i]] ||
                                (!key.empty() && changedComparisonKeys[key]);
    }
}

static bool containsIgnoreCase(const std::string& text, const std::string& query) {
    if (query.empty()) return false;
    auto it = std::search(text.begin(), text.end(), query.begin(), query.end(),
                          [](char a, char b) {
                              return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
                          });
    return it != text.end();
}

// Index of the first line at or after `start` (wrapping) that contains `query`, ignoring case.
static int findLineIgnoreCase(const std::vector<std::string>& lines, const std::string& query, int start) {
    if (query.empty() || lines.empty()) return -1;
    size_t begin = start < 0 ? 0 : static_cast<size_t>(start);
    for (size_t offset = 0; offset < lines.size(); ++offset) {
        size_t index = (begin + offset) % lines.size();
        if (containsIgnoreCase(lines[index], query)) return static_cast<int>(index);
    }
    return -1;
}

static void patchLinesFind(Fl_Widget*, void* p) {
    PatchLinesDlg* d = (PatchLinesDlg*)p;
    std::string query = d->search->value();
    if (query.empty() || d->fileLines.empty()) {
        d->searchMatch = -1;
        patchLinesRefreshStyles(d);
        return;
    }

    d->searchMatch = -1;
    int found = findLineIgnoreCase(d->fileLines, query, d->searchStart);
    if (found >= 0) {
        d->searchStart = static_cast<int>((found + 1) % d->fileLines.size());
        d->searchMatch = found;
        d->lines->middleline(found + 1);
        patchLinesRefreshStyles(d);
        d->lines->redraw();
        return;
    }
    patchLinesRefreshStyles(d);
    fl_alert("No lines found for:\n%s", query.c_str());
}

static std::string patchLineDisplay(size_t index, const std::string& line, bool selected, int searchMatch = -1) {
    // FLTK browser format codes let unselected lines remain readable but visually secondary.
    const char format = '\x01';
    std::string display = std::to_string(index + 1) + ": " + line;
    std::string styled;
    if (!selected) {
        styled = std::string(1, format) + "C" + std::to_string(fl_rgb_color(155, 155, 155));
    }
    if (static_cast<int>(index) == searchMatch)
        styled += std::string(1, format) + "B" + std::to_string(fl_rgb_color(35, 105, 55));
    if (styled.empty()) return display;
    return styled + std::string(1, format) + "." + display;
}

static void patchLinesRefreshStyles(PatchLinesDlg* d) {
    if (d->updating) return;
    d->updating = true;
    for (size_t i = 0; i < d->fileLines.size(); ++i)
        d->lines->text(static_cast<int>(i + 1),
                       patchLineDisplay(i, patchLineContent(d, i), d->currentSelection[i], d->searchMatch).c_str());
    d->lines->redraw();
    d->updating = false;
}

static void patchLinesSelectionChanged(Fl_Widget*, void* p) {
    PatchLinesDlg* d = (PatchLinesDlg*)p;
    if (d->updating) return;
    std::vector<bool> next(d->fileLines.size(), false);
    for (size_t i = 0; i < next.size(); ++i)
        next[i] = d->lines->selected(static_cast<int>(i + 1)) != 0;
    if (next != d->currentSelection) {
        d->selectionHistory.push_back(d->currentSelection);
        d->currentSelection = std::move(next);
        patchLinesRefreshStyles(d);
    }
}

static void patchLinesFocusSelected(PatchLinesDlg* d, int direction) {
    int current = d->lines->value() - 1;
    int next = nextSelectedIndex(d->currentSelection, current, direction);
    if (next < 0) return;
    d->lines->middleline(next + 1);
    d->lines->value(next + 1);
    d->lines->redraw();
}

static void patchLinesUndo(Fl_Widget*, void* p) {
    PatchLinesDlg* d = (PatchLinesDlg*)p;
    if (!undoSelection(d->currentSelection, d->selectionHistory)) return;
    d->updating = true;
    for (size_t i = 0; i < d->currentSelection.size(); ++i)
        d->lines->select(static_cast<int>(i + 1), d->currentSelection[i] ? 1 : 0);
    d->updating = false;
    patchLinesRefreshStyles(d);
}

static void patchLinesNext(Fl_Widget*, void* p) {
    patchLinesFocusSelected((PatchLinesDlg*)p, 1);
}

static void patchLinesPrevious(Fl_Widget*, void* p) {
    patchLinesFocusSelected((PatchLinesDlg*)p, -1);
}

static void patchLinesOk(Fl_Widget*, void* p) {
    PatchLinesDlg* d = (PatchLinesDlg*)p;
    d->selected = patchSelectionOutput(d);
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

static bool selectPatchLines(const fs::path& file,
                             const std::vector<std::string>& initialSelected,
                             const std::vector<std::string>& comparisonLines,
                             std::vector<std::string>& out) {
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
    d.lines->format_char('\x01');
    d.lines->callback(patchLinesSelectionChanged, &d);
    d.fileLines = readLines(file);
    
    initPatchDialogState(d, initialSelected, comparisonLines);
    for (size_t i = 0; i < d.fileLines.size(); ++i) {
        bool selected = d.currentSelection[i];
        d.lines->add(patchLineDisplay(i, patchLineContent(&d, i), selected, d.searchMatch).c_str());
        if (selected) d.lines->select(static_cast<int>(i + 1));
    }
    (new Fl_Button(10, 450, 130, 30, "Previous selected"))->callback(patchLinesPrevious, &d);
    (new Fl_Button(145, 450, 130, 30, "Next selected"))->callback(patchLinesNext, &d);
    (new Fl_Button(280, 450, 130, 30, "Undo change"))->callback(patchLinesUndo, &d);
    (new Fl_Button(560, 490, 90, 30, "OK"))->callback(patchLinesOk, &d);
    (new Fl_Button(660, 490, 90, 30, "Cancel"))->callback(patchLinesCancel, &d);
    d.win->end();
    runModal(d.win);
    delete d.win;
    if (!d.ok) return false;
    out = patchSelectionOutput(&d);
    return true;
}

static void afSelectPatch(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    std::string rel = f->fileIn->value();
    if (!cleanRel(rel)) {
        fl_alert("%s", "Select a valid file inside the game folder first.");
        return;
    }
    std::vector<std::string> comparisonLines;
    fs::path displayFile = gamePath() / P(rel);
    if (f->extPatchCb_->value()) {
        comparisonLines = readLines(P(f->extFilePath_));
        std::error_code ec;
        fs::path original = g_originalsDir / P(rel);
        if (!fs::is_regular_file(original, ec)) {
            std::string error;
            if (!backupOriginal(rel, error)) {
                fl_alert("Could not save the original for comparison:\n%s", error.c_str());
                return;
            }
        }
        if (fs::is_regular_file(original, ec)) displayFile = original;
    }
    std::vector<std::string> selected;
    if (selectPatchLines(displayFile, f->patchLines_, comparisonLines, selected))
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

struct CommandSpec {
    std::string command;
    bool blocking = true;
};

static CommandSpec parseCommandSpec(const std::string& line) {
    constexpr const char* blockingPrefix = "@blocking\t";
    constexpr const char* nonBlockingPrefix = "@nonblocking\t";
    if (line.rfind(nonBlockingPrefix, 0) == 0)
        return {line.substr(std::strlen(nonBlockingPrefix)), false};
    if (line.rfind(blockingPrefix, 0) == 0)
        return {line.substr(std::strlen(blockingPrefix)), true};
    constexpr const char* parallelDisplay = "[parallel] ";
    constexpr const char* blockingDisplay = "[blocking] ";
    if (line.rfind(parallelDisplay, 0) == 0)
        return {line.substr(std::strlen(parallelDisplay)), false};
    if (line.rfind(blockingDisplay, 0) == 0)
        return {line.substr(std::strlen(blockingDisplay)), true};
    return {line, true};
}

// Non-blank, trimmed commands shown in the list, in order.
static std::vector<std::string> collectCommands(Fl_Multi_Browser* b) {
    std::vector<std::string> cmds;
    for (int i = 1; i <= b->size(); ++i) {
        CommandSpec spec = parseCommandSpec(b->text(i));
        spec.command = trimCopy(spec.command);
        if (!spec.command.empty()) cmds.push_back(spec.command);
    }
    return cmds;
}

static std::string serializeCommandSpec(const CommandSpec& spec) {
    return std::string(spec.blocking ? "@blocking\t" : "@nonblocking\t") + spec.command;
}

static std::string displayCommandSpec(const CommandSpec& spec) {
    return std::string(spec.blocking ? "[blocking] " : "[parallel] ") + spec.command;
}

static CommandSpec commandAt(Fl_Multi_Browser* b, int row) {
    return parseCommandSpec(b->text(row));
}

static std::vector<std::string> collectCommandLines(Fl_Multi_Browser* b) {
    std::vector<std::string> lines;
    for (int i = 1; i <= b->size(); ++i) {
        CommandSpec spec = commandAt(b, i);
        spec.command = trimCopy(spec.command);
        if (!spec.command.empty()) lines.push_back(serializeCommandSpec(spec));
    }
    return lines;
}

static void afCommandSelectionCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    int row = selectedRow(f->cmdsBrowser);
    if (row && f->blockingCb)
        f->blockingCb->value(commandAt(f->cmdsBrowser, row).blocking ? 1 : 0);
}

static void afBlockingChangedCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    int row = selectedRow(f->cmdsBrowser);
    if (!row) return;
    CommandSpec spec = commandAt(f->cmdsBrowser, row);
    spec.blocking = f->blockingCb->value() != 0;
    f->cmdsBrowser->text(row, displayCommandSpec(spec).c_str());
}

static bool afAppendCommand(AddFormDlg* f, const std::string& text, std::string& error) {
    std::string cmd = trimCopy(text);
    if (cmd.empty()) { error = "A command cannot be empty."; return false; }
    CommandSpec spec{cmd, !f->blockingCb || f->blockingCb->value() != 0};
    f->cmdsBrowser->add(displayCommandSpec(spec).c_str());
    return true;
}

static bool afReplaceCommand(AddFormDlg* f, int row, const std::string& text, std::string& error) {
    if (row < 1 || row > f->cmdsBrowser->size()) { error = "Select a command from the list to edit."; return false; }
    std::string cmd = trimCopy(text);
    if (cmd.empty()) { error = "A command cannot be empty. Use Remove command to delete it."; return false; }
    CommandSpec spec = commandAt(f->cmdsBrowser, row);
    spec.command = cmd;
    f->cmdsBrowser->text(row, displayCommandSpec(spec).c_str());
    return true;
}

static void afAddCmdCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    const char* cmdIn = fl_input("Enter command to run before launch:", "");
    if (!cmdIn) return;
    std::string err;
    if (!trimCopy(cmdIn).empty() && !afAppendCommand(f, cmdIn, err)) fl_alert("%s", err.c_str());
}

static void afEditCmdCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    int row = selectedRow(f->cmdsBrowser);
    if (!row) { fl_alert("%s", "Select a command from the list to edit."); return; }
    CommandSpec spec = commandAt(f->cmdsBrowser, row);
    const char* in = fl_input("Edit command:", spec.command.c_str());
    if (!in) return;
    std::string err;
    if (!afReplaceCommand(f, row, in, err)) fl_alert("%s", err.c_str());
}

static void afRemoveCmdCb(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    int idx = selectedRow(f->cmdsBrowser);
    if (!idx) {
        fl_alert("%s", "Select a command from the list to remove.");
        return;
    }
    f->cmdsBrowser->remove(idx);
    afCommandSelectionCb(nullptr, f);
}

// ---- version operations (shared by the dialogs; GUI-free so they can be tested) ----

static const char* const kInvalidNameMsg =
    "Invalid version name.\nAvoid  \\ / : * ? \" < > | =  and leading/trailing spaces.";

// Creates a new named version. `lines` are the patch lines (Patch) or the commands (Commands);
// Full copies the current game file. Names are unique per tracked config regardless of kind.
static bool createVersion(const std::string& relIn, const std::string& ver, VerKind kind,
                          const std::vector<std::string>& lines, std::string& error,
                          std::string* relOut = nullptr) {
    if (!gameOk()) { error = "Set the game folder first (Settings)."; return false; }
    std::string rel = relIn;
    std::vector<std::string> content;
    if (kind == VerKind::Commands) {
        rel = kPreLaunchRel;
        for (auto& l : lines) {
            std::string t = trimCopy(l);
            if (!t.empty()) content.push_back(t);
        }
        if (content.empty()) { error = "Add at least one pre-launch command."; return false; }
        if (!validName(ver)) { error = kInvalidNameMsg; return false; }
    } else {
        if (!cleanRel(rel)) {
            error = "Invalid file path.\nUse a path relative to the game folder, e.g. config/input.ini";
            return false;
        }
        if (!validName(ver)) { error = kInvalidNameMsg; return false; }
        fs::path from = gamePath() / P(rel);
        std::error_code ec;
        if (!fs::is_regular_file(from, ec)) { error = "Game file not found:\n" + S(from); return false; }
        if (kind == VerKind::Patch && lines.empty()) {
            error = "Use 'Select patch lines...' to choose at least one line.";
            return false;
        }
        std::string err;
        if (!backupOriginal(rel, err)) { error = "Could not back up the original:\n" + err; return false; }
        content = lines;
    }
    if (versionExists(rel, ver)) { error = "Version '" + ver + "' of that config already exists."; return false; }

    fs::path dst = versionPath(rel, ver, kind == VerKind::Patch);
    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (kind == VerKind::Full) {
        fs::copy_file(gamePath() / P(rel), dst, fs::copy_options::none, ec);
        if (ec) { error = "Could not save the version:\n" + ec.message(); return false; }
    } else if (!writeLines(dst, content)) {
        error = kind == VerKind::Commands ? "Could not save pre-launch commands version file."
                                          : "Could not save the patch version file.";
        return false;
    }
    if (relOut) *relOut = rel;
    return true;
}

static bool deleteVersion(const std::string& rel, const std::string& name, VerKind kind) {
    std::error_code ec;
    fs::path file = versionPath(rel, name, kind == VerKind::Patch);
    if (!fs::remove(file, ec) || ec) return false;
    fs::path dir = file.parent_path();
    while (!dir.empty() && dir != g_versionsDir && fs::is_empty(dir, ec) && !ec) {
        fs::path parent = dir.parent_path();
        fs::remove(dir, ec);
        dir = parent;
    }
    return true;
}

// Copies an existing version to a new name for the same tracked config, keeping its kind.
static bool cloneVersion(const std::string& rel, const std::string& name, VerKind kind,
                         const std::string& newName, std::string& error) {
    std::error_code ec;
    fs::path src = versionPath(rel, name, kind == VerKind::Patch);
    if (!fs::is_regular_file(src, ec)) { error = "Version '" + name + "' no longer exists."; return false; }
    if (!validName(newName)) { error = kInvalidNameMsg; return false; }
    if (versionExists(rel, newName)) { error = "Version '" + newName + "' of that config already exists."; return false; }
    fs::path dst = versionPath(rel, newName, kind == VerKind::Patch);
    fs::create_directories(dst.parent_path(), ec);
    fs::copy_file(src, dst, fs::copy_options::none, ec);
    if (ec) { error = "Could not clone the version:\n" + ec.message(); return false; }
    return true;
}

// Replaces the contents of an existing version in place (the name never changes).
static bool saveEditedVersion(const std::string& rel, const std::string& name, VerKind kind,
                              const std::vector<std::string>& linesIn, std::string& error) {
    std::error_code ec;
    fs::path file = versionPath(rel, name, kind == VerKind::Patch);
    if (!fs::is_regular_file(file, ec)) { error = "Version '" + name + "' no longer exists."; return false; }
    std::vector<std::string> lines;
    if (kind == VerKind::Commands) {
        for (auto& l : linesIn) {
            std::string t = trimCopy(l);
            if (!t.empty()) lines.push_back(t);
        }
        if (lines.empty()) { error = "A pre-launch command list needs at least one command."; return false; }
    } else {
        lines = linesIn;
        if (kind == VerKind::Patch && lines.empty()) { error = "A patch needs at least one line."; return false; }
    }
    // Write beside the version then rename, so a failed save never leaves a half-written
    // version (or a stray temp file that would be listed as a version).
    fs::path tmp = file;
    tmp += ".saving";
    if (!writeLines(tmp, lines)) { fs::remove(tmp, ec); error = "Could not save the version."; return false; }
    fs::rename(tmp, file, ec);
    if (ec) { fs::remove(tmp, ec); error = "Could not save the version."; return false; }
    return true;
}

struct EditVerDlg {
    Fl_Window* win = nullptr;
    Fl_Text_Editor* editor = nullptr;
    Fl_Text_Buffer* buf = nullptr;
    std::string rel, name;
    VerKind kind = VerKind::Full;
    bool ok = false;
};

static void evSave(Fl_Widget*, void* p) {
    EditVerDlg* d = (EditVerDlg*)p;
    char* raw = d->buf->text();
    std::string text = raw ? raw : "";
    free(raw);
    std::string err;
    if (!saveEditedVersion(d->rel, d->name, d->kind, splitLines(text), err)) {
        fl_alert("%s", err.c_str());
        return;
    }
    d->ok = true;
    d->win->hide();
}

static void evCancel(Fl_Widget*, void* p) { ((EditVerDlg*)p)->win->hide(); }

static bool editVersionDialog(const std::string& rel, const std::string& name, VerKind kind) {
    std::error_code ec;
    fs::path file = versionPath(rel, name, kind == VerKind::Patch);
    if (!fs::is_regular_file(file, ec)) {
        fl_alert("Version file not found:\n%s", S(file).c_str());
        return false;
    }
    EditVerDlg d;
    d.rel = rel; d.name = name; d.kind = kind;
    std::string title = "Edit version '" + name + "' of " + displayRel(rel);
    d.win = new Fl_Window(640, 470);
    d.win->copy_label(title.c_str());
    const char* hint = kind == VerKind::Commands ? "One command per line, run in order before the game launches:"
                     : kind == VerKind::Patch    ? "One patch line per row; key=value lines replace matching keys:"
                                                 : "Full file contents (replace the game file when applied):";
    mkLabel(10, 8, 620, hint, true);
    d.editor = new Fl_Text_Editor(10, 32, 620, 380);
    d.buf = new Fl_Text_Buffer;
    d.editor->buffer(d.buf);
    d.editor->textfont(FL_COURIER);
    d.buf->text(joinLines(readLines(file)).c_str());
    (new Fl_Button(440, 425, 90, 30, "Save"))->callback(evSave, &d);
    (new Fl_Button(540, 425, 90, 30, "Cancel"))->callback(evCancel, &d);
    d.win->end();
    runModal(d.win);
    d.editor->buffer(nullptr);
    delete d.win;
    delete d.buf;
    return d.ok;
}

static void afOk(Fl_Widget*, void* p) {
    AddFormDlg* f = (AddFormDlg*)p;
    bool isCmds = (f->typeChoice->value() == 1);
    bool isPatch = !isCmds && (f->patchCb_->value() != 0 || f->extPatchCb_->value() != 0);
    VerKind kind = isCmds ? VerKind::Commands : (isPatch ? VerKind::Patch : VerKind::Full);
    std::string rel = f->fileIn->value();
    std::string ver = f->nameIn->value();
    std::vector<std::string> lines = isCmds ? collectCommandLines(f->cmdsBrowser) : f->patchLines_;

    std::string err, outRel;
    if (!createVersion(rel, ver, kind, lines, err, &outRel)) {
        fl_alert("%s", err.c_str());
        return;
    }
    f->ok_ = true;
    f->rel_ = outRel;
    f->ver_ = ver;
    f->isPatch_ = isPatch;
    f->isCmdList_ = isCmds;
    f->win_->hide();
}

static void afCancel(Fl_Widget*, void* p) { ((AddFormDlg*)p)->win_->hide(); }

static void buildAddVersionWindow(AddFormDlg& f, const std::string& prefillRel) {
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
    f.cmdsBrowser->callback(afCommandSelectionCb, &f);
    f.cmdsBrowser->hide();

    f.addCmdBtn = new Fl_Button(10, 188, 110, 25, "Add command");
    f.addCmdBtn->callback(afAddCmdCb, &f);
    f.addCmdBtn->hide();

    f.editCmdBtn = new Fl_Button(130, 188, 120, 25, "Edit command");
    f.editCmdBtn->callback(afEditCmdCb, &f);
    f.editCmdBtn->hide();

    f.removeCmdBtn = new Fl_Button(260, 188, 130, 25, "Remove command");
    f.removeCmdBtn->callback(afRemoveCmdCb, &f);
    f.removeCmdBtn->hide();

    f.blockingCb = new Fl_Check_Button(400, 188, 150, 25, "Wait for completion");
    f.blockingCb->value(1);
    f.blockingCb->callback(afBlockingChangedCb, &f);
    f.blockingCb->hide();

    f.nameLabel = mkLabel(10, 225, 540, "Version name (e.g. 1080p, xbox):");
    f.nameIn = new Fl_Input(10, 247, 540, 25);

    (new Fl_Button(340, 435, 130, 30, "Save version"))->callback(afOk, &f);
    (new Fl_Button(480, 435, 70, 30, "Cancel"))->callback(afCancel, &f);
    f.win_->end();

    if (!prefillRel.empty() && prefillRel != kPreLaunchDisplay) {
        f.fileIn->value(prefillRel.c_str());
    }
}

static bool addVersionForm(const std::string& prefillRel, std::string& outRel, std::string& outVer, bool& outIsPatch) {
    AddFormDlg f;
    buildAddVersionWindow(f, prefillRel);
    runModal(f.win_);
    delete f.win_;
    if (f.ok_) { outRel = f.rel_; outVer = f.ver_; outIsPatch = f.isPatch_; }
    return f.ok_;
}

// ======================================================== dialog: config versions ==

struct VerMgr {
    Fl_Window* win = nullptr;
    Fl_Hold_Browser *filesB = nullptr, *versB = nullptr;
    Fl_Button *addBtn = nullptr, *editBtn = nullptr, *cloneBtn = nullptr, *deleteBtn = nullptr;
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

struct VerSelection {
    std::string rel, dispFile, display;
    ParsedVersion ver;
};

static bool vmGetSelection(VerMgr* m, VerSelection& s) {
    int fi = m->filesB->value(), vi = m->versB->value();
    if (!fi || !vi) { fl_alert("%s", "Select a tracked config and one of its versions first."); return false; }
    s.dispFile = m->filesB->text(fi);
    s.rel = realRel(s.dispFile);
    s.display = m->versB->text(vi);
    s.ver = parseVersionDisplayEx(s.display);
    return true;
}

static void vmAddCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    int i = m->filesB->value();
    std::string pre = i ? m->filesB->text(i) : "";
    std::string rel, ver;
    bool isPatch = false;
    if (addVersionForm(pre, rel, ver, isPatch)) {
        VerKind kind = rel == kPreLaunchRel ? VerKind::Commands : (isPatch ? VerKind::Patch : VerKind::Full);
        vmReload(m, displayRel(rel), versionDisplay(ver, kind));
        setStatus("Saved version '" + ver + "' for " + displayRel(rel) + ".");
    }
}

static void vmEditCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    VerSelection s;
    if (!vmGetSelection(m, s)) return;
    if (editVersionDialog(s.rel, s.ver.name, s.ver.kind)) {
        vmReload(m, s.dispFile, s.display);
        setStatus("Saved changes to version '" + s.ver.name + "' of " + s.dispFile + ".");
    }
}

static void vmCloneCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    VerSelection s;
    if (!vmGetSelection(m, s)) return;
    std::string suggestion = s.ver.name + "_copy";
    const char* in = fl_input("Name for the cloned version:", suggestion.c_str());
    if (!in) return;
    std::string newName = in, err;
    if (!cloneVersion(s.rel, s.ver.name, s.ver.kind, newName, err)) {
        fl_alert("%s", err.c_str());
        return;
    }
    vmReload(m, s.dispFile, versionDisplay(newName, s.ver.kind));
    setStatus("Cloned version '" + s.ver.name + "' to '" + newName + "' for " + s.dispFile + ".");
}

static void vmDeleteCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    VerSelection s;
    if (!vmGetSelection(m, s)) return;

    std::string used;
    for (auto& u : modesUsing(s.rel, s.display)) used += "\n  - " + u;
    std::string warn = used.empty() ? "" : "\n\nIt is used by:" + used + "\nThose entries will be flagged as missing.";
    if (fl_choice("Delete version '%s' of %s?%s", "Cancel", "Delete", nullptr, s.ver.name.c_str(),
                  s.dispFile.c_str(), warn.c_str()) != 1)
        return;

    if (!deleteVersion(s.rel, s.ver.name, s.ver.kind))
        setStatus("Could not delete version '" + s.ver.name + "' of " + s.dispFile + ".");
    else
        setStatus("Deleted version '" + s.ver.name + "' of " + s.dispFile + ".");
    vmReload(m, s.dispFile);
}

static void vmCloseCb(Fl_Widget*, void* p) { ((VerMgr*)p)->win->hide(); }

static void buildVersionsWindow(VerMgr& m) {
    m.win = new Fl_Window(680, 390, "Config Versions");
    m.win->color(lightBlueColor());
    mkLabel(10, 10, 300, "Tracked Configs", true);
    mkLabel(320, 10, 350, "Versions", true);
    m.filesB = mkBrowser(10, 32, 300, 250);
    m.filesB->callback(vmFilesCb, &m);
    m.versB = mkBrowser(320, 32, 350, 250);
    m.addBtn = new Fl_Button(10, 287, 150, 28, "Add version...");
    m.addBtn->callback(vmAddCb, &m);
    m.editBtn = new Fl_Button(320, 287, 112, 28, "Edit version");
    m.editBtn->callback(vmEditCb, &m);
    m.cloneBtn = new Fl_Button(437, 287, 112, 28, "Clone version");
    m.cloneBtn->callback(vmCloneCb, &m);
    m.deleteBtn = new Fl_Button(554, 287, 116, 28, "Delete version");
    m.deleteBtn->callback(vmDeleteCb, &m);

    std::string info = "Originals of touched game files are saved once in:\n" + S(g_originalsDir);
    Fl_Box* ib = new Fl_Box(10, 325, 540, 55);
    ib->copy_label(info.c_str());
    ib->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    (new Fl_Button(570, 350, 100, 30, "Close"))->callback(vmCloseCb, &m);
    m.win->end();
}

static void onVersions(Fl_Widget*, void*) {
    VerMgr m;
    buildVersionsWindow(m);
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

// ---- mode-entry logic (GUI-free) ----

static Entry makeEntryFromDisplay(const std::string& dispFile, const std::string& dispVersion) {
    return {realRel(dispFile), parseVersionDisplayEx(dispVersion).name};
}

// Adds the entry, or replaces the existing entry for the same file. Returns its 0-based index.
static int setEntry(std::vector<Entry>& entries, const Entry& ne) {
    int idx = -1;
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].file == ne.file) { entries[i] = ne; idx = (int)i; }
    if (idx < 0) { entries.push_back(ne); idx = (int)entries.size() - 1; }
    return idx;
}

static bool removeEntry(std::vector<Entry>& entries, size_t index) {
    if (index >= entries.size()) return false;
    entries.erase(entries.begin() + index);
    return true;
}

static std::string modeEntryLabel(const Entry& e) {
    return displayRel(e.file) + "   ->   " + e.version + (versionExists(e.file, e.version) ? "" : "   [MISSING]");
}

static void meRefreshEntries(ModeEditor* m) {
    m->entriesB->clear();
    for (auto& e : m->entries) m->entriesB->add(modeEntryLabel(e).c_str());
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
    std::string dispFile = displayRel(e.file);
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
    int idx = setEntry(m->entries, makeEntryFromDisplay(m->filesB->text(fi), m->versB->text(vi)));
    if (!saveMode(m->file, m->entries)) fl_alert("%s", "Could not save the mode file.");
    meRefreshEntries(m);
    m->entriesB->value(idx + 1);
}

static void meRemoveCb(Fl_Widget*, void* p) {
    ModeEditor* m = (ModeEditor*)p;
    int i = m->entriesB->value();
    if (!i) { fl_alert("%s", "Select an entry to remove."); return; }
    removeEntry(m->entries, (size_t)(i - 1));
    if (!saveMode(m->file, m->entries)) fl_alert("%s", "Could not save the mode file.");
    meRefreshEntries(m);
}

static void meCloseCb(Fl_Widget*, void* p) { ((ModeEditor*)p)->win->hide(); }

static void editMode(const Category& c, const std::string& name) {
    ModeEditor m;
    m.file = modeFile(c, name);
    m.entries = loadModeResolved(m.file);
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

static bool validateSettings(const std::string& folder, const std::string& executable, std::string& error) {
    std::error_code ec;
    if (!folder.empty() && !fs::is_directory(P(folder), ec)) {
        error = "That folder does not exist.";
        return false;
    }
    if (!executable.empty() && !fs::is_regular_file(P(executable), ec)) {
        error = "The configured executable does not exist or is not a file.";
        return false;
    }
    return true;
}

static void cfgOk(Fl_Widget*, void* p) {
    CfgDlg* d = (CfgDlg*)p;
    std::string path = d->game->value();
    std::string executable = d->executable->value();
    std::string err;
    if (!validateSettings(path, executable, err)) {
        fl_alert("%s", err.c_str());
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

static bool createMode(const Category& c, const std::string& name, std::string& error) {
    if (!validName(name)) {
        error = "Invalid name (avoid  \\ / : * ? \" < > | =  and leading/trailing spaces).";
        return false;
    }
    fs::path f = modeFile(c, name);
    std::error_code ec;
    if (fs::exists(f, ec)) { error = "'" + name + "' already exists."; return false; }
    if (!saveMode(f, {})) { error = "Could not create the mode file."; return false; }
    return true;
}

static bool removeMode(const Category& c, const std::string& name, std::string& error) {
    std::error_code ec;
    fs::remove(modeFile(c, name), ec);
    if (ec) { error = ec.message(); return false; }
    return true;
}

static void onAddMode(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    const char* in = fl_input("New %s name:", "", c->singular);
    if (!in) return;
    std::string name = in, err;
    if (!createMode(*c, name, err)) { setStatus(err); return; }
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
    std::string err;
    bool ok = removeMode(*c, name, err);
    refresh(*c);
    setStatus(ok ? "Removed '" + name + "'." : "Error: " + err);
    saveConfig();
}

static void onEditMode(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    std::string name = selectedName(*c);
    if (name.empty()) { setStatus("Select a " + std::string(c->singular) + " to edit."); return; }
    editMode(*c, name);
}

static int runSynchronousCommand(const std::string& cmdLine, const fs::path& workDir, bool blocking = true) {
    if (cmdLine.empty()) return 0;
    if (!blocking) {
#ifdef _WIN32
        std::string command = "cmd.exe /C " + cmdLine;
        std::vector<char> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back('\0');
        STARTUPINFOA startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        BOOL ok = CreateProcessA(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE,
                                 CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW, nullptr,
                                 S(workDir).c_str(), &startup, &process);
        if (!ok) return -1;
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
#else
        pid_t pid = fork();
        if (pid < 0) return -1;
        if (pid == 0) {
            if (chdir(workDir.c_str()) != 0) _exit(126);
            execl("/bin/sh", "sh", "-c", cmdLine.c_str(), (char*)nullptr);
            _exit(127);
        }
        return 0;
#endif
    }
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

// ---- pre-launch commands -------------------------------------------------------------

using CommandRunner = std::function<int(const std::string&, const fs::path&, bool)>;

struct PreLaunchList {
    std::string mode, version;
    bool exists = false;
};

// Sleep-style commands are the only pre-launch barriers. Everything else is
// started asynchronously so it cannot delay the next command or the launch.
static bool isSleepCommand(const std::string& command) {
    std::string s = trimCopy(command);
    size_t end = s.find_first_of(" \t");
    std::string executable = s.substr(0, end);
#ifdef _WIN32
    std::transform(executable.begin(), executable.end(), executable.begin(),
                   [](unsigned char ch) { return (char)std::tolower(ch); });
    return executable == "sleep" || executable == "timeout";
#else
    return executable == "sleep";
#endif
}

// Pre-launch command lists of the selected modes: resolution first, then controller.
// Lists from different modes never override each other; all of them run.
static std::vector<PreLaunchList> selectedPreLaunchLists() {
    std::vector<PreLaunchList> out;
    for (auto& c : g_cats) {
        std::string name = selectedName(c);
        if (name.empty()) continue;
        for (auto& e : loadModeResolved(modeFile(c, name)))
            if (e.file == kPreLaunchRel)
                out.push_back({std::string(c.singular) + " '" + name + "'", e.version,
                               versionExists(e.file, e.version)});
    }
    return out;
}

// Empty when every selected pre-launch list exists on disk.
static std::string missingPreLaunchMessage() {
    for (auto& l : selectedPreLaunchLists())
        if (!l.exists)
            return "Pre-launch commands version '" + l.version + "' is missing (used by " + l.mode + ").";
    return "";
}

struct PreLaunchReport {
    struct Ran { std::string command; int code = 0; bool blocking = true; };
    std::vector<Ran> ran;        // every command that was executed, in order
    std::vector<std::string> lines;  // one confirmation line per command (also written to the log)
    int failed = 0;
};

static PreLaunchReport runPreLaunchCommands(const fs::path& workDir, const CommandRunner& run) {
    PreLaunchReport r;
    for (auto& l : selectedPreLaunchLists()) {
        if (!l.exists) continue;
        for (auto& raw : readLines(versionPath(kPreLaunchRel, l.version, false))) {
            CommandSpec spec = parseCommandSpec(raw);
            std::string cmd = trimCopy(spec.command);
            if (cmd.empty()) continue;
            bool barrier = isSleepCommand(cmd);
            int code = run(cmd, workDir, barrier);
            r.ran.push_back({cmd, code, barrier});
            if (code != 0) ++r.failed;
            std::string line = std::string("Pre-launch command ") +
                               (barrier ? (code == 0 ? "ran OK" : "FAILED") : (code == 0 ? "started" : "FAILED")) +
                               " (exit code " + std::to_string(code) + ", " + l.mode + " / '" + l.version +
                               "'): " + cmd;
            r.lines.push_back(line);
            debugLog(line);
        }
    }
    return r;
}

// Text for the status area confirming what ran; empty when no pre-launch command ran.
static std::string preLaunchSummary(const PreLaunchReport& r) {
    if (r.ran.empty()) return "";
    std::string s;
    if (r.failed == 0) {
        s = "Pre-launch: ran " + std::to_string(r.ran.size()) + " command(s) OK: ";
        for (size_t i = 0; i < r.ran.size(); ++i) s += (i ? "; " : "") + r.ran[i].command;
    } else {
        s = "Pre-launch: " + std::to_string(r.failed) + " of " + std::to_string(r.ran.size()) +
            " command(s) FAILED: ";
        bool first = true;
        for (auto& c : r.ran)
            if (c.code != 0) {
                s += (first ? "" : "; ") + c.command + " (exit code " + std::to_string(c.code) + ")";
                first = false;
            }
    }
    return s;
}

// The launch result is appended to (never replaces) the pre-launch confirmation.
static std::string composeLaunchStatus(const PreLaunchReport& r, const std::string& launchMsg) {
    std::string s = preLaunchSummary(r);
    return s.empty() ? launchMsg : s + "\n" + launchMsg;
}

static bool checkExecutable(const std::string& path, fs::path& out, std::string& error) {
    if (path.empty()) { error = "Configure the executable first in Settings."; return false; }
    std::error_code ec;
    out = fs::absolute(P(path), ec);
    if (ec || !fs::is_regular_file(out, ec)) { error = "Configured executable does not exist: " + path; return false; }
    return true;
}

static void onLaunch(Fl_Widget*, void*) {
    fs::path executable;
    std::string err;
    if (!checkExecutable(g_executablePath, executable, err)) { setStatus(err); return; }

    std::string missing = missingPreLaunchMessage();
    if (!missing.empty()) {
        setStatus(missing + " The game was not launched.");
        debugLog(missing);
        return;
    }

    PreLaunchReport report = runPreLaunchCommands(executable.parent_path(), runSynchronousCommand);

    std::string launchMsg;
#ifdef _WIN32
    HINSTANCE result = ShellExecuteW(nullptr, L"open", executable.wstring().c_str(),
                                     nullptr, executable.parent_path().wstring().c_str(), SW_SHOWNORMAL);
    launchMsg = ((INT_PTR)result <= 32) ? "Could not launch " + S(executable) + "."
                                        : "Launched " + S(executable) + ".";
#else
    pid_t pid = fork();
    if (pid < 0) {
        launchMsg = "Could not launch " + S(executable) + ".";
    } else {
        if (pid == 0) {
            if (chdir(executable.parent_path().c_str()) != 0) _exit(126);
            execl(executable.c_str(), executable.c_str(), (char*)nullptr);
            _exit(127);
        }
        launchMsg = "Launched " + S(executable) + ".";
    }
#endif
    setStatus(composeLaunchStatus(report, launchMsg));
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
        for (auto& e : loadModeResolved(modeFile(c, name))) {
            std::string rel = e.file;
            bool isCmds = (rel == kPreLaunchRel);
            if (!isCmds && (!cleanRel(rel) || !validName(e.version))) {
                setStatus("Invalid entry '" + e.file + "=" + e.version + "' in " + label + ". Nothing was changed.");
                return;
            }
            bool found = false;
            if (!isCmds)  // command lists of both modes all run; only files can be overridden
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
        if (!versionExists(it.rel, it.ver)) {
            if (it.rel == kPreLaunchRel)
                setStatus("Pre-launch commands version '" + it.ver + "' is missing (used by " + it.from +
                          "). Nothing was changed.");
            else
                setStatus("Version '" + it.ver + "' of " + it.rel + " is missing (used by " + it.from +
                          "). Nothing was changed.");
            return;
        }
    }

    for (auto& it : plan) {
        if (it.rel == kPreLaunchRel) continue;
        std::string err;
        if (!backupOriginal(it.rel, err)) {
            setStatus("Could not back up " + it.rel + ": " + err + ". Nothing was changed.");
            return;
        }
    }

    int n = 0, lists = 0;
    for (auto& it : plan) {
        if (it.rel == kPreLaunchRel) {
            ++lists;
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
    if (lists > 0)
        msg += " " + std::to_string(lists) + " pre-launch command list(s) will run when you launch the game.";
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
    Fl_Window* win = new Fl_Window(540, 540, "Game Launcher");
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

    g_status = new Fl_Box(10, 440, 520, 90);
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
