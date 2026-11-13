# GUI Requirements

This document is the behavioral requirements and regression-test checklist for
the GTR2 Dashboard launcher.

## Main launcher

- Provide a cross-platform FLTK desktop GUI for Linux and Windows.
- Show the configured game folder and provide Settings for changing it.
- Let the user configure the executable that is launched.
- Provide native folder/file choosers for the game folder, executable, tracked
  config files, and external patch source files.
- Persist settings, selected modes, versions, modes, originals, and logs beside
  the launcher.
- Show clear status messages when an operation succeeds, is invalid, or fails.
- Enable Apply when at least one mode is selected.
- Launch the configured executable from its parent directory.

## Modes

- Support independently named Resolution and Controller modes.
- Add, remove, select, and edit modes.
- Validate mode names and reject path traversal, separators, reserved
  characters, leading/trailing spaces, and trailing dots.
- Edit a mode as a list of tracked config-to-version assignments.
- Add, replace, and remove assignments in a mode.
- Mark missing versions in the mode editor.
- When both selected modes configure the same file, the Controller assignment
  takes precedence and the status reports the override.

## Configuration versions

- Track files by clean paths relative to the game folder.
- Add named full-file versions by copying the current game file.
- Add named line-based patch versions.
- Add pre-launch command-list versions and execute their commands sequentially
  before launch.
- List full, patch, and pre-launch versions by tracked config.
- Delete versions and report modes that reference the deleted version.
- Save the first touched copy of every original file in `originals/` and never
  overwrite that backup.
- Apply full versions by replacement and patch versions by merging keyed and
  unkeyed lines into the immutable original.
- Refuse to modify files when a selected version is missing or invalid.
- Restore every saved original file after confirmation.

## Patch-line selection

- The **Select patch lines** dialog must show line numbers and file contents.
- When creating a patch from an external file, lines that differ from the
  current game file must be selected by default.
- Lines that are not selected must use lighter-grey text than selected lines.
- Selecting and deselecting lines must update the patch selection.
- Provide **Previous selected** and **Next selected** buttons that move focus
  to the previous/next selected line and wrap around the file.
- Provide **Undo change** to reverse the most recent selection change.
- Require at least one selected line before saving a patch.
- Preserve the selected line order when writing the patch.
- Support case-insensitive search and a Find next action.

## Validation and tests

The automated `make test` suite must cover each non-platform-specific behavior
above, including path/name validation, configuration and mode persistence,
version discovery, immutable backups, full-file and patch application,
controller precedence, external-file difference detection, default changed-line
selection, dimmed unselected-line styling, selected-line navigation, and
selection undo state. Native dialogs, rendering, and executable launching still
require manual smoke testing on Linux and Windows.
