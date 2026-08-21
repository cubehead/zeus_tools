[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$requiredFiles = @(
    'README.md',
    'README_zh-CN.md',
    'LICENSE',
    'PRIVACY.md',
    'SECURITY.md',
    'CONTRIBUTING.md',
    'CHANGELOG.md',
    'THIRD_PARTY_NOTICES.md',
    'docs/adr/001-ui-framework.md',
    'docs/adr/002-processing-registry.md',
    'docs/README.md',
    'docs/development.md',
    'docs/implementation-plan.md',
    'docs/packaging.md',
    'docs/product-requirements.md',
    'docs/release-checklist.md',
    'docs/releases/v0.1.0.md',
    'docs/releases/v0.2.0.md',
    'docs/releases/v0.2.1.md',
    'docs/assets/overview-dark.jpg',
    'docs/assets/csv-table.jpg',
    'docs/assets/digest-hmac.jpg',
    'docs/assets/jwt-inspect.jpg',
    'docs/assets/image-preview.jpg',
    'docs/licenses/EUI-NEO-APACHE-2.0.txt',
    'docs/licenses/FONT-AWESOME-FREE.txt',
    'docs/licenses/FREETYPE.txt',
    'docs/licenses/GLFW.md',
    'docs/licenses/LIBPNG.txt',
    'docs/licenses/MD4C-MIT.md',
    'docs/licenses/PUGIXML-MIT.md',
    'docs/licenses/TRAY-MIT.txt',
    'docs/licenses/YAML-CPP-MIT.txt',
    'docs/licenses/YYJSON-MIT.txt',
    'docs/licenses/ZLIB.txt',
    'docs/licenses/tomlplusplus.txt',
    'cmake/validate_installed_package.cmake',
    'scripts/check-package.sh',
    'scripts/check-package.ps1',
    'scripts/check-repository.sh',
    'scripts/check-repository.ps1',
    'scripts/sign-windows.ps1'
)

foreach ($relativePath in $requiredFiles) {
    $path = Join-Path $projectRoot $relativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -eq 0) {
        throw "Missing or empty required file: $relativePath"
    }
}

$cmakeLists = Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw -Encoding utf8
$thirdPartyNotices = Get-Content -LiteralPath (Join-Path $projectRoot 'THIRD_PARTY_NOTICES.md') -Raw -Encoding utf8
$dependencyRevisions = @(
    'f2a3b72104bd946988f8ebe0a13dda956f3455ae',
    'ee86beb30e4973f5feffe3ce63bfa4fbadf72f38',
    '56e3bb550c91fd7005566f19c079cb7a503223cf',
    '30172438cee64926dc41fdd9c11fb3ba5b2ba9de'
)
foreach ($revision in $dependencyRevisions) {
    if (-not $cmakeLists.Contains($revision) -or -not $thirdPartyNotices.Contains($revision)) {
        throw "Dependency revision is missing from CMake or third-party notices: $revision"
    }
}

$git = (Get-Command git -ErrorAction Stop).Source
$ignoredPaths = @(
    'build/.zeus-ignore-probe',
    '.codegraph/.zeus-ignore-probe',
    '.cursor/.zeus-ignore-probe',
    'resources/icons/zeus-tools.iconset/.zeus-ignore-probe'
)
foreach ($relativePath in $ignoredPaths) {
    & $git -C $projectRoot check-ignore --quiet $relativePath
    if ($LASTEXITCODE -ne 0) {
        throw "Local/generated path is not ignored: $relativePath"
    }
}

if (Test-Path -LiteralPath (Join-Path $projectRoot '.github/workflows') -PathType Container) {
    throw 'Unexpected .github/workflows directory: this repository currently uses manual checks.'
}

$rg = (Get-Command rg -ErrorAction Stop).Source
& $rg -n --hidden --glob '!build/**' --glob '!.git/**' `
    '(BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY|AKIA[0-9A-Z]{16})' $projectRoot
if ($LASTEXITCODE -eq 0) {
    throw 'Possible credential material detected.'
}
if ($LASTEXITCODE -ne 1) {
    throw "Credential scan failed with exit code $LASTEXITCODE"
}

Write-Host 'Repository publication checks passed.'
