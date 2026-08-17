[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Fa-f ]{40,}$')]
    [string]$CertificateThumbprint,

    [string]$BuildRoot = 'build/package-windows',
    [uri]$TimestampUrl = 'https://timestamp.digicert.com',
    [switch]$MachineStore
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($TimestampUrl.Scheme -ne 'https') {
    throw 'TimestampUrl must use HTTPS'
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$resolvedBuildRoot = (Resolve-Path -LiteralPath (Join-Path $projectRoot $BuildRoot)).Path
$executables = @(
    Join-Path $resolvedBuildRoot 'ZeusTools.exe'
    Join-Path $resolvedBuildRoot 'zeus-tools-cli.exe'
)
foreach ($executable in $executables) {
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Missing Windows executable: $executable"
    }
}

$thumbprint = ($CertificateThumbprint -replace '\s', '').ToUpperInvariant()
if ($thumbprint -notmatch '^[0-9A-F]{40}$') {
    throw 'Certificate thumbprint must contain exactly 40 hexadecimal digits'
}

$storeLocation = if ($MachineStore) { 'LocalMachine' } else { 'CurrentUser' }
$certificatePath = "Cert:\$storeLocation\My\$thumbprint"
if (-not (Test-Path -LiteralPath $certificatePath)) {
    throw "Certificate was not found in $storeLocation\My"
}
$certificate = Get-Item -LiteralPath $certificatePath
if (-not $certificate.HasPrivateKey) {
    throw 'The selected certificate does not have an accessible private key'
}
if ($certificate.NotAfter -le (Get-Date)) {
    throw "The selected certificate expired on $($certificate.NotAfter.ToString('u'))"
}
$codeSigningOid = '1.3.6.1.5.5.7.3.3'
$supportsCodeSigning = @($certificate.EnhancedKeyUsageList) |
    Where-Object { $_.ObjectId.Value -eq $codeSigningOid }
if ($supportsCodeSigning.Count -eq 0) {
    throw 'The selected certificate is not valid for code signing'
}

$signtoolCommand = Get-Command signtool.exe -ErrorAction SilentlyContinue
if ($null -ne $signtoolCommand) {
    $signtool = $signtoolCommand.Source
} else {
    $kitsBin = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $signtoolMatches = @(
        Get-ChildItem -LiteralPath $kitsBin -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'x64\signtool.exe' } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf }
    )
    if ($signtoolMatches.Count -eq 0) {
        throw 'signtool.exe was not found; install the Windows SDK signing tools'
    }
    $signtool = $signtoolMatches[0]
}

$storeArguments = @('/s', 'My')
if ($MachineStore) {
    $storeArguments += '/sm'
}

foreach ($executable in $executables) {
    if (-not $PSCmdlet.ShouldProcess($executable, 'Apply Authenticode signature')) {
        continue
    }
    & $signtool sign `
        /fd SHA256 `
        /td SHA256 `
        /tr $TimestampUrl.AbsoluteUri `
        @storeArguments `
        /sha1 $thumbprint `
        $executable
    if ($LASTEXITCODE -ne 0) {
        throw "Signing failed for $executable with exit code $LASTEXITCODE"
    }

    & $signtool verify /pa /all $executable
    if ($LASTEXITCODE -ne 0) {
        throw "Signature verification failed for $executable with exit code $LASTEXITCODE"
    }
    $signature = Get-AuthenticodeSignature -LiteralPath $executable
    if ($signature.Status -ne 'Valid') {
        throw "Authenticode status for $executable is $($signature.Status)"
    }
    if ($null -eq $signature.TimeStamperCertificate) {
        throw "The signature for $executable does not contain a trusted timestamp"
    }
}

Write-Host 'Windows GUI and CLI Authenticode signatures verified'
