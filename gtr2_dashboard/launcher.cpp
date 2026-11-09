// launcher.cpp - tiny preconfiguration frontend for a game.
//
// Concepts
//   * Tracked file : a file inside the game folder, identified by its path
//                    relative to that folder (e.g. "config/input.ini").
//   * Version      : a named snapshot of a tracked file, stored by us.
//   * Mode         : a resolution or a controller option. A mode is a list of
//                    (tracked file -> version) entries.
//   * Apply        : for the selected resolution mode and controller mode,
//                    copy the chosen version of each file into the game folder.
//   * Originals    : the first time we ever touch a game file, the pristine
//                    copy is saved in originals/ and never overwritten.
//
// Layout on disk (next to the executable)
//   launcher.cfg                       game path + last selections
//   versions/<relpath>/<version>       one file per saved version
//   modes/resolutions/<name>.mode      lines of  <relpath>=<version>
//   modes/controllers/<name>.mode
//   originals/<relpath>                disaster-recovery copies
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
#include <cstring>
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

// ================================================================ helpers ===

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

// Names of modes and versions become file names, and '=' is our separator.
static bool validName(const std::string& n) {
    if (n.empty() || n == "." || n == "..") return false;
    if (n.front() == ' ' || n.back() == ' ' || n.back() == '.') return false;
    return n.find_first_of("\\/:*?\"<>|=") == std::string::npos;
}

// Normalises a path relative to the game folder; rejects absolute paths and "..".
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
    b->format_char(0);  // plain text: '@' has no special meaning
    return b;
}

// =================================================================== state ===

struct Category {
    const char* title;     // shown above the list
    const char* subdir;    // folder under modes/, also config key
    const char* singular;  // used in dialogs
    fs::path dir;
    Fl_Hold_Browser* list = nullptr;
};

// Order matters: later categories win when two modes set the same file.
static Category g_cats[2] = {
    {"Resolution", "resolutions", "resolution", {}},
    {"Controller", "controllers", "controller", {}},
};

static fs::path g_base, g_cfgFile, g_modesDir, g_versionsDir, g_originalsDir;
static std::string g_gameFolder;
static Fl_Box* g_gameLabel = nullptr;
static Fl_Box* g_status = nullptr;

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
                    (g_gameFolder.empty() ? std::string("(not set - open Configuration)") : g_gameFolder);
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
}

// ======================================================== versions (disk) ===

using VersionMap = std::map<std::string, std::vector<std::string>>;  // relpath -> versions

static fs::path versionPath(const std::string& rel, const std::string& ver) {
    return g_versionsDir / P(rel) / P(ver);
}

// Every regular file under versions/ is a version; its parent folder
// (relative to versions/) is the tracked file's path.
static VersionMap scanVersions() {
    VersionMap m;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(g_versionsDir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code e2;
        if (!it->is_regular_file(e2)) continue;
        fs::path rel = it->path().parent_path().lexically_relative(g_versionsDir);
        if (rel.empty() || rel == fs::path(".")) continue;
        m[rel.generic_u8string()].push_back(S(it->path().filename()));
    }
    for (auto& kv : m) std::sort(kv.second.begin(), kv.second.end());
    return m;
}

static std::vector<std::string> modesUsing(const std::string& rel, const std::string& ver) {
    std::vector<std::string> out;
    for (auto& c : g_cats)
        for (auto& n : scanModes(c.dir))
            for (auto& e : loadMode(modeFile(c, n)))
                if (e.file == rel && e.version == ver) {
                    out.push_back(std::string(c.singular) + " " + n);
                    break;
                }
    return out;
}

// Path of a file chosen by the user, relative to the game folder.
// (On Windows the comparison is case-sensitive; pick files via the Browse button.)
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

// Saves the game's current copy of <rel> in originals/ unless a backup already
// exists. The first backup is the pristine one, so it is never overwritten.
static bool backupOriginal(const std::string& rel, std::string& err) {
    std::error_code ec;
    fs::path dst = g_originalsDir / P(rel);
    if (fs::exists(dst, ec)) return true;
    fs::path src = gamePath() / P(rel);
    if (!fs::is_regular_file(src, ec)) return true;  // file does not exist yet: nothing to save
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

struct AddForm {
    Fl_Window* win = nullptr;
    Fl_Input *file = nullptr, *name = nullptr, *src = nullptr;
    bool ok = false;
    std::string rel, ver;
};

static void afBrowseFile(Fl_Widget*, void* p) {
    AddForm* f = (AddForm*)p;
    if (!gameOk()) { fl_alert("%s", "Set the game folder first (Configuration)."); return; }
    Fl_Native_File_Chooser fc;
    fc.title("Select a file inside the game folder");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fc.directory(g_gameFolder.c_str());
    if (fc.show() != 0) return;
    std::string rel;
    if (!relFromGame(P(fc.filename()), rel))
        fl_alert("%s", "That file is not inside the game folder.");
    else
        f->file->value(rel.c_str());
}

static void afBrowseSrc(Fl_Widget*, void* p) {
    AddForm* f = (AddForm*)p;
    Fl_Native_File_Chooser fc;
    fc.title("Select the file to import as this version");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (fc.show() == 0) f->src->value(fc.filename());
}

static void afOk(Fl_Widget*, void* p) {
    AddForm* f = (AddForm*)p;
    if (!gameOk()) { fl_alert("%s", "Set the game folder first (Configuration)."); return; }

    std::string rel = f->file->value(), ver = f->name->value(), src = f->src->value();
    if (!cleanRel(rel)) {
        fl_alert("%s", "Invalid file path.\nUse a path relative to the game folder, e.g. config/input.ini");
        return;
    }
    if (!validName(ver)) {
        fl_alert("%s", "Invalid version name.\nAvoid  \\ / : * ? \" < > | =  and leading/trailing spaces.");
        return;
    }
    fs::path from = src.empty() ? gamePath() / P(rel) : P(src);
    std::error_code ec;
    if (!fs::is_regular_file(from, ec)) {
        fl_alert("Source file not found:\n%s", S(from).c_str());
        return;
    }
    fs::path dst = versionPath(rel, ver);
    if (fs::exists(dst, ec)) {
        fl_alert("Version '%s' of that file already exists.", ver.c_str());
        return;
    }
    std::string err;
    if (!backupOriginal(rel, err)) {
        fl_alert("Could not back up the original:\n%s", err.c_str());
        return;
    }
    fs::create_directories(dst.parent_path(), ec);
    if (!ec) fs::copy_file(from, dst, fs::copy_options::none, ec);
    if (ec) {
        fl_alert("Could not save the version:\n%s", ec.message().c_str());
        return;
    }
    f->ok = true;
    f->rel = rel;
    f->ver = ver;
    f->win->hide();
}

static void afCancel(Fl_Widget*, void* p) { ((AddForm*)p)->win->hide(); }

static bool addVersionForm(const std::string& prefillRel, std::string& outRel, std::string& outVer) {
    AddForm f;
    f.win = new Fl_Window(560, 235, "Add version");
    mkLabel(10, 8, 540, "File inside the game folder (e.g. config/input.ini):");
    f.file = new Fl_Input(10, 30, 440, 25);
    (new Fl_Button(460, 30, 90, 25, "Browse..."))->callback(afBrowseFile, &f);
    mkLabel(10, 65, 540, "Version name (e.g. 1080p, xbox):");
    f.name = new Fl_Input(10, 87, 540, 25);
    mkLabel(10, 122, 540, "Content from file (empty = the file as it is now in the game folder):");
    f.src = new Fl_Input(10, 144, 440, 25);
    (new Fl_Button(460, 144, 90, 25, "Browse..."))->callback(afBrowseSrc, &f);
    (new Fl_Button(340, 192, 130, 30, "Save version"))->callback(afOk, &f);
    (new Fl_Button(480, 192, 70, 30, "Cancel"))->callback(afCancel, &f);
    f.win->end();
    f.file->value(prefillRel.c_str());

    runModal(f.win);
    delete f.win;
    if (f.ok) { outRel = f.rel; outVer = f.ver; }
    return f.ok;
}

// ================================================ dialog: file versions =====

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
    if (addVersionForm(pre, rel, ver)) {
        vmReload(m, rel, ver);
        setStatus("Saved version '" + ver + "' of " + rel + ".");
    }
}

static void vmDeleteCb(Fl_Widget*, void* p) {
    VerMgr* m = (VerMgr*)p;
    int fi = m->filesB->value(), vi = m->versB->value();
    if (!fi || !vi) { fl_alert("%s", "Select a file and one of its versions first."); return; }
    std::string rel = m->filesB->text(fi), ver = m->versB->text(vi);

    std::string used;
    for (auto& u : modesUsing(rel, ver)) used += "\n  - " + u;
    std::string warn = used.empty() ? "" : "\n\nIt is used by:" + used + "\nThose entries will be flagged as missing.";
    if (fl_choice("Delete version '%s' of %s?%s", "Cancel", "Delete", nullptr, ver.c_str(), rel.c_str(),
                  warn.c_str()) != 1)
        return;

    std::error_code ec;
    fs::remove(versionPath(rel, ver), ec);
    // tidy up empty folders left behind
    fs::path dir = versionPath(rel, ver).parent_path();
    while (dir != g_versionsDir && fs::is_empty(dir, ec) && !ec) {
        fs::path parent = dir.parent_path();
        fs::remove(dir, ec);
        dir = parent;
    }
    vmReload(m, rel);
}

static void vmCloseCb(Fl_Widget*, void* p) { ((VerMgr*)p)->win->hide(); }

static void onVersions(Fl_Widget*, void*) {
    VerMgr m;
    m.win = new Fl_Window(620, 390, "File versions");
    mkLabel(10, 10, 300, "Tracked files", true);
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
    runModal(m.win);
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
        bool ok = fs::is_regular_file(versionPath(e.file, e.version), ec);
        std::string t = e.file + "   ->   " + e.version + (ok ? "" : "   [MISSING]");
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
    m->filesB->value(0);
    selectByText(m->filesB, e.file);
    meFilesCb(nullptr, m);
    selectByText(m->versB, e.version);
}

static void meSetCb(Fl_Widget*, void* p) {
    ModeEditor* m = (ModeEditor*)p;
    int fi = m->filesB->value(), vi = m->versB->value();
    if (!fi || !vi) { fl_alert("%s", "Select a file and a version first."); return; }
    Entry ne{m->filesB->text(fi), m->versB->text(vi)};
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

    m.win = new Fl_Window(620, 475, "Edit mode");
    std::string title = std::string(c.title) + ": " + name;
    mkLabel(10, 8, 600, "", true)->copy_label(title.c_str());

    mkLabel(10, 34, 600, "Files this mode changes   (file -> version):");
    m.entriesB = mkBrowser(10, 56, 600, 130);
    m.entriesB->callback(meEntryCb, &m);
    (new Fl_Button(10, 191, 130, 26, "Remove entry"))->callback(meRemoveCb, &m);

    mkLabel(10, 228, 300, "Tracked files");
    mkLabel(320, 228, 290, "Versions");
    m.filesB = mkBrowser(10, 249, 300, 140);
    m.filesB->callback(meFilesCb, &m);
    m.versB = mkBrowser(320, 249, 290, 140);
    (new Fl_Button(10, 395, 200, 28, "Set file -> version"))->callback(meSetCb, &m);
    (new Fl_Button(510, 435, 100, 30, "Close"))->callback(meCloseCb, &m);
    m.win->end();

    for (auto& kv : m.vm) m.filesB->add(kv.first.c_str());
    meRefreshEntries(&m);
    runModal(m.win);
    delete m.win;
}

// ======================================================== dialog: config ====

struct CfgDlg {
    Fl_Window* win = nullptr;
    Fl_Input* game = nullptr;
};

static void cfgBrowse(Fl_Widget*, void* p) {
    CfgDlg* d = (CfgDlg*)p;
    Fl_Native_File_Chooser fc;
    fc.title("Select the game's installation folder");
    fc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    if (d->game->value()[0]) fc.directory(d->game->value());
    if (fc.show() == 0) d->game->value(fc.filename());
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
    g_gameFolder = path;
    updateGameLabel();
    saveConfig();
    d->win->hide();
}

static void cfgCancel(Fl_Widget*, void* p) { ((CfgDlg*)p)->win->hide(); }

static void onConfig(Fl_Widget*, void*) {
    CfgDlg d;
    d.win = new Fl_Window(520, 130, "Configuration");
    mkLabel(10, 10, 500, "Game folder:");
    d.game = new Fl_Input(10, 32, 400, 25);
    d.game->value(g_gameFolder.c_str());
    (new Fl_Button(420, 32, 90, 25, "Browse..."))->callback(cfgBrowse, &d);
    (new Fl_Button(10, 85, 140, 30, "Open data folder"))->callback(cfgOpenData);
    (new Fl_Button(350, 85, 80, 30, "OK"))->callback(cfgOk, &d);
    (new Fl_Button(440, 85, 70, 30, "Cancel"))->callback(cfgCancel, &d);
    d.win->end();
    runModal(d.win);
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
    editMode(*c, name);  // let the user define its files right away
}

static void onRemoveMode(Fl_Widget*, void* p) {
    Category* c = (Category*)p;
    std::string name = selectedName(*c);
    if (name.empty()) { setStatus("Select a " + std::string(c->singular) + " to remove."); return; }
    if (fl_choice("Delete %s '%s'?\n(Saved file versions are not affected.)", "Cancel", "Delete", nullptr,
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

static void onApply(Fl_Widget*, void*) {
    if (!gameOk()) { setStatus("Game folder is not set or does not exist. Open Configuration."); return; }

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
            if (!cleanRel(rel) || !validName(e.version)) {
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
    if (plan.empty()) { setStatus("The selected modes have no file entries. Use Edit... to add some."); return; }

    // Pre-flight: every version must exist before anything is touched.
    std::error_code ec;
    for (auto& it : plan)
        if (!fs::is_regular_file(versionPath(it.rel, it.ver), ec)) {
            setStatus("Version '" + it.ver + "' of " + it.rel + " is missing (used by " + it.from +
                      "). Nothing was changed.");
            return;
        }
    // Disaster recovery: make sure the originals are saved before overwriting.
    for (auto& it : plan) {
        std::string err;
        if (!backupOriginal(it.rel, err)) {
            setStatus("Could not back up " + it.rel + ": " + err + ". Nothing was changed.");
            return;
        }
    }
    // Copy.
    int n = 0;
    for (auto& it : plan) {
        fs::path dst = gamePath() / P(it.rel);
        fs::create_directories(dst.parent_path(), ec);
        if (!ec) fs::copy_file(versionPath(it.rel, it.ver), dst, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            setStatus("Error writing " + it.rel + ": " + ec.message() +
                      ". The game may be partly updated; originals are safe, use Restore originals.");
            return;
        }
        ++n;
    }
    saveConfig();
    std::string msg = "Applied " + std::to_string(n) + " file(s) for " + modes + ".";
    if (!overridden.empty())
        msg += " Note: " + overridden[0] + (overridden.size() > 1 ? " (and others)" : "") +
               " was set by both modes; the later one (controller) won.";
    setStatus(msg);
}

// ====================================================================== GUI ==

static void buildCategory(Category& c, int x, int y, int w, int h) {
    mkLabel(x, y, w, c.title, true);
    c.list = mkBrowser(x, y + 22, w, h);
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

    Fl::scheme("gtk+");
    Fl_Window* win = new Fl_Window(540, 430, "Game launcher");

    (new Fl_Button(10, 10, 140, 28, "Configuration..."))->callback(onConfig);
    (new Fl_Button(160, 10, 140, 28, "File versions..."))->callback(onVersions);

    g_gameLabel = mkLabel(10, 45, 520, "");

    buildCategory(g_cats[0], 10, 75, 255, 190);
    buildCategory(g_cats[1], 275, 75, 255, 190);

    Fl_Button* apply = new Fl_Button(10, 330, 360, 35, "Apply");
    apply->labelfont(FL_BOLD);
    apply->callback(onApply);
    (new Fl_Button(380, 330, 150, 35, "Restore originals"))->callback(onRestore);

    g_status = new Fl_Box(10, 375, 520, 45);
    g_status->box(FL_DOWN_BOX);
    g_status->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    win->end();

    updateGameLabel();
    for (auto& c : g_cats) refresh(c, cfg[c.subdir]);
    if (g_gameFolder.empty()) setStatus("First run: open Configuration and set the game folder.");

    win->show(argc, argv);
    int rc = Fl::run();
    saveConfig();
    return rc;
}
