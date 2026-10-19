[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PackagePath,

    [Parameter()]
    [string]$InstallDir = $PSScriptRoot,

    [switch]$StartAfter
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-PackageRoot {
    param([string]$ExtractDir)

    $candidates = @(
        (Join-Path $ExtractDir 'dist\server'),
        (Join-Path $ExtractDir 'server'),
        $ExtractDir
    )

    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate 'dist_back\skymp5-server.js')) {
            return $candidate
        }
    }

    throw 'Could not locate extracted server root inside package'
}

function Backup-PathIfExists {
    param(
        [string]$SourcePath,
        [string]$BackupDir
    )

    if (Test-Path $SourcePath) {
        New-Item -ItemType Directory -Path $BackupDir -Force | Out-Null
        Copy-Item -Path $SourcePath -Destination $BackupDir -Recurse -Force
    }
}

function Stop-SkympServer {
    $processes = Get-CimInstance Win32_Process |
        Where-Object {
            $_.Name -eq 'node.exe' -and $_.CommandLine -match 'dist_back[\\/]skymp5-server\.js'
        }

    foreach ($process in $processes) {
        Write-Host "[SkyMP] Stopping existing server process $($process.ProcessId)..."
        Stop-Process -Id $process.ProcessId -Force -ErrorAction SilentlyContinue
    }
}

function Copy-ReleaseFiles {
    param(
        [string]$SourceDir,
        [string]$TargetDir
    )

    $excluded = @('data', 'server-settings.json', 'server-settings-dump.json', 'server-settings-merged.json')

    Get-ChildItem -LiteralPath $SourceDir | ForEach-Object {
        if ($excluded -contains $_.Name) {
            return
        }

        Copy-Item -Path $_.FullName -Destination $TargetDir -Recurse -Force
    }
}

if (-not (Test-Path $PackagePath -PathType Leaf)) {
    throw "Package not found: $PackagePath"
}

$resolvedInstallDir = (Resolve-Path -LiteralPath $InstallDir).Path
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backupDir = Join-Path $resolvedInstallDir "backups\update-$timestamp"
$extractDir = Join-Path ([System.IO.Path]::GetTempPath()) ("skymp-update-" + [guid]::NewGuid().ToString('N'))

New-Item -ItemType Directory -Path $extractDir -Force | Out-Null

try {
    Write-Host "[SkyMP] Extracting package: $PackagePath"
    $lowerPath = $PackagePath.ToLowerInvariant()
    if ($lowerPath.EndsWith('.zip')) {
        Expand-Archive -Path $PackagePath -DestinationPath $extractDir -Force
    } elseif ($lowerPath.EndsWith('.tar.gz') -or $lowerPath.EndsWith('.tgz')) {
        tar -xzf $PackagePath -C $extractDir
        if ($LASTEXITCODE -ne 0) {
            throw 'tar extraction failed'
        }
    } else {
        throw "Unsupported package format: $PackagePath"
    }

    $packageRoot = Get-PackageRoot -ExtractDir $extractDir

    Write-Host "[SkyMP] Creating backup in $backupDir"
    Backup-PathIfExists -SourcePath (Join-Path $resolvedInstallDir 'data') -BackupDir $backupDir
    Backup-PathIfExists -SourcePath (Join-Path $resolvedInstallDir 'server-settings.json') -BackupDir $backupDir
    Backup-PathIfExists -SourcePath (Join-Path $resolvedInstallDir 'server-settings-dump.json') -BackupDir $backupDir
    Backup-PathIfExists -SourcePath (Join-Path $resolvedInstallDir 'server-settings-merged.json') -BackupDir $backupDir

    Stop-SkympServer

    Write-Host "[SkyMP] Copying new release files into $resolvedInstallDir"
    Copy-ReleaseFiles -SourceDir $packageRoot -TargetDir $resolvedInstallDir

    if ($StartAfter) {
        Write-Host '[SkyMP] Starting server...'
        Start-Process -FilePath (Join-Path $resolvedInstallDir 'launch_server.bat') -WorkingDirectory $resolvedInstallDir | Out-Null
    } else {
        Write-Host '[SkyMP] Update finished. Start the server with .\launch_server.bat'
    }

    Write-Host "[SkyMP] Backup kept at: $backupDir"
}
finally {
    if (Test-Path $extractDir) {
        Remove-Item -Path $extractDir -Recurse -Force
    }
}