param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$DockerArgs
)

$ErrorActionPreference = "Stop"

$distro = if ($env:MEMOCHAT_WSL_DISTRO) { $env:MEMOCHAT_WSL_DISTRO } else { "archlinux" }
$linuxProjectRoot = if ($env:MEMOCHAT_WSL_PROJECT_ROOT) { $env:MEMOCHAT_WSL_PROJECT_ROOT } else { "/root/code/MemoChat" }
$linuxEnvFile = if ($env:MEMOCHAT_LINUX_ENV_FILE) { $env:MEMOCHAT_LINUX_ENV_FILE } else { "/root/.memochat-linux-env" }
$windowsProjectRoot = if ($env:MEMOCHAT_WINDOWS_PROJECT_ROOT) {
    $env:MEMOCHAT_WINDOWS_PROJECT_ROOT
} else {
    (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
}
$legacyWindowsDockerDataRoot = if ($env:MEMOCHAT_WINDOWS_DOCKER_DATA_ROOT) {
    $env:MEMOCHAT_WINDOWS_DOCKER_DATA_ROOT
} else {
    "D:\docker-data\memochat"
}
$linuxDockerDataRoot = if ($env:MEMOCHAT_DOCKER_DATA_ROOT) {
    $env:MEMOCHAT_DOCKER_DATA_ROOT
} else {
    "/data/docker-data/memochat"
}

function Quote-BashArg {
    param([string]$Value)
    return "'" + $Value.Replace("'", "'\''") + "'"
}

function Convert-ArchDockerArg {
    param([string]$Value)

    if ($null -eq $Value -or $Value.Length -eq 0) {
        return $Value
    }

    $converted = $Value
    $containsPath = $false

    $windowsRootBackslash = $windowsProjectRoot.TrimEnd("\", "/")
    $windowsRootSlash = $windowsRootBackslash.Replace("\", "/")
    foreach ($root in @($windowsRootBackslash, $windowsRootSlash)) {
        if ($root -and $converted.IndexOf($root, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
            $converted = [regex]::Replace(
                $converted,
                [regex]::Escape($root),
                { param($m) $linuxProjectRoot },
                [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
            )
            $containsPath = $true
        }
    }

    $legacyBackslash = $legacyWindowsDockerDataRoot.TrimEnd("\", "/")
    $legacySlash = $legacyBackslash.Replace("\", "/")
    foreach ($root in @($legacyBackslash, $legacySlash)) {
        if ($root -and $converted.IndexOf($root, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
            $converted = [regex]::Replace(
                $converted,
                [regex]::Escape($root),
                { param($m) $linuxDockerDataRoot },
                [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
            )
            $containsPath = $true
        }
    }

    if ($containsPath) {
        $converted = $converted.Replace("\", "/")
    }

    return $converted
}

$quotedArgs = @()
foreach ($arg in $DockerArgs) {
    $quotedArgs += Quote-BashArg (Convert-ArchDockerArg $arg)
}

$command = "cd $(Quote-BashArg $linuxProjectRoot) && source $(Quote-BashArg $linuxEnvFile) && docker"
if ($quotedArgs.Count -gt 0) {
    $command += " " + ($quotedArgs -join " ")
}

Push-Location $env:USERPROFILE
try {
    & wsl.exe -d $distro -- bash -lc $command
} finally {
    Pop-Location
}
