# GUI Requirements

This document is the behavioral requirements and regression-test checklist for the
GTR2 Dashboard launcher.

## How this document is enforced

- Every requirement is one table row with a stable ID (`REQ-AREA-NN`).
- `tests.cpp` calls `covers({"REQ-..."})` at the end of each test that verifies a
  requirement. After all tests have run, `testRequirementsTraceability()` reads this file
  and **fails `make test`** if
  - a row that is not marked `[manual]` was not covered by a test that executed, or
  - a test claims an ID that is not defined here.
- Adding a requirement therefore means adding a test (or explicitly marking the row
  `[manual]`). Never reuse or renumber an ID; retire a requirement by deleting the row and
  its `covers` entries together.
- `[manual]` rows need a smoke test on Linux and Windows before a release: native dialogs,
  rendering, and starting the real executable cannot run in the automated suite.
- The suite builds FLTK widgets without showing them, so run it with a display
  (`xvfb-run make test` on a headless machine).

## Main launcher

| ID | Requirement | Verified by |
| --- | --- | --- |
| REQ-MAIN-01 | Provide a cross-platform FLTK desktop GUI for Linux and Windows. `[manual]` | smoke test |
| REQ-MAIN-02 | Show the configured game folder (or a "not set" hint) on the main window. | testSettingsAndConfig |
| REQ-MAIN-03 | Settings lets the user change the game folder and the launched executable; a folder or executable that does not exist (or is the wrong kind) is rejected, empty values are allowed. | testSettingsAndConfig |
| REQ-MAIN-04 | Provide native folder/file choosers for the game folder, executable, tracked config files and external patch source files. `[manual]` | smoke test |
| REQ-MAIN-05 | Persist settings, selected modes, versions, modes, originals and logs beside the launcher (not in the game folder). | testSettingsAndConfig |
| REQ-MAIN-06 | Show a clear status message when an operation succeeds, is invalid, or fails (including no game folder, nothing selected, empty mode). | testApplyRefusals |
| REQ-MAIN-07 | Apply is highlighted when at least one mode is selected; pressing Apply with nothing selected changes nothing and says what to do. | testApplyRefusals |
| REQ-MAIN-08 | Launch refuses with a clear message when no executable is configured or the configured file does not exist. | testSettingsAndConfig |
| REQ-MAIN-09 | Launch the configured executable from its parent directory. `[manual]` | smoke test |

## Modes

| ID | Requirement | Verified by |
| --- | --- | --- |
| REQ-MODE-01 | Resolution and Controller modes are independent categories; the same name may exist in both. | testModesAndEntries |
| REQ-MODE-02 | Add a mode; a duplicate name is refused. | testModesAndEntries |
| REQ-MODE-03 | Remove a mode (saved config versions are not affected). | testModesAndEntries |
| REQ-MODE-04 | Select a mode from the sorted list; the selection is remembered. | testModesAndEntries, testSettingsAndConfig |
| REQ-MODE-05 | Validate names: reject empty, `.`/`..`, path separators, reserved characters (`\ / : * ? " < > \| =`), leading/trailing spaces and trailing dots. | testNamesAndPaths, testModesAndEntries |
| REQ-MODE-06 | A mode is a list of tracked-config-to-version assignments that is saved and loaded unchanged. | testModesAndEntries |
| REQ-MODE-07 | In the mode editor, assignments can be added, replaced (one per tracked config) and removed; picking a version from the list stores its clean name (no `[full]`, `[patch]` or `[pre-launch]` suffix). | testModesAndEntries, testPreLaunchConfiguredIsNeverMissing |
| REQ-MODE-08 | An assignment whose version does not exist is marked `[MISSING]` in the mode editor; existing ones are not. | testModesAndEntries |
| REQ-MODE-09 | When both selected modes configure the same file, the Controller assignment takes precedence and the status reports the override. | testApplyWorkflow |
| REQ-MODE-10 | Mode files written by older builds that contain a display suffix (e.g. `default [pre-launch]`) still resolve to the existing version instead of being reported missing. | testPreLaunchConfiguredIsNeverMissing |

## Configuration versions

| ID | Requirement | Verified by |
| --- | --- | --- |
| REQ-VER-01 | Track files by clean paths relative to the game folder; absolute paths, `..` escapes and nonexistent game files are refused. | testNamesAndPaths, testCreateAndListVersions |
| REQ-VER-02 | Add a named full-file version by copying the current game file. | testCreateAndListVersions |
| REQ-VER-03 | Add a named line-based patch version (at least one line required). | testCreateAndListVersions |
| REQ-VER-04 | Add a named pre-launch command-list version (blank lines dropped, commands trimmed, at least one command required). | testCreateAndListVersions |
| REQ-VER-05 | List full, patch and pre-launch versions per tracked config (pre-launch lists under "Pre-launch Commands"); the kind suffix shown in the list is removed correctly for every kind. | testCreateAndListVersions, testVersionDisplayParsing |
| REQ-VER-06 | Delete a version (full, patch or pre-launch), report the modes that reference it, flag those entries as missing afterwards and remove empty version folders. | testEditCloneDeleteVersions |
| REQ-VER-07 | Version names are unique per tracked config regardless of kind; a refused version leaves no files behind. | testCreateAndListVersions |
| REQ-VER-08 | **Edit version** (Config Versions screen): edit the contents of the selected full, patch or pre-launch version in place. The name never changes, modes that use it keep working, an empty patch or command list is refused, a missing version is never created, and a refused edit leaves the version untouched. | testEditCloneDeleteVersions |
| REQ-VER-09 | **Clone version** (Config Versions screen): copy the selected version to a new valid, unused name for the same tracked config, keeping its kind and leaving the source unchanged; invalid or existing names and missing sources are refused. | testEditCloneDeleteVersions |
| REQ-VER-10 | Save the first touched copy of every original file in `originals/` and never overwrite that backup. | testOriginalBackup, testCreateAndListVersions, testApplyWorkflow |
| REQ-VER-11 | Apply full versions by replacing the game file. | testApplyWorkflow |
| REQ-VER-12 | Apply patch versions by merging keyed and unkeyed lines into the immutable original (never into a previously patched file). | testTextAndPatchMerging, testPatchAppliesToImmutableOriginal |
| REQ-VER-13 | Refuse to modify any file when a selected version is missing or an entry is invalid. | testApplyRefusals |
| REQ-VER-14 | Restore every saved original file into the game folder (after confirmation). | testRestoreOriginals |
| REQ-VER-15 | The Config Versions screen has Add version, **Edit version**, **Clone version** and Delete version buttons, each wired to its action. | testScreensHaveButtons |

## Pre-launch commands

| ID | Requirement | Verified by |
| --- | --- | --- |
| REQ-PRE-01 | The Add Version screen has an **Edit command** button for pre-launch command lists. It is shown only for that type and changes the selected command in place; an empty command, or no selected command, is refused and leaves the list unchanged. | testScreensHaveButtons, testAddVersionCommandList |
| REQ-PRE-02 | Commands of the selected modes run before the game starts, from the executable's folder. Ordinary commands start asynchronously; sleep-style commands (`sleep`, or Windows `timeout`) wait before the next command and before launch. Resolution commands are started before controller commands. | testPreLaunchRunAndConfirm, testRealCommandRunner |
| REQ-PRE-03 | After launching, the status area confirms which pre-launch commands ran (count and command text) and each command is also written to `launcher.log` with its exit code, mode and version. | testPreLaunchRunAndConfirm |
| REQ-PRE-04 | A failing command is reported with its exit code; the failure message is not overwritten by the "Launched" message, and the remaining commands still run. | testPreLaunchRunAndConfirm |
| REQ-PRE-05 | A selected pre-launch version that does not exist is reported (version name and mode) and the game is not launched; it is never silently skipped. Apply also refuses and changes nothing. | testPreLaunchRunAndConfirm, testPreLaunchConfiguredIsNeverMissing |
| REQ-PRE-06 | A pre-launch version that exists is never reported as missing: not in the mode editor, not by Apply, not at Launch (regression: the `[pre-launch]` list suffix was removed with the wrong length, so the saved name could never be found). | testPreLaunchConfiguredIsNeverMissing, testVersionDisplayParsing |
| REQ-PRE-07 | Command lists from the resolution and the controller mode all run; they never override each other and are not reported as an overridden file by Apply. | testPreLaunchRunAndConfirm |
| REQ-PRE-08 | On the Add Version screen commands can be added (trimmed, non-blank) and removed. | testAddVersionCommandList |
| REQ-PRE-09 | Pre-launch commands start asynchronously by default so they do not block the next command or the main launch. Sleep-style commands are the exception and act as barriers; existing blocking/non-blocking markers remain readable for compatibility but do not change this scheduling. | testPreLaunchRunAndConfirm, testAddVersionCommandList |

## Patch-line selection

The **Select patch lines** dialog:

| ID | Requirement | Verified by |
| --- | --- | --- |
| REQ-PATCH-01 | Show line numbers and file contents. | testPatchLogic |
| REQ-PATCH-02 | When creating a patch from an external file, lines that differ from the current game file are selected by default and show the external file's value; with no external file nothing is selected by default; previously chosen lines are preselected. | testPatchDialogState |
| REQ-PATCH-03 | Lines that are not selected use lighter-grey text than selected lines. | testPatchLogic, testPatchDialogWidgets |
| REQ-PATCH-04 | Selecting and deselecting lines updates the patch selection. | testPatchDialogWidgets |
| REQ-PATCH-05 | **Previous selected** and **Next selected** move focus to the previous/next selected line and wrap around the file. | testPatchLogic, testPatchDialogWidgets |
| REQ-PATCH-06 | **Undo change** reverses the most recent selection change (repeatedly, until the history is empty). | testPatchLogic, testPatchDialogWidgets |
| REQ-PATCH-07 | Require at least one selected line before saving a patch. | testPatchOutput |
| REQ-PATCH-08 | Preserve the selected line order when writing the patch. | testPatchOutput |
| REQ-PATCH-09 | Case-insensitive search with a Find next action that continues after the previous match and wraps. | testPatchLogic |

## Regressions this suite guards against

| Reported problem | Cause | Guarded by |
| --- | --- | --- |
| A pre-launch command list could be configured in a mode, then the status said the version is missing. | `parseVersionDisplay` compared 14 characters for the 13-character `" [pre-launch]"` suffix, so the mode stored `default [pre-launch]`. Full versions had the same problem (`[full]` never removed), which also made Delete a full version silently do nothing. | REQ-PRE-06, REQ-MODE-07, REQ-MODE-10, REQ-VER-05, REQ-VER-06 |
| No confirmation that a pre-launch command ran; a failure message was replaced by "Launched". | `onLaunch` set the status per command, then overwrote it. | REQ-PRE-03, REQ-PRE-04 |
| A missing pre-launch version was silently skipped at launch. | `onLaunch` ignored versions that were not found. | REQ-PRE-05 |
| Apply claimed pre-launch lists of both modes "overrode" each other. | Command lists were deduplicated like files. | REQ-PRE-07 |
