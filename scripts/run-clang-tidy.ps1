# Runs clang-tidy on the pure C++ core sources (DSP, threading, parameter
# registry) with explicit include paths (project + MSVC STL + Windows SDK).
# These units have no VST3 SDK dependency, so they can be analyzed without a
# compile_commands.json.
#
# The VST3-boundary sources (processor/controller/editor) are analyzed in CI
# via the clangd/compile_commands workflow (doc/test-strategy.md §9).

param(
    [string]$ClangTidy = "C:\Program Files\LLVM\bin\clang-tidy.exe",
    [string]$VsRoot = "C:\Program Files\Microsoft Visual Studio\18\Community",
    [string]$SdkRoot = "C:\Program Files (x86)\Windows Kits\10\Include"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

# Locate the newest MSVC STL and Windows SDK include directories.
$msvc = Get-ChildItem "$VsRoot\VC\Tools\MSVC" -Directory |
    Sort-Object Name -Descending | Select-Object -First 1
$sdk = Get-ChildItem $SdkRoot -Directory |
    Sort-Object Name -Descending | Select-Object -First 1

$sources = @(
    "$root\source\dsp\sine_oscillator.cpp",
    "$root\source\threading\worker_thread.cpp",
    "$root\source\vst\common\parameter_registry.cpp"
)

$systemIncludes = @(
    "-isystem", "$($msvc.FullName)\include",
    "-isystem", "$($sdk.FullName)\ucrt",
    "-isystem", "$($sdk.FullName)\shared",
    "-isystem", "$($sdk.FullName)\um",
    "-isystem", "$($sdk.FullName)\winrt"
)

$projectIncludes = @(
    "-I", "$root\source",
    "-I", "$root\source\vst",
    "-I", "$root\source\dsp",
    "-I", "$root\source\threading",
    "-I", "$root\third_party\moodycamel"
)

& $ClangTidy $sources -- -std=c++20 @systemIncludes @projectIncludes

exit $LASTEXITCODE
