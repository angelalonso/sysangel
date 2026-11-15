sudo apt install g++ make cmake curl mingw-w64 libfltk1.3-dev
make          # native Linux binary
make win      # gamelauncher.exe
make test     # core and filesystem workflow tests (needs a display; use xvfb-run make test on a headless machine)

## Implemented features

- Cross-platform FLTK GUI for Linux and Windows.
- Configuration of the game folder and executable to launch.
- Add, remove, and edit resolution and controller modes.
- Assign tracked game files to named full-file versions.
- Save selected files as line-based patches and apply them to the original file.
- Add, edit, clone and delete named versions from the Config Versions screen.
- Pre-launch command lists: add, edit and remove commands; the commands run before the game
  starts and the status area confirms what ran (and any failure). Details are also in `launcher.log`.
- Store versions, modes, configuration, and logs beside the launcher.
- Back up each original game file once in `originals/` before changing it.
- Apply selected resolution and controller modes; controller entries take precedence
  when both modes configure the same file.
- Restore all backed-up originals.
- Launch the configured executable.

Behavior is specified in `000_REQUIREMENTS_GUI.md`, one row per requirement ID. Each automated
test declares the requirement IDs it verifies, and `make test` fails if a requirement has no
executed test (or a test names an unknown requirement). GUI dialogs, native file choosers and
starting the real executable are marked `[manual]` and still need a smoke test on each target
platform.
