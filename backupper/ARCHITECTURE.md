# Application Features and Navigation Flow

- Main screen (`screen-main`):
  - Displays the primary application dashboard.
  - Automatically handles configuration lifecycle:
    - Checks for the existence of `cfg.yml`.
    - Automatically instantiates `cfg.yml` from `cfg.yml.template` if missing.
  - Provides the following primary actions:
    - **Add Mix Tape** button:
      - Opens a native GTK folder chooser dialog ("Select Mix Tape Folder").
      - If a folder is selected:
        - Logs the selected folder directory string path to the console.
        - Closes the dialog and remains on the Main screen.
      - If cancelled:
        - Closes the dialog safely and returns to the Main screen.
    - **Configuration View** action:
      - Reads, sanitizes (escapes quotes and newlines), and evaluates local JSON configuration payload data.
      - Transfers data asynchronously via `window.receiveConfig`.
    - **Exit App** action:
      - Sends a termination request to the back-end lifecycle loop, shutting down the WebView UI entirely.
  - **Keyboard Interactions**:
    - Pressing **Escape**: Routes the user directly to the Exit Confirmation screen.

- Mixes screen (`screen-mixes`):
  - Displays sub-screen options for managing mixed tracks.
  - **Keyboard Interactions**:
    - Pressing **Escape**: Dismisses the view and routes back to the Main screen.

- Exit Confirmation screen (`screen-confirm-exit`):
  - Prompts the user to confirm application exit.
  - **Keyboard Interactions**:
    - Pressing **Escape**: Cancels the exit request and safely drops back to the Main screen.
