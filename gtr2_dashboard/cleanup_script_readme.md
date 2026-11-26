# Bulk File & Folder Cleanup Script (`DeleteList.ps1`)

A lightweight Windows PowerShell utility that allows you to select a text file containing a list of file and folder paths, preview the targets, and permanently delete them after explicit confirmation.

---

## 📋 Prerequisites & One-Time Setup

By default, Windows blocks PowerShell scripts from running via a double-click to prevent accidental execution. Follow these steps once to set up a convenient double-clickable shortcut.

### 1. Save the Script
Ensure the PowerShell script is saved with the `.ps1` extension (e.g., `DeleteList.ps1`) in a permanent location on your computer.

### 2. Enable Local Script Execution
1. Press the **Windows Key**, type `PowerShell`, right-click **Windows PowerShell**, and select **Run as administrator**.
2. Paste and run the following command:
   ```powershell
   Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
   ```
3. Type `Y` and press **Enter** to confirm.

### 3. Create a Double-Click Shortcut
1. Right-click your saved `DeleteList.ps1` file.
2. Select **Send to** > **Desktop (create shortcut)**.
3. Go to your Desktop, right-click the new shortcut, and choose **Properties**.
4. In the **Target** box, prepend `powershell.exe -ExecutionPolicy Bypass -File ` before the existing file path. It should look like this:
   ```cmd
   powershell.exe -ExecutionPolicy Bypass -File "C:\Path\To\Your\DeleteList.ps1"
   ```
5. Click **Apply** and **OK**.

---

## 📝 Preparing Your Target List

Create a standard `.txt` file containing the full paths of the files and folders you wish to delete.

* **Format:** One full path per line.
* **Comments:** Any line starting with `#` will be ignored.
* **Blank Lines:** Empty lines are automatically skipped.

### Example `targets.txt`
```text
# Temporary project folders to remove
C:\Users\YourName\Desktop\Old_Project_Backup
C:\Users\YourName\Downloads\installer_cache

# Specific files to clean up
C:\Users\YourName\Documents\draft_notes.docx
D:\Temp\debug_log.txt
```

---

## 🚀 How to Use

1. **Launch:** Double-click the `DeleteList.ps1` shortcut you created on your Desktop.
2. **Select File:** A standard Windows file picker dialog will open. Browse to and select your `.txt` list of targets, then click **Open**.
3. **Review Targets:** The console window will display every path from your file alongside its current status:
   * `[Found]` — The file or folder exists and is queued for deletion.
   * `[Missing]` — The path could not be found (it will be safely skipped).
4. **Confirm Deletion:** To proceed, type `YES` (case-sensitive, all caps) and press **Enter**. Typing anything else will safely cancel the operation without deleting anything.
5. **Finish:** Once the script finishes processing the list, press **Enter** to close the window.

---

## ⚠️ Important Safety Warnings

* **Permanent Deletion:** This script uses `Remove-Item -Force -Recurse`. Deleted files and folders **bypass the Windows Recycle Bin** and cannot be easily recovered.
* **Verify Paths:** Always double-check the `[Found]` list in the console window before typing `YES`.
* **Permissions:** If you are deleting protected system files or folders inside `C:\Program Files`, you may need to right-click your desktop shortcut and select **Run as administrator**.