[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Artifact,

    [switch]$RequireSignature,
    [switch]$SkipGui
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$cmakeLists = Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw -Encoding utf8
if ($cmakeLists -notmatch 'project\(ZeusTools VERSION ([^ )]+)') {
    throw 'Unable to read the project version from CMakeLists.txt'
}
$projectVersion = $Matches[1]
$resolvedArtifact = (Resolve-Path -LiteralPath $Artifact).Path
$temporaryRoot = $null

function Assert-ExitCode([string]$Operation) {
    if ($LASTEXITCODE -ne 0) {
        throw "$Operation failed with exit code $LASTEXITCODE"
    }
}

try {
    if (Test-Path -LiteralPath $resolvedArtifact -PathType Leaf) {
        if ([IO.Path]::GetExtension($resolvedArtifact) -ne '.zip') {
            throw "Windows package must be a ZIP archive: $resolvedArtifact"
        }

        $checksumPath = "$resolvedArtifact.sha256"
        if (-not (Test-Path -LiteralPath $checksumPath -PathType Leaf)) {
            throw "Missing SHA-256 checksum: $checksumPath"
        }
        $checksumText = Get-Content -LiteralPath $checksumPath -Raw -Encoding ascii
        $expectedHash = ($checksumText -split '\s+')[0].ToLowerInvariant()
        $actualHash = (Get-FileHash -LiteralPath $resolvedArtifact -Algorithm SHA256).Hash.ToLowerInvariant()
        if ([string]::IsNullOrWhiteSpace($expectedHash) -or $expectedHash -ne $actualHash) {
            throw "SHA-256 mismatch: $resolvedArtifact"
        }
        Write-Host "SHA-256 verified: $actualHash"

        $temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ("zeus-package-{0}" -f [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
        Expand-Archive -LiteralPath $resolvedArtifact -DestinationPath $temporaryRoot
        $guiMatches = @(Get-ChildItem -LiteralPath $temporaryRoot -Recurse -File -Filter 'ZeusTools.exe')
        if ($guiMatches.Count -ne 1) {
            throw "Expected exactly one ZeusTools.exe in the archive; found $($guiMatches.Count)"
        }
        $packageRoot = $guiMatches[0].Directory.FullName
    } elseif (Test-Path -LiteralPath $resolvedArtifact -PathType Container) {
        $packageRoot = $resolvedArtifact
    } else {
        throw "Package does not exist: $resolvedArtifact"
    }

    $cmake = (Get-Command cmake -ErrorAction Stop).Source
    & $cmake `
        "-DZEUS_PACKAGE_ROOT=$packageRoot" `
        '-DZEUS_PACKAGE_PLATFORM=windows' `
        "-DZEUS_PACKAGE_VERSION=$projectVersion" `
        -P (Join-Path $projectRoot 'cmake/validate_installed_package.cmake')
    Assert-ExitCode 'Package content validation'

    $gui = Join-Path $packageRoot 'ZeusTools.exe'
    $cli = Join-Path $packageRoot 'zeus-tools-cli.exe'
    $versionInfo = (Get-Item -LiteralPath $gui).VersionInfo
    if ($versionInfo.ProductVersion -ne $projectVersion) {
        throw "ProductVersion $($versionInfo.ProductVersion) does not match $projectVersion"
    }
    Write-Host "VersionInfo verified: file=$($versionInfo.FileVersion) product=$($versionInfo.ProductVersion)"

    & $cmake `
        "-DZEUS_CLI=$cli" `
        "-DZEUS_FIXTURES=$(Join-Path $projectRoot 'tests/fixtures')" `
        -P (Join-Path $projectRoot 'tests/cli_smoke.cmake')
    Assert-ExitCode 'Packaged CLI smoke test'
    Write-Host 'Packaged CLI smoke test passed'

    $signature = Get-AuthenticodeSignature -LiteralPath $gui
    Write-Host "Authenticode status: $($signature.Status)"
    if ($RequireSignature -and $signature.Status -ne 'Valid') {
        throw "A valid Authenticode signature is required; status is $($signature.Status)"
    }

    if (-not $SkipGui) {
        $process = Start-Process -FilePath $gui -WorkingDirectory $packageRoot -WindowStyle Hidden -PassThru
        try {
            Start-Sleep -Seconds 4
            if ($process.HasExited) {
                throw "GUI exited during startup with code $($process.ExitCode)"
            }
            Write-Host 'GUI startup smoke test passed'
        } finally {
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id
                $process.WaitForExit()
            }
        }
    }

    Write-Host "Validated Zeus Tools $projectVersion on $([Environment]::OSVersion.VersionString)"
} finally {
    if ($null -ne $temporaryRoot -and (Test-Path -LiteralPath $temporaryRoot)) {
        $resolvedTemporaryRoot = (Resolve-Path -LiteralPath $temporaryRoot).Path
        $expectedParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
        $actualParent = [IO.Directory]::GetParent($resolvedTemporaryRoot).FullName.TrimEnd('\')
        if ($actualParent -ne $expectedParent -or (Split-Path -Leaf $resolvedTemporaryRoot) -notlike 'zeus-package-*') {
            throw "Refusing to remove unexpected temporary path: $resolvedTemporaryRoot"
        }
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    }
}
