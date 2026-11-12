#define main launcher_program_main
#include "launcher.cpp"
#undef main

#include <cstdlib>
#include <stdexcept>

namespace {

void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

fs::path makeTempRoot() {
    fs::path root = fs::temp_directory_path() /
                    ("gtr2_dashboard_tests_" + std::to_string(std::rand()));
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root);
    return root;
}

void testNamesAndPaths() {
    check(validName("1080p"), "valid version/mode name rejected");
    check(!validName(""), "empty name accepted");
    check(!validName("bad/name"), "path separator accepted in name");
    check(!validName(" trailing"), "leading space accepted in name");
    check(!validName("trailing."), "trailing dot accepted in name");

    std::string rel = "config/../input.ini";
    check(cleanRel(rel) && rel == "input.ini", "relative path was not normalized");
    rel = "../outside.ini";
    check(!cleanRel(rel), "path escaping game folder accepted");
    rel = "/absolute.ini";
    check(!cleanRel(rel), "absolute path accepted");
}

void testTextAndPatches(const fs::path& root) {
    fs::path file = root / "input.ini";
    check(writeLines(file, {"width=800", "height=600", "# comment"}), "writeLines failed");
    check(readLines(file) == std::vector<std::string>({"width=800", "height=600", "# comment"}),
          "readLines did not preserve file contents");

    check(extractKey("  width = 1920") == "width", "key extraction failed");
    check(extractKey("# width=1920").empty(), "comment treated as a key");

    auto merged = applyPatch({"width=800", "height=600", "# comment"},
                             {"width=1920", "height=1080", "new_option=yes"});
    check(merged == std::vector<std::string>({"width=1920", "height=1080", "# comment", "new_option=yes"}),
          "patch replacement/insertion failed");
}

void testModesAndVersions(const fs::path& root) {
    g_modesDir = root / "modes";
    g_versionsDir = root / "versions";
    g_originalsDir = root / "originals";
    g_cats[0].dir = g_modesDir / "resolutions";
    g_cats[1].dir = g_modesDir / "controllers";

    std::error_code ec;
    fs::create_directories(g_cats[0].dir);
    fs::create_directories(g_cats[1].dir);
    fs::create_directories(g_versionsDir / "config");

    fs::path mode = modeFile(g_cats[0], "1920x1080");
    check(saveMode(mode, {{"config/input.ini", "keyboard"}, {"config/video.ini", "wide"}}),
          "saveMode failed");
    auto entries = loadMode(mode);
    check(entries.size() == 2 && entries[0].file == "config/input.ini" &&
              entries[1].version == "wide", "loadMode failed");
    check(scanModes(g_cats[0].dir) == std::vector<std::string>({"1920x1080"}),
          "scanModes failed");

    fs::path full = versionPath("config/input.ini", "keyboard", false);
    fs::path patch = versionPath("config/input.ini", "xbox", true);
    fs::create_directories(full.parent_path());
    check(writeLines(full, {"keyboard=yes"}), "could not create full version fixture");
    check(writeLines(patch, {"keyboard=no"}), "could not create patch version fixture");

    auto versions = scanVersions();
    check(versions["config/input.ini"].size() == 2, "scanVersions missed a version");
    check(parseVersionDisplay("xbox [patch]") == std::make_pair(std::string("xbox"), true),
          "patch display parsing failed");
    check(parseVersionDisplay("keyboard [full]") == std::make_pair(std::string("keyboard [full]"), false),
          "full display parsing unexpectedly changed");

    check(modesUsing("config/input.ini", "keyboard [full]") ==
              std::vector<std::string>({"resolution 1920x1080"}),
          "modesUsing did not find a referenced version");
}

void testOriginalBackup(const fs::path& root) {
    g_gameFolder = (root / "game").string();
    fs::create_directories(gamePath() / "config");
    fs::create_directories(g_originalsDir);
    fs::path gameFile = gamePath() / "config/input.ini";
    check(writeLines(gameFile, {"original=yes"}), "could not create game fixture");

    std::string error;
    check(backupOriginal("config/input.ini", error), "backupOriginal failed: " + error);
    check(readLines(g_originalsDir / "config/input.ini") == std::vector<std::string>({"original=yes"}),
          "original backup contents incorrect");

    check(writeLines(gameFile, {"modified=yes"}), "could not modify game fixture");
    check(backupOriginal("config/input.ini", error), "second backupOriginal failed");
    check(readLines(g_originalsDir / "config/input.ini") == std::vector<std::string>({"original=yes"}),
          "original backup was overwritten");
}

void testConfig(const fs::path& root) {
    g_cfgFile = root / "launcher.cfg";
    std::ofstream out(g_cfgFile);
    out << "game_path=/tmp/game\n";
    out << "executable_path=/tmp/game/game\n";
    out << "resolutions=1920x1080\n";
    out.close();

    auto config = loadConfig();
    check(config["game_path"] == "/tmp/game", "game path config was not loaded");
    check(config["executable_path"] == "/tmp/game/game", "executable config was not loaded");
    check(config["resolutions"] == "1920x1080", "selected mode config was not loaded");
}

void testApplyWorkflow(const fs::path& root) {
    std::error_code ec;
    fs::remove_all(root, ec);
    g_base = root;
    g_cfgFile = root / "launcher.cfg";
    g_modesDir = root / "modes";
    g_versionsDir = root / "versions";
    g_originalsDir = root / "originals";
    g_gameFolder = (root / "game").string();
    g_executablePath.clear();

    fs::create_directories(g_cats[0].dir = g_modesDir / "resolutions");
    fs::create_directories(g_cats[1].dir = g_modesDir / "controllers");
    fs::create_directories(g_versionsDir / "config");
    fs::create_directories(gamePath() / "config");
    fs::create_directories(g_originalsDir);

    check(writeLines(gamePath() / "config/input.ini", {"input=original", "keep=yes"}),
          "could not create input fixture");
    check(writeLines(gamePath() / "config/video.ini", {"video=original"}),
          "could not create video fixture");
    fs::create_directories(versionPath("config/input.ini", "resolution", false).parent_path());
    fs::create_directories(versionPath("config/video.ini", "wide", false).parent_path());
    check(writeLines(versionPath("config/input.ini", "resolution", false), {"input=resolution"}),
          "could not create resolution version");
    check(writeLines(versionPath("config/input.ini", "controller", true), {"input=controller"}),
          "could not create controller patch");
    check(writeLines(versionPath("config/video.ini", "wide", false), {"video=wide"}),
          "could not create video version");

    check(saveMode(modeFile(g_cats[0], "1080p"),
                   {{"config/input.ini", "resolution"}, {"config/video.ini", "wide"}}),
          "could not save resolution fixture");
    check(saveMode(modeFile(g_cats[1], "xbox"), {{"config/input.ini", "controller"}}),
          "could not save controller fixture");

    g_cats[0].list = new Fl_Hold_Browser(0, 0, 1, 1);
    g_cats[1].list = new Fl_Hold_Browser(0, 0, 1, 1);
    g_cats[0].list->add("1080p");
    g_cats[1].list->add("xbox");
    g_cats[0].list->value(1);
    g_cats[1].list->value(1);
    g_status = new Fl_Box(0, 0, 1, 1);

    onApply(nullptr, nullptr);

    auto appliedInput = readLines(gamePath() / "config/input.ini");
    check(appliedInput == std::vector<std::string>({"input=controller", "keep=yes"}),
          "controller mode did not take precedence for a conflicting file: " +
              (appliedInput.empty() ? std::string("<empty>") : appliedInput.front()));
    check(readLines(gamePath() / "config/video.ini") ==
              std::vector<std::string>({"video=wide"}),
          "full version was not applied");
    check(readLines(g_originalsDir / "config/input.ini") ==
              std::vector<std::string>({"input=original", "keep=yes"}),
          "input original was not preserved");
    check(readLines(g_originalsDir / "config/video.ini") ==
              std::vector<std::string>({"video=original"}),
          "video original was not preserved");

    delete g_cats[0].list;
    delete g_cats[1].list;
    delete g_status;
    g_cats[0].list = nullptr;
    g_cats[1].list = nullptr;
    g_status = nullptr;
}

} // namespace

int main() {
    fs::path root = makeTempRoot();
    try {
        testNamesAndPaths();
        testTextAndPatches(root);
        testModesAndVersions(root);
        testOriginalBackup(root);
        testConfig(root);
        testApplyWorkflow(root);
        std::error_code ec;
        fs::remove_all(root, ec);
        std::cout << "All launcher tests passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::error_code ec;
        fs::remove_all(root, ec);
        std::cerr << "Launcher test failure: " << e.what() << "\n";
        return 1;
    }
}
