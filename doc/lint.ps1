<#
.SYNOPSIS
  NetSDRStation-VST LLM-Wiki Lint Script
.DESCRIPTION
  Checks the doc/ wiki for consistency problems:
  1. Orphan pages (not listed in index.md)
  2. Duplicate index entries
  3. Stale claims (stale_after >= today)
  4. Contradictions (✅ vs ❌ on same feature — heuristic)
  5. Cross-reference health (deprecated files not in archive/)
  6. Summary report

  Usage: pwsh doc/lint.ps1
  Exit code: 0 = all clean, 1 = warnings, 2 = errors
#>

param(
  [switch]$Fix      # Auto-fix stale entries in index.md where possible
)

$ErrorActionPreference = "Stop"
$today = [DateTime]"2026-08-29"
$rootDir = Split-Path -Parent $PSScriptRoot
$docDir = Join-Path $rootDir "doc"
$indexFile = Join-Path $docDir "index.md"
$hasErrors = $false
$hasWarnings = $false

Write-Host "=== NetSDRStation-VST Wiki Lint ===" -ForegroundColor Cyan
Write-Host "Date: $($today.ToString('yyyy-MM-dd'))`n" -ForegroundColor Gray

# ─── Helper: extract all `](./some/path) ` links from index.md ───
function Get-IndexLinks {
  param([string]$Path)
  $content = Get-Content -Path $Path -Raw
  $pattern = '\]\(\./([^)]+)\)'
  $matches = [regex]::Matches($content, $pattern)
  return $matches | ForEach-Object { $_.Groups[1].Value -replace '/', '\' } | ForEach-Object { $_.Trim() }
}

# ─── Helper: parse YAML frontmatter ───
function Get-Frontmatter {
  param([string]$Path)
  $content = Get-Content -Path $Path -Raw
  if ($content -match '^---\s*\n(.*?)\n---') {
    $yaml = $Matches[1]
    $result = @{}
    # Simple line-by-line parser for flat YAML keys (no nested blocks except generated/verified)
    $currentKey = $null
    $nested = @{}
    foreach ($line in $yaml -split '\n') {
      if ($line -match '^(\w+):\s*(.*)') {
        $currentKey = $Matches[1]
        $val = $Matches[2].Trim()
        if ($val -eq '') { $result[$currentKey] = $null; $nested[$currentKey] = @{} }
        else { $result[$currentKey] = $val.Trim('"'' ') }
      } elseif ($line -match '^\s+(\w+):\s*(.*)' -and $currentKey) {
        $subKey = $Matches[1]
        $subVal = $Matches[2].Trim().Trim('"'' ')
        $nested[$currentKey][$subKey] = $subVal
      }
    }
    foreach ($k in $nested.Keys) { $result[$k] = $nested[$k] }
    return $result
  }
  return @{}
}

# ──────────────────────────────────────────────────────────────────
# CHECK 1 — Orphan pages
# ──────────────────────────────────────────────────────────────────
Write-Host "─── Check 1: Orphan pages ───" -ForegroundColor Yellow

$indexLinks = Get-IndexLinks -Path $indexFile
$allDocFiles = @()
Get-ChildItem -Path $docDir -Filter "*.md" -Recurse | ForEach-Object {
  $rel = $_.FullName.Substring($docDir.Length + 1)
  $allDocFiles += @{ Path = $_.FullName; Relative = $rel; Name = $_.Name }
}

$orphans = @()
$orphanCount = 0
foreach ($f in $allDocFiles) {
  # Skip index, log, code_wiki by design
  if ($f.Name -match '^(index|log|code_wiki)\.md$') { continue }
  # Archive files get an extra "archive/" prefix in index links
  $searchKey = $f.Relative
  $found = $indexLinks | Where-Object { $_ -eq $searchKey -or $_ -eq $f.Name }
  if (-not $found) {
    $orphans += $f
    $orphanCount++
  }
}

if ($orphanCount -eq 0) {
  Write-Host "  ✓ No orphan pages found" -ForegroundColor Green
} else {
  $hasWarnings = $true
  Write-Host "  ⚠ $orphanCount orphan page(s):" -ForegroundColor Yellow
  foreach ($o in $orphans) {
    Write-Host "    - $($o.Relative)" -ForegroundColor Yellow
  }
}

# ──────────────────────────────────────────────────────────────────
# CHECK 2 — Duplicate index entries
# ──────────────────────────────────────────────────────────────────
Write-Host "`n─── Check 2: Duplicate index entries ───" -ForegroundColor Yellow

$dupGroups = $indexLinks | Group-Object | Where-Object { $_.Count -gt 1 }
if (-not $dupGroups) {
  Write-Host "  ✓ No duplicate entries" -ForegroundColor Green
} else {
  $hasWarnings = $true
  foreach ($g in $dupGroups) {
    Write-Host "  ~ Duplicate: $($g.Name) appears $($g.Count) times (ok if cross-category)" -ForegroundColor Yellow
  }
}

# ──────────────────────────────────────────────────────────────────
# CHECK 3 — Stale claims
# ──────────────────────────────────────────────────────────────────
Write-Host "`n─── Check 3: Stale claims ───" -ForegroundColor Yellow

$staleFiles = @()
foreach ($f in (Get-ChildItem -Path $docDir -Filter "*.md" -Recurse)) {
  if ($f.Name -match '^(index|log|code_wiki)\.md$') { continue }
  $fm = Get-Frontmatter -Path $f.FullName
  if ($fm.ContainsKey('stale_after') -and $fm['stale_after']) {
    $staleDateStr = $fm['stale_after']
    try {
      $staleDate = [DateTime]::ParseExact($staleDateStr, 'yyyy-MM-dd', $null)
      $relPath = $f.FullName.Substring($docDir.Length + 1)
      if ($today -ge $staleDate) {
        $staleFiles += @{ Path = $relPath; StaleAfter = $staleDateStr; FreshDays = ($today - $staleDate).Days }
      } else {
        Write-Host "  ✓ $relPath — fresh until $staleDateStr" -ForegroundColor DarkGray
      }
    } catch {
      Write-Host "  ⚠ $($f.Name): unparseable stale_after '$($fm['stale_after'])'" -ForegroundColor Yellow
    }
  }
}

if ($staleFiles.Count -eq 0) {
  Write-Host "  ✓ No stale files" -ForegroundColor Green
} else {
  $hasWarnings = $true
  Write-Host "  ⚠ $($staleFiles.Count) stale file(s) (consider update or extension):" -ForegroundColor Yellow
  foreach ($sf in $staleFiles) {
    Write-Host "    - $($sf.Path) — stale since $($sf.StaleAfter) ($($sf.FreshDays) days ago)" -ForegroundColor Yellow
  }
}

# ──────────────────────────────────────────────────────────────────
# CHECK 4 — Contradictions (heuristic: ✅ vs ❌ on same line context)
# ──────────────────────────────────────────────────────────────────
Write-Host "`n─── Check 4: Contradictions (✅ vs ❌) ───" -ForegroundColor Yellow

# Heuristic: find lines with ✅ in one file and ❌ in another about same element
# We look for "|" table lines containing both an emoji checkmark and a semantic keyword
$checkMarkFiles = @{}
$crossMarkFiles = @{}
foreach ($f in (Get-ChildItem -Path $docDir -Filter "*.md" -Recurse)) {
  if ($f.Name -match '^(index|log|code_wiki)\.md$') { continue }
  $lines = Get-Content -Path $f.FullName
  $rel = $f.FullName.Substring($docDir.Length + 1)
  $checks = @($lines | Where-Object { $_ -match '\|.*✅.*\|' })
  $crosses = @($lines | Where-Object { $_ -match '\|.*❌.*\|' })
  if ($checks) { $checkMarkFiles[$rel] = $checks }
  if ($crosses) { $crossMarkFiles[$rel] = $crosses }
}

Write-Host "  ℹ  Files with ✅ marks: $($checkMarkFiles.Keys.Count)" -ForegroundColor Gray
Write-Host "  ℹ  Files with ❌ marks: $($crossMarkFiles.Keys.Count)" -ForegroundColor Gray
Write-Host "  ℹ  Manual review recommended — automated contradiction detection is heuristic" -ForegroundColor DarkGray

# Flag any file that has BOTH ✅ and ❌ (possible internal inconsistency)
$both = @()
foreach ($f in $checkMarkFiles.Keys) {
  if ($crossMarkFiles.ContainsKey($f)) {
    $both += $f
  }
}
if ($both.Count -gt 0) {
  $hasWarnings = $true
  Write-Host "  ⚠ $($both.Count) file(s) with mixed ✅/❌ marks (possible internal inconsistency):" -ForegroundColor Yellow
  foreach ($b in $both) { Write-Host "    - $b" -ForegroundColor Yellow }
}

# ──────────────────────────────────────────────────────────────────
# CHECK 5 — Cross-reference health (deprecated not in archive/)
# ──────────────────────────────────────────────────────────────────
Write-Host "`n─── Check 5: Deprecated files outside archive/ ───" -ForegroundColor Yellow

$deprecatedOutside = @()
foreach ($f in (Get-ChildItem -Path $docDir -Filter "*.md" -Exclude "archive/*")) {
  if ($f.Directory.Name -eq 'archive') { continue }
  if ($f.Name -match '^(index|log|code_wiki)\.md$') { continue }
  $fm = Get-Frontmatter -Path $f.FullName
  if ($fm.ContainsKey('status') -and $fm['status'] -eq 'deprecated') {
    $deprecatedOutside += $f.FullName.Substring($docDir.Length + 1)
  }
}

if ($deprecatedOutside.Count -eq 0) {
  Write-Host "  ✓ No deprecated files outside archive/" -ForegroundColor Green
} else {
  $hasWarnings = $true
  Write-Host "  ⚠ $($deprecatedOutside.Count) deprecated file(s) still in doc/ root:" -ForegroundColor Yellow
  foreach ($d in $deprecatedOutside) {
    Write-Host "    - $d → should move to doc/archive/" -ForegroundColor Yellow
  }
}

# ──────────────────────────────────────────────────────────────────
# Summary
# ──────────────────────────────────────────────────────────────────
Write-Host "`n═══════════════════════════════════════" -ForegroundColor Cyan
if ($hasErrors) {
  Write-Host "  ✗ ERRORS found — fix before proceeding" -ForegroundColor Red
} elseif ($hasWarnings) {
  Write-Host "  ⚠ WARNINGS found — review recommended" -ForegroundColor Yellow
} else {
  Write-Host "  ✓ All checks passed — wiki is clean" -ForegroundColor Green
}
Write-Host "═══════════════════════════════════════" -ForegroundColor Cyan

if ($hasErrors) { exit 2 }
if ($hasWarnings) { exit 1 }
exit 0