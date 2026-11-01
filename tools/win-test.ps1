<#
.SYNOPSIS
  Run a ROM's self-playing test build in ares on Windows and collect its log and screenshots.

.DESCRIPTION
  Gets the test ROM (from -Rom, or the CI build of the commit checked out here), opens it in
  ares, saves the ROM's debug output and a screenshot of the ares window at every CHECK line
  (and every few seconds), then closes ares. It prints PASS or FAIL and exits 0 on PASS,
  1 on FAIL, and 2 when the ROM never said either.

  Needs: ares (https://ares-emu.net), and for CI builds Git and the GitHub CLI signed in.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\win-test.ps1
  powershell -ExecutionPolicy Bypass -File tools\win-test.ps1 -Rom dist\clankers-test.z64 -Seconds 300
#>
param(
    [string]$Name = "clankers",                 # the ROM; its test build is <name>-test.z64
    [string]$Rom = "",                          # a local .z64 to run instead of the CI build
    [string]$Ares = "C:\ares\ares.exe",
    [int]$Seconds = 240,                        # give up after this long
    [int]$ShotEvery = 3,                        # seconds between timed screenshots
    [string]$Out = ""                           # where to put the results (default test-out\<commit>)
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
[Win]::SetProcessDPIAware() | Out-Null

function Get-Commit { (git rev-parse HEAD).Trim() }

# ------------------------------------------------------------------ the ROM
$sha = ""
if (-not $Rom) {
    $sha = Get-Commit
    if (-not $Out) { $Out = Join-Path "test-out" $sha.Substring(0, 8) }
    New-Item -ItemType Directory -Force -Path $Out | Out-Null
    Write-Host "Waiting for CI to build $($sha.Substring(0, 8))..."
    $deadline = (Get-Date).AddMinutes(20)
    while ($true) {
        $runs = gh run list --commit $sha --workflow build.yml --json databaseId,status,conclusion | ConvertFrom-Json
        $run = $runs | Select-Object -First 1
        if ($run -and $run.status -eq "completed") { break }
        if ((Get-Date) -gt $deadline) { Write-Host "NO RESULT: CI did not finish building $sha"; exit 2 }
        Start-Sleep -Seconds 20
    }
    if ($run.conclusion -ne "success") { Write-Host "FAIL: the CI build of $sha did not succeed ($($run.conclusion))"; exit 1 }
    $romDir = Join-Path $Out "roms"
    gh run download $run.databaseId -n "j64-roms-$sha" -D $romDir | Out-Null
    $Rom = Join-Path $romDir "$Name-test.z64"
}
if (-not $Out) { $Out = Join-Path "test-out" ([IO.Path]::GetFileNameWithoutExtension($Rom)) }
New-Item -ItemType Directory -Force -Path $Out | Out-Null
if (-not (Test-Path $Rom)) { Write-Host "NO RESULT: $Rom not found"; exit 2 }
if (-not (Test-Path $Ares)) { Write-Host "NO RESULT: ares not found at $Ares (pass -Ares)"; exit 2 }
$Rom = (Resolve-Path $Rom).Path

# ------------------------------------------------------------------ run it
$log = Join-Path $Out "log.txt"
$err = Join-Path $Out "ares-stderr.txt"
Remove-Item -ErrorAction SilentlyContinue $log, $err
Get-ChildItem -ErrorAction SilentlyContinue (Join-Path $Out "shot-*.png") | Remove-Item

# Homebrew Mode makes ares print the ROM's ISViewer output (debugf) to its stdout.
$aresArgs = @("--system", "`"Nintendo 64`"", "--setting", "General/HomebrewMode=true", "`"$Rom`"")
$p = Start-Process -FilePath $Ares -ArgumentList $aresArgs -PassThru `
        -RedirectStandardOutput $log -RedirectStandardError $err
Write-Host "Running $Rom in ares (pid $($p.Id))"

$shots = 0
function Save-Shot([string]$label) {
    $p.Refresh()
    $h = $p.MainWindowHandle
    if ($h -eq [IntPtr]::Zero) { return }
    [Win]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 150
    $r = New-Object Win+RECT
    [Win]::GetWindowRect($h, [ref]$r) | Out-Null
    $w = $r.Right - $r.Left; $hgt = $r.Bottom - $r.Top
    if ($w -le 0 -or $hgt -le 0) { return }
    $bmp = New-Object System.Drawing.Bitmap $w, $hgt
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
    $script:shots++
    $safe = ($label -replace '[^A-Za-z0-9-]+', '-').Trim('-')
    $file = Join-Path $Out ("shot-{0:D3}-{1}.png" -f $script:shots, $safe)
    $bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
}

$result = ""
$seen = 0
$start = Get-Date
$nextShot = $start.AddSeconds(2)
while (((Get-Date) - $start).TotalSeconds -lt $Seconds -and -not $p.HasExited) {
    Start-Sleep -Milliseconds 250
    $lines = @()
    if (Test-Path $log) { $lines = @(Get-Content $log -ErrorAction SilentlyContinue) }
    for ($i = $seen; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        Write-Host "  $line"
        if ($line -match '^(CHECK|PASS|FAIL) (.*?) \|') {
            Start-Sleep -Milliseconds 300   # the ROM holds still for a second at a checkpoint
            Save-Shot "$($Matches[1])-$($Matches[2])"
            if ($Matches[1] -ne "CHECK") { $result = $line }
        }
    }
    $seen = $lines.Count
    if ($result) { break }
    if ((Get-Date) -ge $nextShot) { Save-Shot "timed"; $nextShot = (Get-Date).AddSeconds($ShotEvery) }
}

if (-not $p.HasExited) { $p.CloseMainWindow() | Out-Null; Start-Sleep -Seconds 2 }
if (-not $p.HasExited) { $p.Kill() }

Write-Host ""
Write-Host "Results in $Out ($shots screenshots, log.txt)"
if ($result -like "PASS*") { Write-Host $result; exit 0 }
if ($result -like "FAIL*") { Write-Host $result; exit 1 }
Write-Host "NO RESULT: the ROM printed no PASS or FAIL in $Seconds s (see the screenshots)"
exit 2
