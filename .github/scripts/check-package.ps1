# Fails when a program or library in the package loads a library that neither the package nor
# Windows provides, which is what stops an installed application from starting.
param(
    [Parameter(Mandatory)] [string] $Stage,
    [string[]] $ProvidedLater = @()
)

$ErrorActionPreference = 'Stop'

$system = Join-Path $env:SystemRoot 'System32'
$shipped = @{}
foreach ($file in Get-ChildItem -LiteralPath $Stage -File) {
    $shipped[$file.Name.ToLowerInvariant()] = $true
}

$missing = [System.Collections.Generic.List[string]]::new()
$binaries = Get-ChildItem -LiteralPath $Stage -File | Where-Object { $_.Extension -in '.exe', '.dll' }
foreach ($binary in $binaries) {
    $reading = $false
    foreach ($line in (dumpbin /nologo /dependents $binary.FullName)) {
        if ($line -match 'Image has the following dependencies') {
            $reading = $true
            continue
        }
        if ($line -match 'delay load dependencies' -or $line -match '^\s*Summary\s*$') {
            $reading = $false
            continue
        }
        if (-not $reading -or -not ($line -match '^\s+(\S+\.dll)\s*$')) {
            continue
        }
        $name = $Matches[1].ToLowerInvariant()
        if ($shipped.ContainsKey($name)) { continue }
        if ($name -like 'api-ms-*' -or $name -like 'ext-ms-*') { continue }
        # The Visual C++ runtime is installed alongside the application.
        if ($name -match '^(vcruntime|msvcp|concrt|vccorlib)\d+' -or $name -eq 'ucrtbased.dll') { continue }
        if (Test-Path -LiteralPath (Join-Path $system $name)) { continue }
        if (@($ProvidedLater | Where-Object { $name -like $_ }).Count -gt 0) { continue }
        $missing.Add("$($binary.Name) needs $name")
    }
}

if ($missing.Count -gt 0) {
    $missing | ForEach-Object { Write-Host $_ }
    throw 'Libraries the application loads are missing from the package'
}
Write-Host "Every library the application loads is in $Stage"
