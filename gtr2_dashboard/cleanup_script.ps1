# Ensure script runs with Administrator privileges if needed, or normal user
Add-Type -AssemblyName System.Windows.Forms

# 1. Prompt user to select a text file containing paths
Write-Host "Please select the text file containing the list of files and folders..." -ForegroundColor Cyan

$OpenFileDialog = New-Object System.Windows.Forms.OpenFileDialog
$OpenFileDialog.Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
$OpenFileDialog.Title = "Select the file list"

$Result = $OpenFileDialog.ShowDialog()

if ($Result -ne [System.Windows.Forms.DialogResult]::OK) {
    Write-Host "No file selected. Exiting." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit
}

$FilePath = $OpenFileDialog.FileName
Write-Host "Loaded file: $FilePath" -ForegroundColor Green

# 2. Read lines from the file and filter out empty lines or comments
if (-not (Test-Path $FilePath)) {
    Write-Host "Error: The selected file does not exist." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit
}

$Targets = Get-Content $FilePath | Where-Object { $_.Trim() -ne "" -and -not $_.StartsWith("#") }

if ($Targets.Count -eq 0) {
    Write-Host "The file is empty or contains no valid paths." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit
}

# 3. Show the list of files/folders to the user
Write-Host "`n----------------------------------------" -ForegroundColor DarkGray
Write-Host "The following items will be targeted for deletion:" -ForegroundColor Yellow
Write-Host "----------------------------------------" -ForegroundColor DarkGray

foreach ($Target in $Targets) {
    if (Test-Path $Target) {
        Write-Host "  [Found]   $Target" -ForegroundColor White
    } else {
        Write-Host "  [Missing] $Target" -ForegroundColor DarkGray
    }
}

Write-Host "----------------------------------------" -ForegroundColor DarkGray

# 4. Ask for confirmation before deleting
$Confirmation = Read-Host "Are you SURE you want to delete these items? Type 'YES' to proceed"

if ($Confirmation -cne 'YES') {
    Write-Host "Operation cancelled by user. No files were deleted." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit
}

# 5. Perform deletion
Write-Host "`nStarting deletion process..." -ForegroundColor Cyan

foreach ($Target in $Targets) {
    if (Test-Path $Target) {
        try {
            # Remove-Item works for both files and folders (-Recurse handles folders with contents)
            Remove-Item -LiteralPath $Target -Recurse -Force -ErrorAction Stop
            Write-Host "Deleted: $Target" -ForegroundColor Green
        }
        catch {
            Write-Host "Failed to delete '$Target': $_" -ForegroundColor Red
        }
    } else {
        Write-Host "Skipped (not found): $Target" -ForegroundColor DarkGray
    }
}

Write-Host "`nProcess completed." -ForegroundColor Cyan
Read-Host "Press Enter to close"
