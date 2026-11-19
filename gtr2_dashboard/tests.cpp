// tests.cpp - automated tests for launcher.cpp.
//
// Every requirement in 000_REQUIREMENTS_GUI.md has an ID (REQ-AREA-NN). A test calls
// covers({...}) once its checks pass. At the end, testRequirementsTraceability() reads the
// requirements document and FAILS if
//   * a requirement row (not marked [manual]) was not covered by a test that ran, or
//   * a test claims an ID that the document does not define.
// So adding a requirement without a test, or a test for a nonexistent requirement, breaks
// `make test`.
//
// The tests build FLTK widgets without showing them; run them where a display is available
// (for headless machines: xvfb-run ./tests).

#define main launcher_program_main
#include "launcher.cpp"
#undef main

#include <cstdlib>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {

using Lines = std::vector<std::string>;

void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::set<std::string> g_covered;
void covers(std::initializer_list<const char*> ids) {
    for (const char* id : ids) g_covered.insert(id);
}

fs::path makeTempRoot() {
    fs::path root = fs::temp_directory_path() /
                    ("gtr2_dashboard_tests_" + std::to_string(std::rand()));
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root);
    return root;
}

std::string statusText() {
    return g_status && g_status->label() ? std::string(g_status->label()) : std::string();
}

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

// ------------------------------------------------------------------ test environment

void ensureWidgets() {
    for (auto& c : g_cats)
        if (!c.list) c.list = mkBrowser(0, 0, 1, 1);
    if (!g_status) g_status = new Fl_Box(0, 0, 1, 1);
    if (!g_gameLabel) g_gameLabel = new Fl_Box(0, 0, 1, 1);
}

void selectMode(Category& c, const std::string& name) {
    c.list->clear();
    if (!name.empty()) {
        c.list->add(name.c_str());
        c.list->value(1);
    }
}

// Fresh launcher data folder + game folder under `root`.
void resetEnv(const fs::path& root) {
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root);
    g_base = root;
    g_cfgFile = root / "launcher.cfg";
    g_modesDir = root / "modes";
    g_versionsDir = root / "versions";
    g_originalsDir = root / "originals";
    g_gameFolder = (root / "game").string();
    g_executablePath.clear();
    g_cats[0].dir = g_modesDir / "resolutions";
    g_cats[1].dir = g_modesDir / "controllers";
    fs::create_directories(g_cats[0].dir);
    fs::create_directories(g_cats[1].dir);
    fs::create_directories(g_versionsDir);
    fs::create_directories(g_originalsDir);
    fs::create_directories(gamePath() / "config");
    ensureWidgets();
    selectMode(g_cats[0], "");
    selectMode(g_cats[1], "");
    g_status->copy_label("");
}

void writeGame(const std::string& rel, const Lines& lines) {
    fs::create_directories((gamePath() / P(rel)).parent_path());
    check(writeLines(gamePath() / P(rel), lines), "could not write game fixture " + rel);
}

void writeVersion(const std::string& rel, const std::string& name, VerKind kind, const Lines& lines) {
    fs::path file = versionPath(rel, name, kind == VerKind::Patch);
    fs::create_directories(file.parent_path());
    check(writeLines(file, lines), "could not write version fixture " + name);
}

// ------------------------------------------------------------------ names, paths, text

void testNamesAndPaths() {
    check(validName("1080p"), "valid version/mode name rejected");
    check(!validName(""), "empty name accepted");
    check(!validName("bad/name"), "path separator accepted in name");
    check(!validName(" trailing"), "leading space accepted in name");
    check(!validName("trailing "), "trailing space accepted in name");
    check(!validName("trailing."), "trailing dot accepted in name");
    check(!validName("."), "'.' accepted as a name");
    check(!validName(".."), "'..' accepted as a name (path traversal)");
    for (char ch : std::string("\\/:*?\"<>|="))
        check(!validName(std::string("a") + ch + "b"), std::string("reserved character accepted: ") + ch);

    std::string rel = "config/../input.ini";
    check(cleanRel(rel) && rel == "input.ini", "relative path was not normalized");
    rel = "../outside.ini";
    check(!cleanRel(rel), "path escaping game folder accepted");
    rel = "a/../../outside.ini";
    check(!cleanRel(rel), "nested path escaping game folder accepted");
    rel = "/absolute.ini";
    check(!cleanRel(rel), "absolute path accepted");
    rel = "";
    check(!cleanRel(rel), "empty path accepted");
    covers({"REQ-MODE-05", "REQ-VER-01"});
}

void testTextAndPatchMerging(const fs::path& root) {
    resetEnv(root);
    fs::path file = root / "input.ini";
    check(writeLines(file, {"width=800", "height=600", "# comment"}), "writeLines failed");
    check(readLines(file) == Lines({"width=800", "height=600", "# comment"}),
          "readLines did not preserve file contents");
    check(splitLines("a\r\nb\n\nc\n") == Lines({"a", "b", "", "c"}), "splitLines mishandled CRLF/blank lines");
    check(splitLines("").empty(), "splitLines of empty text returned lines");
    check(splitLines(joinLines({"x", "y"})) == Lines({"x", "y"}), "join/split round trip failed");

    check(extractKey("  width = 1920") == "width", "key extraction failed");
    check(extractKey("# width=1920").empty(), "comment treated as a key");

    auto merged = applyPatch({"width=800", "height=600", "# comment"},
                             {"width=1920", "height=1080", "new_option=yes"});
    check(merged == Lines({"width=1920", "height=1080", "# comment", "new_option=yes"}),
          "patch replacement/insertion failed");
    covers({"REQ-VER-12"});
}

// ------------------------------------------------------------------ patch-line dialog

void testPatchLogic() {
    check(computeDiffIndices({"width=800", "height=600", "# comment"},
                             {"width=1920", "height=600", "# comment", "new_option=yes"}) ==
              std::vector<size_t>({0, 3}),
          "external-file differences were not identified by line");

    check(nextSelectedIndex({true, false, true, false}, -1, 1) == 0, "next did not start at the first selection");
    check(nextSelectedIndex({true, false, true, false}, 0, 1) == 2, "next skipped a selected line");
    check(nextSelectedIndex({true, false, true, false}, 2, 1) == 0, "next did not wrap around");
    check(nextSelectedIndex({true, false, true, false}, 0, -1) == 2, "previous did not wrap");
    check(nextSelectedIndex({false, false}, 0, 1) == -1, "navigation returned a line when none was selected");

    std::vector<bool> selection = {true, false, true};
    std::vector<std::vector<bool>> history = {{true, false, false}};
    check(undoSelection(selection, history) && selection == std::vector<bool>({true, false, false}),
          "undo did not restore the previous patch-line selection");
    check(!undoSelection(selection, history), "undo reported a change when history was empty");

    check(patchLineDisplay(0, "width=800", true) == "1: width=800", "line number/content not shown");
    std::string dimmed = patchLineDisplay(1, "height=600", false);
    check(contains(dimmed, "height=600") && contains(dimmed, "2: ") && dimmed != "2: height=600",
          "unselected patch lines were not styled as lighter text");
    check(patchLineDisplay(1, "height=600", true) == "2: height=600",
          "selected patch lines were incorrectly dimmed");
    check(contains(patchLineDisplay(1, "height=600", false, 1), std::string(1, '\x01') + "B"),
          "search result was not given a background highlight");

    check(findLineIgnoreCase({"Width=800", "height=600", "WIDTH_LIMIT=1"}, "width", 0) == 0,
          "search missed the first match");
    check(findLineIgnoreCase({"Width=800", "height=600", "WIDTH_LIMIT=1"}, "width", 1) == 2,
          "Find next did not continue after the previous match");
    check(findLineIgnoreCase({"Width=800", "height=600", "WIDTH_LIMIT=1"}, "width", 3) == 0,
          "Find next did not wrap");
    check(findLineIgnoreCase({"Width=800", "height=600"}, "HEIGHT", 0) == 1, "search is case sensitive");
    check(findLineIgnoreCase({"a=1"}, "zzz", 0) == -1, "search found a missing text");
    check(findLineIgnoreCase({"a=1"}, "", 0) == -1, "empty search matched");
    covers({"REQ-PATCH-01", "REQ-PATCH-03", "REQ-PATCH-05", "REQ-PATCH-06", "REQ-PATCH-09"});
}

void testPatchDialogState() {
    PatchLinesDlg d;
    d.fileLines = {"width=800", "height=600", "# comment"};
    Lines ext = {"width=1920", "height=600", "# comment", "new_option=yes"};
    initPatchDialogState(d, computeDiffLines(d.fileLines, ext), ext);
    check(d.currentSelection == std::vector<bool>({true, false, false}),
          "lines that differ in the external file were not selected by default");
    check(patchLineContent(&d, 0) == "width=1920", "selected line did not show the comparison-file value");
    check(patchLineContent(&d, 1) == "height=600", "unchanged line changed");

    PatchLinesDlg none;
    none.fileLines = d.fileLines;
    initPatchDialogState(none, {}, {});
    check(none.currentSelection == std::vector<bool>({false, false, false}),
          "lines were selected without any comparison file");

    PatchLinesDlg prior;
    prior.fileLines = d.fileLines;
    initPatchDialogState(prior, {"height=600"}, {});
    check(prior.currentSelection == std::vector<bool>({false, true, false}),
          "previously chosen patch lines were not preselected");

    PatchLinesDlg dlg;
    dlg.fileLines = {"width=800", "# original"};
    dlg.currentSelection = {true, false};
    dlg.comparisonByKey["width"] = "width=1920";
    dlg.comparisonByIndex = {"", ""};
    check(patchLineContent(&dlg, 0) == "width=1920", "selected lines did not display the comparison-file version");
    check(patchLineContent(&dlg, 1) == "# original", "unselected lines did not display the original-file version");
    covers({"REQ-PATCH-02"});
}

void testPatchOutput() {
    PatchLinesDlg d;
    d.fileLines = {"c=3", "a=1", "b=2", "# note"};
    d.currentSelection = {true, false, true, true};
    d.comparisonByIndex.assign(4, "");
    check(patchSelectionOutput(&d) == Lines({"c=3", "b=2", "# note"}), "selected line order was not preserved");
    d.comparisonByKey["b"] = "b=99";
    check(patchSelectionOutput(&d) == Lines({"c=3", "b=99", "# note"}),
          "patch did not use the comparison-file value for a selected line");
    d.currentSelection = {false, false, false, false};
    check(patchSelectionOutput(&d).empty(), "patch output was not empty with nothing selected");
    covers({"REQ-PATCH-07", "REQ-PATCH-08"});
}

void testPatchDialogWidgets() {
    ensureWidgets();
    PatchLinesDlg d;
    d.fileLines = {"a=1", "b=2", "c=3", "d=4"};
    d.currentSelection.assign(4, false);
    d.comparisonByIndex.assign(4, "");
    d.lines = new Fl_Multi_Browser(0, 0, 200, 100);
    d.lines->format_char('\x01');
    for (size_t i = 0; i < 4; ++i) d.lines->add(patchLineDisplay(i, d.fileLines[i], false).c_str());

    d.lines->select(2);
    d.lines->select(4);
    patchLinesSelectionChanged(nullptr, &d);
    check(d.currentSelection == std::vector<bool>({false, true, false, true}),
          "selecting lines did not update the patch selection");
    check(d.selectionHistory.size() == 1, "selection change was not recorded for undo");
    check(std::string(d.lines->text(2)) == "2: b=2", "selected line is not shown in normal text");
    check(std::string(d.lines->text(1)) != "1: a=1", "unselected line is not shown dimmed");

    d.lines->select(4, 0);
    patchLinesSelectionChanged(nullptr, &d);
    check(d.currentSelection == std::vector<bool>({false, true, false, false}),
          "deselecting a line did not update the patch selection");

    patchLinesUndo(nullptr, &d);
    check(d.currentSelection == std::vector<bool>({false, true, false, true}) && d.lines->selected(4),
          "Undo change did not restore the last selection");
    patchLinesUndo(nullptr, &d);
    check(d.currentSelection == std::vector<bool>({false, false, false, false}) && !d.lines->selected(2),
          "second Undo change did not restore the earlier selection");

    d.lines->select(2);
    d.lines->select(4);
    patchLinesSelectionChanged(nullptr, &d);
    check(d.lines->value() == 4, "test setup: focus is not on the last selected line");
    patchLinesNext(nullptr, &d);
    check(d.lines->value() == 2, "Next selected did not wrap to the first selected line");
    patchLinesNext(nullptr, &d);
    check(d.lines->value() == 4, "Next selected did not move to the next selected line");
    patchLinesPrevious(nullptr, &d);
    check(d.lines->value() == 2, "Previous selected did not move to the previous selected line");
    patchLinesPrevious(nullptr, &d);
    check(d.lines->value() == 4, "Previous selected did not wrap to the last selected line");

    delete d.lines;
    covers({"REQ-PATCH-04", "REQ-PATCH-05", "REQ-PATCH-06"});
}

// ------------------------------------------------------------------ version display parsing

void testVersionDisplayParsing() {
    check(parseVersionDisplay("xbox [patch]") == std::make_pair(std::string("xbox"), true),
          "patch display parsing failed");
    check(parseVersionDisplay("keyboard [full]") == std::make_pair(std::string("keyboard"), false),
          "full display suffix was not removed (version would be reported missing)");
    check(parseVersionDisplay("default [pre-launch]") == std::make_pair(std::string("default"), false),
          "pre-launch display suffix was not removed (version would be reported missing)");
    check(parseVersionDisplay("plain") == std::make_pair(std::string("plain"), false), "plain name changed");

    // Property: every kind round-trips, whatever the name length.
    for (const char* name : {"a", "default", "high perf", "x [y]", "1080p_ultra_wide_version"})
        for (VerKind kind : {VerKind::Full, VerKind::Patch, VerKind::Commands}) {
            ParsedVersion p = parseVersionDisplayEx(versionDisplay(name, kind));
            check(p.name == name && p.kind == kind && p.hadSuffix,
                  std::string("display round trip failed for '") + name + "'");
        }
    check(displayRel(kPreLaunchRel) == "Pre-launch Commands" && realRel("Pre-launch Commands") == kPreLaunchRel &&
              realRel("config/input.ini") == "config/input.ini",
          "pre-launch tracked-config name mapping failed");
    covers({"REQ-VER-05", "REQ-PRE-06"});
}

// ------------------------------------------------------------------ modes

void testModesAndEntries(const fs::path& root) {
    resetEnv(root);
    std::string err;
    check(createMode(g_cats[0], "1080p", err), err);
    check(fs::exists(modeFile(g_cats[0], "1080p")), "mode file was not created");
    check(!createMode(g_cats[0], "1080p", err) && contains(err, "already exists"), "duplicate mode accepted");
    check(!createMode(g_cats[0], "bad/name", err) && contains(err, "Invalid"), "invalid mode name accepted");
    check(!createMode(g_cats[0], "..", err), "path-traversal mode name accepted");
    check(!createMode(g_cats[0], "x.", err), "trailing-dot mode name accepted");
    check(createMode(g_cats[0], "720p", err), err);
    check(createMode(g_cats[1], "1080p", err), "same name must be allowed in the other category");
    check(scanModes(g_cats[0].dir) == Lines({"1080p", "720p"}), "scanModes failed or is not sorted");
    check(scanModes(g_cats[1].dir) == Lines({"1080p"}), "resolution and controller modes are not independent");
    covers({"REQ-MODE-01", "REQ-MODE-02", "REQ-MODE-05"});

    refresh(g_cats[0], "720p");
    check(g_cats[0].list->size() == 2 && selectedName(g_cats[0]) == "720p", "refresh did not select the mode");
    covers({"REQ-MODE-04"});

    check(removeMode(g_cats[0], "720p", err), err);
    check(scanModes(g_cats[0].dir) == Lines({"1080p"}), "mode was not removed");
    covers({"REQ-MODE-03"});

    // Mode = list of config -> version assignments.
    fs::path mode = modeFile(g_cats[0], "1920x1080");
    check(saveMode(mode, {{"config/input.ini", "keyboard"}, {"config/video.ini", "wide"}}), "saveMode failed");
    auto entries = loadMode(mode);
    check(entries.size() == 2 && entries[0].file == "config/input.ini" && entries[1].version == "wide",
          "loadMode failed");
    covers({"REQ-MODE-06"});

    // Editing assignments.
    std::vector<Entry> e = {{"config/input.ini", "keyboard"}};
    check(setEntry(e, {"config/video.ini", "wide"}) == 1 && e.size() == 2, "assignment was not added");
    check(setEntry(e, {"config/input.ini", "xbox"}) == 0 && e.size() == 2 && e[0].version == "xbox",
          "assignment was not replaced");
    check(!removeEntry(e, 5) && e.size() == 2, "out-of-range removal changed the list");
    check(removeEntry(e, 0) && e.size() == 1 && e[0].file == "config/video.ini", "assignment was not removed");
    Entry fromList = makeEntryFromDisplay("config/input.ini", "keyboard [full]");
    check(fromList.file == "config/input.ini" && fromList.version == "keyboard",
          "selecting a full version stored its display suffix");
    covers({"REQ-MODE-07"});

    // Missing versions are marked.
    writeVersion("config/input.ini", "keyboard", VerKind::Full, {"k=1"});
    check(!contains(modeEntryLabel({"config/input.ini", "keyboard"}), "[MISSING]"), "existing version marked missing");
    check(contains(modeEntryLabel({"config/input.ini", "gone"}), "[MISSING]"), "missing version not marked");
    check(contains(modeEntryLabel({kPreLaunchRel, "gone"}), "Pre-launch Commands") &&
              contains(modeEntryLabel({kPreLaunchRel, "gone"}), "[MISSING]"),
          "missing pre-launch version not marked");
    covers({"REQ-MODE-08"});
}

// ------------------------------------------------------------------ versions

void testCreateAndListVersions(const fs::path& root) {
    resetEnv(root);
    writeGame("config/input.ini", {"input=original", "keep=yes"});
    std::string err, rel;

    check(createVersion("config/../config/input.ini", "keyboard", VerKind::Full, {}, err, &rel), err);
    check(rel == "config/input.ini", "tracked path was not normalized");
    check(readLines(versionPath("config/input.ini", "keyboard", false)) == Lines({"input=original", "keep=yes"}),
          "full version is not a copy of the game file");
    check(createVersion("config/input.ini", "xbox", VerKind::Patch, {"input=xbox"}, err), err);
    check(readLines(versionPath("config/input.ini", "xbox", true)) == Lines({"input=xbox"}),
          "patch version contents wrong");
    check(createVersion("ignored", "default", VerKind::Commands, {"echo a", "   ", " echo b "}, err, &rel), err);
    check(rel == kPreLaunchRel, "command list was not stored under the pre-launch entry");
    check(readLines(versionPath(kPreLaunchRel, "default", false)) == Lines({"echo a", "echo b"}),
          "command list contents wrong (blank lines must be dropped, commands trimmed)");
    covers({"REQ-VER-02", "REQ-VER-03", "REQ-VER-04"});

    check(readLines(g_originalsDir / "config/input.ini") == Lines({"input=original", "keep=yes"}),
          "original was not backed up when the first version was created");
    check(!fs::exists(g_originalsDir / kPreLaunchRel), "command lists must not create an original backup");

    auto vm = scanVersions();
    check(vm["config/input.ini"] == Lines({"keyboard [full]", "xbox [patch]"}), "full/patch versions not listed");
    check(vm["Pre-launch Commands"] == Lines({"default [pre-launch]"}), "pre-launch versions not listed");
    covers({"REQ-VER-05"});

    // Names are unique per tracked config, whatever the kind.
    check(!createVersion("config/input.ini", "keyboard", VerKind::Full, {}, err) && contains(err, "already exists"),
          "duplicate full version accepted");
    check(!createVersion("config/input.ini", "keyboard", VerKind::Patch, {"a=1"}, err), "patch with a full version's name accepted");
    check(!createVersion("config/input.ini", "xbox", VerKind::Full, {}, err), "full with a patch version's name accepted");
    check(!createVersion("x", "default", VerKind::Commands, {"echo"}, err), "duplicate command list accepted");
    covers({"REQ-VER-07"});

    // Refusals must not leave files behind.
    check(!createVersion("config/input.ini", "bad/name", VerKind::Full, {}, err) && contains(err, "Invalid version name"),
          "invalid version name accepted");
    check(!createVersion("../escape.ini", "v", VerKind::Full, {}, err) && contains(err, "Invalid file path"),
          "path outside the game folder accepted");
    check(!createVersion("config/missing.ini", "v", VerKind::Full, {}, err) && contains(err, "not found"),
          "version of a nonexistent game file accepted");
    check(!createVersion("config/input.ini", "empty", VerKind::Patch, {}, err), "patch without lines accepted");
    check(!createVersion("x", "nocmd", VerKind::Commands, {"  ", ""}, err), "command list without commands accepted");
    check(!createVersion("x", "bad=name", VerKind::Commands, {"echo"}, err), "invalid command-list name accepted");
    check(!fs::exists(versionPath("config/input.ini", "empty", true)) &&
              !fs::exists(versionPath(kPreLaunchRel, "nocmd", false)),
          "a refused version left a file behind");
    std::string saved = g_gameFolder;
    g_gameFolder.clear();
    check(!createVersion("config/input.ini", "nogame", VerKind::Full, {}, err), "version created without a game folder");
    g_gameFolder = saved;
    covers({"REQ-VER-01", "REQ-VER-02", "REQ-VER-04"});
}

void testEditCloneDeleteVersions(const fs::path& root) {
    resetEnv(root);
    writeGame("config/input.ini", {"input=original"});
    std::string err;
    check(createVersion("config/input.ini", "keyboard", VerKind::Full, {}, err), err);
    check(createVersion("config/input.ini", "xbox", VerKind::Patch, {"input=xbox"}, err), err);
    check(createVersion("x", "default", VerKind::Commands, {"echo a"}, err), err);
    check(saveMode(modeFile(g_cats[1], "pad"), {{"config/input.ini", "xbox"}}), "mode fixture");
    check(saveMode(modeFile(g_cats[0], "hd"), {{kPreLaunchRel, "default"}}), "mode fixture");

    // ---- Edit version: changes the contents in place, never the name.
    check(saveEditedVersion("config/input.ini", "keyboard", VerKind::Full, {"input=edited", "x=1"}, err), err);
    check(readLines(versionPath("config/input.ini", "keyboard", false)) == Lines({"input=edited", "x=1"}),
          "edited full version was not saved");
    check(saveEditedVersion("config/input.ini", "xbox", VerKind::Patch, {"input=xbox2"}, err), err);
    check(readLines(versionPath("config/input.ini", "xbox", true)) == Lines({"input=xbox2"}), "edited patch not saved");
    check(saveEditedVersion(kPreLaunchRel, "default", VerKind::Commands, {"  ", " echo z "}, err), err);
    check(readLines(versionPath(kPreLaunchRel, "default", false)) == Lines({"echo z"}),
          "edited command list not saved (blank lines dropped, commands trimmed)");
    auto vm = scanVersions();
    check(vm["config/input.ini"] == Lines({"keyboard [full]", "xbox [patch]"}) &&
              vm["Pre-launch Commands"] == Lines({"default [pre-launch]"}),
          "editing created, renamed or left behind a version file");
    check(modeEntryLabel({"config/input.ini", "xbox"}).find("[MISSING]") == std::string::npos,
          "editing a version broke the modes that use it");

    check(!saveEditedVersion("config/input.ini", "xbox", VerKind::Patch, {}, err), "empty patch saved");
    check(readLines(versionPath("config/input.ini", "xbox", true)) == Lines({"input=xbox2"}),
          "refused edit still changed the patch");
    check(!saveEditedVersion(kPreLaunchRel, "default", VerKind::Commands, {" ", ""}, err), "empty command list saved");
    check(readLines(versionPath(kPreLaunchRel, "default", false)) == Lines({"echo z"}),
          "refused edit still changed the command list");
    check(!saveEditedVersion("config/input.ini", "nope", VerKind::Full, {"x"}, err) &&
              !fs::exists(versionPath("config/input.ini", "nope", false)),
          "editing a missing version created it");
    covers({"REQ-VER-08"});

    // ---- Clone version.
    check(cloneVersion("config/input.ini", "keyboard", VerKind::Full, "keyboard_copy", err), err);
    check(readLines(versionPath("config/input.ini", "keyboard_copy", false)) == Lines({"input=edited", "x=1"}),
          "cloned full version differs from the source");
    check(cloneVersion("config/input.ini", "xbox", VerKind::Patch, "xbox2", err), err);
    check(readLines(versionPath("config/input.ini", "xbox2", true)) == Lines({"input=xbox2"}), "cloned patch differs");
    check(cloneVersion(kPreLaunchRel, "default", VerKind::Commands, "default2", err), err);
    check(readLines(versionPath(kPreLaunchRel, "default2", false)) == Lines({"echo z"}), "cloned command list differs");
    vm = scanVersions();
    check(vm["config/input.ini"] == Lines({"keyboard [full]", "keyboard_copy [full]", "xbox [patch]", "xbox2 [patch]"}),
          "clone did not keep the kind of the source or the source version");
    check(vm["Pre-launch Commands"] == Lines({"default [pre-launch]", "default2 [pre-launch]"}),
          "cloned command list not listed");
    check(readLines(versionPath("config/input.ini", "keyboard", false)) == Lines({"input=edited", "x=1"}),
          "cloning modified the source");
    check(!cloneVersion("config/input.ini", "keyboard", VerKind::Full, "keyboard_copy", err) && contains(err, "already exists"),
          "clone over an existing version accepted");
    check(!cloneVersion("config/input.ini", "keyboard", VerKind::Full, "xbox", err),
          "clone with the name of a version of another kind accepted");
    check(!cloneVersion("config/input.ini", "keyboard", VerKind::Full, "bad/name", err) && contains(err, "Invalid"),
          "clone with an invalid name accepted");
    check(!cloneVersion("config/input.ini", "ghost", VerKind::Full, "ghost2", err) &&
              !fs::exists(versionPath("config/input.ini", "ghost2", false)),
          "clone of a missing version created a file");
    covers({"REQ-VER-09"});

    // ---- Delete version.
    check(modesUsing("config/input.ini", "xbox [patch]") == Lines({"controller pad"}),
          "modes using a version were not reported");
    check(modesUsing("config/input.ini", "xbox2 [patch]").empty(), "unused version reported as used");
    check(modesUsing(kPreLaunchRel, "default [pre-launch]") == Lines({"resolution hd"}),
          "modes using a pre-launch version were not reported");
    check(deleteVersion("config/input.ini", "xbox", VerKind::Patch), "delete failed");
    check(!fs::exists(versionPath("config/input.ini", "xbox", true)), "deleted version still on disk");
    check(!deleteVersion("config/input.ini", "xbox", VerKind::Patch), "deleting twice reported success");
    check(deleteVersion("config/input.ini", "keyboard", VerKind::Full) &&
              !fs::exists(versionPath("config/input.ini", "keyboard", false)),
          "a full version could not be deleted (display suffix not stripped?)");
    check(contains(modeEntryLabel({"config/input.ini", "xbox"}), "[MISSING]"),
          "mode entry of a deleted version was not flagged missing");
    check(deleteVersion(kPreLaunchRel, "default", VerKind::Commands) && deleteVersion(kPreLaunchRel, "default2", VerKind::Commands),
          "pre-launch versions could not be deleted");
    check(!fs::exists(g_versionsDir / kPreLaunchRel), "empty version folder was not cleaned up");
    covers({"REQ-VER-06"});
}

// ------------------------------------------------------------------ originals, apply, restore

void testOriginalBackup(const fs::path& root) {
    resetEnv(root);
    fs::path gameFile = gamePath() / "config/input.ini";
    check(writeLines(gameFile, {"original=yes"}), "could not create game fixture");

    std::string error;
    check(backupOriginal("config/input.ini", error), "backupOriginal failed: " + error);
    check(readLines(g_originalsDir / "config/input.ini") == Lines({"original=yes"}), "original backup contents incorrect");

    check(writeLines(gameFile, {"modified=yes"}), "could not modify game fixture");
    check(backupOriginal("config/input.ini", error), "second backupOriginal failed");
    check(readLines(g_originalsDir / "config/input.ini") == Lines({"original=yes"}), "original backup was overwritten");
    covers({"REQ-VER-10"});
}

void testApplyWorkflow(const fs::path& root) {
    resetEnv(root);
    writeGame("config/input.ini", {"input=original", "keep=yes"});
    writeGame("config/video.ini", {"video=original"});
    writeVersion("config/input.ini", "resolution", VerKind::Full, {"input=resolution"});
    writeVersion("config/input.ini", "controller", VerKind::Patch, {"input=controller"});
    writeVersion("config/video.ini", "wide", VerKind::Full, {"video=wide"});
    check(saveMode(modeFile(g_cats[0], "1080p"), {{"config/input.ini", "resolution"}, {"config/video.ini", "wide"}}),
          "could not save resolution fixture");
    check(saveMode(modeFile(g_cats[1], "xbox"), {{"config/input.ini", "controller"}}),
          "could not save controller fixture");
    selectMode(g_cats[0], "1080p");
    selectMode(g_cats[1], "xbox");

    onApply(nullptr, nullptr);

    check(readLines(gamePath() / "config/input.ini") == Lines({"input=controller", "keep=yes"}),
          "controller mode did not take precedence for a conflicting file");
    check(readLines(gamePath() / "config/video.ini") == Lines({"video=wide"}), "full version was not applied");
    check(readLines(g_originalsDir / "config/input.ini") == Lines({"input=original", "keep=yes"}),
          "input original was not preserved");
    check(readLines(g_originalsDir / "config/video.ini") == Lines({"video=original"}),
          "video original was not preserved");
    check(contains(statusText(), "config/input.ini") && contains(statusText(), "controller"),
          "status did not report the controller override: " + statusText());
    covers({"REQ-VER-11", "REQ-MODE-09", "REQ-VER-10"});
}

void testPatchAppliesToImmutableOriginal(const fs::path& root) {
    resetEnv(root);
    writeGame("config/input.ini", {"a=1", "b=2"});
    writeVersion("config/input.ini", "p1", VerKind::Patch, {"a=10"});
    writeVersion("config/input.ini", "p2", VerKind::Patch, {"c=3"});
    check(saveMode(modeFile(g_cats[0], "m1"), {{"config/input.ini", "p1"}}), "fixture");
    check(saveMode(modeFile(g_cats[0], "m2"), {{"config/input.ini", "p2"}}), "fixture");

    selectMode(g_cats[0], "m1");
    onApply(nullptr, nullptr);
    check(readLines(gamePath() / "config/input.ini") == Lines({"a=10", "b=2"}), "patch p1 not merged");
    selectMode(g_cats[0], "m2");
    onApply(nullptr, nullptr);
    check(readLines(gamePath() / "config/input.ini") == Lines({"a=1", "b=2", "c=3"}),
          "patch was merged into the previously patched file instead of the original");
    check(readLines(g_originalsDir / "config/input.ini") == Lines({"a=1", "b=2"}), "original was modified");
    covers({"REQ-VER-12", "REQ-VER-10"});
}

void testApplyRefusals(const fs::path& root) {
    resetEnv(root);
    writeGame("config/input.ini", {"input=original"});
    writeGame("config/video.ini", {"video=original"});
    writeVersion("config/input.ini", "ok", VerKind::Full, {"input=new"});

    onApply(nullptr, nullptr);
    check(contains(statusText(), "Select a resolution"), "Apply with no mode selected gave no message: " + statusText());
    check(!hasModeSelection(), "hasModeSelection true with nothing selected");

    check(saveMode(modeFile(g_cats[0], "m"), {}), "fixture");
    selectMode(g_cats[0], "m");
    check(hasModeSelection(), "hasModeSelection false with a mode selected");
    onApply(nullptr, nullptr);
    check(contains(statusText(), "no config entries"), "empty mode gave no message: " + statusText());

    // One good entry + one missing version: nothing may be modified.
    check(saveMode(modeFile(g_cats[0], "m"), {{"config/input.ini", "ok"}, {"config/video.ini", "gone"}}), "fixture");
    onApply(nullptr, nullptr);
    check(contains(statusText(), "missing") && contains(statusText(), "Nothing was changed"),
          "missing version not reported: " + statusText());
    check(readLines(gamePath() / "config/input.ini") == Lines({"input=original"}),
          "a file was modified although a selected version was missing");

    // Invalid entry (path traversal): nothing may be modified.
    check(saveMode(modeFile(g_cats[0], "m"), {{"config/input.ini", "ok"}, {"../evil.ini", "ok"}}), "fixture");
    onApply(nullptr, nullptr);
    check(contains(statusText(), "Invalid entry") && readLines(gamePath() / "config/input.ini") == Lines({"input=original"}),
          "invalid entry was not refused: " + statusText());

    std::string saved = g_gameFolder;
    g_gameFolder = (root / "does-not-exist").string();
    onApply(nullptr, nullptr);
    check(contains(statusText(), "Game folder"), "Apply without a valid game folder gave no message");
    g_gameFolder = saved;
    covers({"REQ-VER-13", "REQ-MAIN-06", "REQ-MAIN-07"});
}

void testRestoreOriginals(const fs::path& root) {
    resetEnv(root);
    check(listOriginals().empty(), "originals listed in an empty folder");
    writeGame("config/input.ini", {"input=original"});
    writeGame("video.ini", {"video=original"});
    std::string err;
    check(backupOriginal("config/input.ini", err) && backupOriginal("video.ini", err), err);
    writeGame("config/input.ini", {"changed"});
    writeGame("video.ini", {"changed"});
    fs::remove(gamePath() / "video.ini");

    auto rels = listOriginals();
    check(rels.size() == 2, "originals were not listed");
    int restored = 0;
    check(restoreOriginals(rels, restored, err), err);
    check(restored == 2, "restore count wrong");
    check(readLines(gamePath() / "config/input.ini") == Lines({"input=original"}) &&
              readLines(gamePath() / "video.ini") == Lines({"video=original"}),
          "originals were not restored");
    covers({"REQ-VER-14"});
}

// ------------------------------------------------------------------ settings, config, launch

void testSettingsAndConfig(const fs::path& root) {
    resetEnv(root);
    std::string err;
    check(validateSettings("", "", err), "empty settings must be allowed");
    check(validateSettings(g_gameFolder, "", err), err);
    check(!validateSettings((root / "nope").string(), "", err) && contains(err, "folder"), "missing game folder accepted");
    check(!validateSettings("", (root / "nope.exe").string(), err) && contains(err, "executable"),
          "missing executable accepted");
    check(!validateSettings("", root.string(), err), "a folder was accepted as the executable");
    fs::path exe = root / "game.sh";
    check(writeLines(exe, {"#!/bin/sh"}), "fixture");
    check(validateSettings(g_gameFolder, exe.string(), err), err);
    covers({"REQ-MAIN-03"});

    g_gameFolder.clear();
    updateGameLabel();
    check(contains(g_gameLabel->label(), "not set"), "unset game folder not shown");
    g_gameFolder = "/tmp/some game";
    updateGameLabel();
    check(contains(g_gameLabel->label(), "/tmp/some game"), "game folder not shown");
    covers({"REQ-MAIN-02"});

    // Persistence: settings + selected modes survive a save/load.
    g_gameFolder = "/tmp/game";
    g_executablePath = "/tmp/game/game";
    check(saveMode(modeFile(g_cats[0], "1080p"), {}) && saveMode(modeFile(g_cats[1], "xbox"), {}), "fixture");
    refresh(g_cats[0], "1080p");
    refresh(g_cats[1], "xbox");
    saveConfig();
    auto config = loadConfig();
    check(config["game_path"] == "/tmp/game", "game path config was not loaded");
    check(config["executable_path"] == "/tmp/game/game", "executable config was not loaded");
    check(config["resolutions"] == "1080p" && config["controllers"] == "xbox", "selected modes were not persisted");
    check(fs::exists(g_cfgFile) && g_cfgFile.parent_path() == g_base, "settings are not stored beside the launcher");
    covers({"REQ-MAIN-05"});

    // Launch precheck.
    fs::path out;
    check(!checkExecutable("", out, err) && contains(err, "Settings"), "empty executable accepted");
    check(!checkExecutable((root / "missing").string(), out, err) && contains(err, "does not exist"),
          "missing executable accepted");
    check(checkExecutable(exe.string(), out, err) && out.filename() == "game.sh", err);
    covers({"REQ-MAIN-08"});
}

// ------------------------------------------------------------------ GUI structure

void testScreensHaveButtons() {
    ensureWidgets();
    VerMgr m;
    buildVersionsWindow(m);
    check(m.addBtn && std::string(m.addBtn->label()) == "Add version...", "Add version button missing");
    check(m.editBtn && std::string(m.editBtn->label()) == "Edit version", "Config Versions screen has no 'Edit version' button");
    check(m.cloneBtn && std::string(m.cloneBtn->label()) == "Clone version", "Config Versions screen has no 'Clone version' button");
    check(m.deleteBtn && std::string(m.deleteBtn->label()) == "Delete version", "Delete version button missing");
    check(m.editBtn->callback() == vmEditCb && m.cloneBtn->callback() == vmCloneCb &&
              m.deleteBtn->callback() == vmDeleteCb && m.addBtn->callback() == vmAddCb,
          "a Config Versions button is not wired to its handler");
    check(m.editBtn->visible() && m.cloneBtn->visible(), "Edit/Clone buttons are hidden");
    delete m.win;
    covers({"REQ-VER-15"});

    AddFormDlg f;
    buildAddVersionWindow(f, "");
    check(f.editCmdBtn && std::string(f.editCmdBtn->label()) == "Edit command",
          "Add Version screen has no 'Edit command' button");
    check(f.editCmdBtn->callback() == afEditCmdCb, "'Edit command' button is not wired");
    check(!f.editCmdBtn->visible() && !f.addCmdBtn->visible() && !f.removeCmdBtn->visible(),
          "command buttons visible for a config-file version");
    f.typeChoice->value(1);
    afToggleConfigType(nullptr, &f);
    check(f.editCmdBtn->visible() && f.addCmdBtn->visible() && f.removeCmdBtn->visible() && static_cast<Fl_Widget*>(f.cmdsBrowser)->visible(),
          "command buttons (incl. Edit command) not shown for pre-launch commands");
    f.typeChoice->value(0);
    afToggleConfigType(nullptr, &f);
    check(!f.editCmdBtn->visible() && !static_cast<Fl_Widget*>(f.cmdsBrowser)->visible(), "command buttons not hidden for config files");
    delete f.win_;
    covers({"REQ-PRE-01"});
}

void testAddVersionCommandList() {
    ensureWidgets();
    AddFormDlg f;
    buildAddVersionWindow(f, "");
    std::string err;
    check(afAppendCommand(&f, "  echo one ", err), err);
    check(afAppendCommand(&f, "echo two", err), err);
    check(!afAppendCommand(&f, "   ", err), "blank command added");
    check(collectCommands(f.cmdsBrowser) == Lines({"echo one", "echo two"}), "commands not collected in order, trimmed");

    f.cmdsBrowser->select(2);
    check(selectedRow(f.cmdsBrowser) == 2, "selected command row not found");
    check(afReplaceCommand(&f, 2, "  echo changed ", err), err);
    check(collectCommands(f.cmdsBrowser) == Lines({"echo one", "echo changed"}), "Edit command did not replace the command");
    check(!afReplaceCommand(&f, 2, "  ", err) && contains(err, "empty"), "command edited to nothing");
    check(!afReplaceCommand(&f, 0, "x", err) && !afReplaceCommand(&f, 9, "x", err), "edit of a nonexistent row accepted");
    check(collectCommands(f.cmdsBrowser) == Lines({"echo one", "echo changed"}), "refused edit changed the list");
    f.cmdsBrowser->remove(1);
    check(collectCommands(f.cmdsBrowser) == Lines({"echo changed"}), "command not removed");
    f.cmdsBrowser->select(1);
    f.blockingCb->value(0);
    afBlockingChangedCb(nullptr, &f);
    CommandSpec configured = parseCommandSpec(collectCommandLines(f.cmdsBrowser)[0]);
    check(configured.command == "echo changed" && !configured.blocking,
          "non-blocking command setting was not saved");
    delete f.win_;
    covers({"REQ-PRE-01", "REQ-PRE-08", "REQ-PRE-09"});
}

// ------------------------------------------------------------------ pre-launch commands

struct Call { std::string cmd; fs::path dir; bool blocking = true; };

void testPreLaunchConfiguredIsNeverMissing(const fs::path& root) {
    resetEnv(root);
    writeGame("config/video.ini", {"video=original"});
    std::string err;
    check(createVersion("x", "default", VerKind::Commands, {"echo one", "echo two"}, err), err);
    check(createVersion("config/video.ini", "wide", VerKind::Full, {}, err), err);
    check(saveEditedVersion("config/video.ini", "wide", VerKind::Full, {"video=wide"}, err), err);

    // Exactly what the mode editor does when the user picks the rows in the lists.
    auto vm = scanVersions();
    check(vm["Pre-launch Commands"].size() == 1, "pre-launch version not listed");
    Entry pre = makeEntryFromDisplay("Pre-launch Commands", vm["Pre-launch Commands"][0]);
    check(pre.file == kPreLaunchRel && pre.version == "default", "mode editor stored the wrong pre-launch entry: " + pre.version);
    Entry vid = makeEntryFromDisplay("config/video.ini", vm["config/video.ini"][0]);
    check(vid.version == "wide", "mode editor stored the wrong full-version entry: " + vid.version);
    std::vector<Entry> entries;
    setEntry(entries, pre);
    setEntry(entries, vid);
    check(saveMode(modeFile(g_cats[0], "hd"), entries), "fixture");
    selectMode(g_cats[0], "hd");

    check(!contains(modeEntryLabel(entries[0]), "MISSING"), "pre-launch entry shown as [MISSING] in the mode editor");
    check(missingPreLaunchMessage().empty(), "configured pre-launch version reported missing at launch");
    onApply(nullptr, nullptr);
    check(!contains(statusText(), "missing") && contains(statusText(), "Applied"),
          "Apply reported a configured pre-launch version as missing: " + statusText());
    check(contains(statusText(), "pre-launch command list"), "Apply did not mention the pre-launch list: " + statusText());
    check(readLines(gamePath() / "config/video.ini") == Lines({"video=wide"}), "Apply skipped the file entries");

    // Mode files written by older builds contain the display suffix; they must still work.
    fs::path legacy = modeFile(g_cats[0], "legacy");
    std::ofstream(legacy) << "__pre_launch_commands__=default [pre-launch]\nconfig/video.ini=wide [full]\n";
    auto healed = loadModeResolved(legacy);
    check(healed.size() == 2 && healed[0].version == "default" && healed[1].version == "wide",
          "legacy entries with display suffixes were not healed");
    selectMode(g_cats[0], "legacy");
    check(missingPreLaunchMessage().empty(), "legacy pre-launch entry reported missing at launch");
    onApply(nullptr, nullptr);
    check(!contains(statusText(), "missing"), "legacy entries reported missing by Apply: " + statusText());

    // A genuinely absent list is still reported.
    check(deleteVersion(kPreLaunchRel, "default", VerKind::Commands), "fixture");
    check(contains(missingPreLaunchMessage(), "default") && contains(missingPreLaunchMessage(), "resolution 'legacy'"),
          "absent pre-launch version not reported");
    onApply(nullptr, nullptr);
    check(contains(statusText(), "missing") && contains(statusText(), "Nothing was changed"),
          "Apply did not refuse a missing pre-launch list: " + statusText());
    covers({"REQ-PRE-06", "REQ-MODE-10", "REQ-PRE-05"});
}

void testPreLaunchRunAndConfirm(const fs::path& root) {
    resetEnv(root);
    std::string err;
    check(createVersion("x", "first", VerKind::Commands, {"echo one", "  ", "echo two"}, err), err);
    check(createVersion("x", "second", VerKind::Commands, {"echo three"}, err), err);
    check(saveMode(modeFile(g_cats[0], "hd"), {{kPreLaunchRel, "first"}}), "fixture");
    check(saveMode(modeFile(g_cats[1], "pad"), {{kPreLaunchRel, "second"}}), "fixture");

    // Nothing selected -> nothing runs, nothing is claimed.
    std::vector<Call> calls;
    CommandRunner recorder = [&](const std::string& c, const fs::path& d, bool blocking) {
        calls.push_back({c, d, blocking});
        return 0;
    };
    auto none = runPreLaunchCommands(root, recorder);
    check(calls.empty() && none.ran.empty() && preLaunchSummary(none).empty(), "commands ran with no mode selected");

    // Both modes: every list runs, resolution first, then controller; blank lines skipped.
    selectMode(g_cats[0], "hd");
    selectMode(g_cats[1], "pad");
    fs::path work = root / "game";
    auto report = runPreLaunchCommands(work, recorder);
    check(calls.size() == 3 && calls[0].cmd == "echo one" && calls[1].cmd == "echo two" && calls[2].cmd == "echo three",
          "pre-launch commands did not run sequentially in order (resolution list, then controller list)");
    check(calls[0].dir == work && calls[2].dir == work &&
              !calls[0].blocking && !calls[2].blocking,
          "commands were not run from the given working directory or started asynchronously");
    check(report.failed == 0 && report.ran.size() == 3, "report counts wrong");
    covers({"REQ-PRE-02", "REQ-PRE-07"});

    // Confirmation shown to the user and written to the log.
    check(report.lines.size() == 3 && contains(report.lines[0], "started") && contains(report.lines[0], "echo one") &&
              contains(report.lines[0], "resolution 'hd'") && contains(report.lines[2], "controller 'pad'"),
          "per-command confirmation lines are missing details");
    std::string summary = preLaunchSummary(report);
    check(contains(summary, "ran 3 command(s) OK") && contains(summary, "echo one") && contains(summary, "echo three"),
          "status summary does not confirm the commands that ran: " + summary);
    std::string status = composeLaunchStatus(report, "Launched /g/game.");
    check(contains(status, "ran 3 command(s) OK") && contains(status, "Launched /g/game."),
          "launch status lost the pre-launch confirmation or the launch message");
    std::string log;
    {
        std::ifstream in(g_base / "launcher.log");
        log.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    check(contains(log, "echo one") && contains(log, "echo three") && contains(log, "started"),
          "pre-launch confirmation was not written to launcher.log");
    check(composeLaunchStatus(none, "Launched.") == "Launched.", "launch status changed when no command ran");
    covers({"REQ-PRE-03"});

    // A failure is reported and survives the launch message (it used to be overwritten).
    CommandRunner failing = [&](const std::string& c, const fs::path&, bool) { return c == "echo two" ? 3 : 0; };
    auto bad = runPreLaunchCommands(work, failing);
    check(bad.failed == 1 && bad.ran.size() == 3, "a failing command stopped the others or was not counted");
    check(contains(bad.lines[1], "FAILED") && contains(bad.lines[1], "exit code 3"), "failure line lacks the exit code");
    std::string failStatus = composeLaunchStatus(bad, "Launched /g/game.");
    check(contains(failStatus, "FAILED") && contains(failStatus, "echo two") && contains(failStatus, "exit code 3") &&
              contains(failStatus, "Launched /g/game."),
          "failure message was lost or overwritten by the launch message: " + failStatus);
    covers({"REQ-PRE-04"});

    // A missing list is never silently skipped by the runner's precheck.
    check(deleteVersion(kPreLaunchRel, "second", VerKind::Commands), "fixture");
    check(contains(missingPreLaunchMessage(), "second") && contains(missingPreLaunchMessage(), "controller 'pad'"),
          "missing pre-launch list not reported before launch");
    calls.clear();
    runPreLaunchCommands(work, recorder);
    check(calls.size() == 2, "available lists must still be runnable by the runner");
    covers({"REQ-PRE-05"});

    check(saveEditedVersion(kPreLaunchRel, "first", VerKind::Commands,
                            {"@blocking\t echo one", "@nonblocking\t echo two", "sleep 0"}, err), err);
    calls.clear();
    selectMode(g_cats[1], "");
    auto mixed = runPreLaunchCommands(work, recorder);
    check(mixed.ran.size() == 3 && calls.size() == 3 &&
              !calls[0].blocking && !calls[1].blocking && calls[2].blocking,
          "ordinary commands were not asynchronous or sleep was not a barrier");
    check(contains(mixed.lines[0], "started") && contains(mixed.lines[1], "started") &&
              contains(mixed.lines[2], "ran OK"),
          "command status did not distinguish asynchronous commands from sleep barriers");
    covers({"REQ-PRE-09"});

    // Both modes pointing at pre-launch lists is not a file override.
    check(createVersion("x", "second", VerKind::Commands, {"echo three"}, err), err);
    selectMode(g_cats[1], "pad");
    onApply(nullptr, nullptr);
    check(!contains(statusText(), "set by both modes") && contains(statusText(), "2 pre-launch command list(s)"),
          "pre-launch lists were treated as overriding each other: " + statusText());
    covers({"REQ-PRE-07"});
}

void testRealCommandRunner(const fs::path& root) {
    resetEnv(root);
    check(runSynchronousCommand("exit 3", root) == 3, "exit code of a command was not returned");
    check(runSynchronousCommand("exit 0", root) == 0, "successful command reported failure");
    check(runSynchronousCommand("echo hi> marker.txt", root) == 0 && fs::exists(root / "marker.txt"),
          "command did not run inside the requested working directory");
    covers({"REQ-PRE-02"});
}

// ------------------------------------------------------------------ requirements traceability

fs::path findRequirementsDoc() {
    std::error_code ec;
    std::vector<fs::path> dirs = {fs::path(__FILE__).parent_path(), fs::current_path(ec), fs::current_path(ec) / "..",
                                  g_base};
    for (auto& d : dirs) {
        fs::path candidate = d / "000_REQUIREMENTS_GUI.md";
        if (fs::is_regular_file(candidate, ec)) return candidate;
    }
    return {};
}

void testRequirementsTraceability() {
    fs::path doc = findRequirementsDoc();
    check(!doc.empty(), "000_REQUIREMENTS_GUI.md not found; run the tests from the project folder");
    std::ifstream in(doc);
    std::string line;
    std::set<std::string> required, manual;
    std::regex row(R"(^\|\s*(REQ-[A-Z]+-[0-9]+)\s*\|)");
    while (std::getline(in, line)) {
        std::smatch m;
        if (!std::regex_search(line, m, row)) continue;
        (line.find("[manual]") != std::string::npos ? manual : required).insert(m[1]);
    }
    check(required.size() > 40, "requirements document has too few requirement rows");

    std::string missing, unknown;
    for (auto& id : required)
        if (!g_covered.count(id)) missing += " " + id;
    for (auto& id : g_covered)
        if (!required.count(id) && !manual.count(id)) unknown += " " + id;
    check(missing.empty(), "requirements without an executed test (add a test or mark the row [manual]):" + missing);
    check(unknown.empty(), "tests claim requirement IDs that 000_REQUIREMENTS_GUI.md does not define:" + unknown);
}

} // namespace

int main() {
    std::ostringstream logSink;       // debugLog() also writes to std::clog; keep test output clean
    struct ClogRestore {
        std::streambuf* old;
        ~ClogRestore() { std::clog.rdbuf(old); }
    } restoreClog{std::clog.rdbuf(logSink.rdbuf())};
    fs::path root = makeTempRoot();
    int rc = 0;
    try {
        testNamesAndPaths();
        testTextAndPatchMerging(root / "t1");
        testPatchLogic();
        testPatchDialogState();
        testPatchOutput();
        testPatchDialogWidgets();
        testVersionDisplayParsing();
        testModesAndEntries(root / "t2");
        testCreateAndListVersions(root / "t3");
        testEditCloneDeleteVersions(root / "t4");
        testOriginalBackup(root / "t5");
        testApplyWorkflow(root / "t6");
        testPatchAppliesToImmutableOriginal(root / "t7");
        testApplyRefusals(root / "t8");
        testRestoreOriginals(root / "t9");
        testSettingsAndConfig(root / "t10");
        testScreensHaveButtons();
        testAddVersionCommandList();
        testPreLaunchConfiguredIsNeverMissing(root / "t11");
        testPreLaunchRunAndConfirm(root / "t12");
        testRealCommandRunner(root / "t13");
        testRequirementsTraceability();
        std::cout << "All launcher tests passed (" << g_covered.size() << " requirements covered).\n";
    } catch (const std::exception& e) {
        std::cerr << "Launcher test failure: " << e.what() << "\n";
        rc = 1;
    }
    std::error_code ec;
    fs::remove_all(root, ec);
    return rc;
}
