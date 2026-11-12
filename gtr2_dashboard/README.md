sudo apt install g++ make cmake curl mingw-w64 libfltk1.3-dev
make          # native Linux binary
make win      # gamelauncher.exe
make test     # core and filesystem workflow tests

## Implemented features

- Cross-platform FLTK GUI for Linux and Windows.
- Configuration of the game folder and executable to launch.
- Add, remove, and edit resolution and controller modes.
- Assign tracked game files to named full-file versions.
- Save selected files as line-based patches and apply them to the original file.
- Store versions, modes, configuration, and logs beside the launcher.
- Back up each original game file once in `originals/` before changing it.
- Apply selected resolution and controller modes; controller entries take precedence
  when both modes configure the same file.
- Restore all backed-up originals.
- Launch the configured executable.

The automated tests cover path/name validation, configuration and mode parsing,
version discovery, patch merging, immutable original backups, full-file application,
patch application, and mode conflict precedence. GUI dialogs and native file chooser
interactions still require manual smoke testing on each target platform.
