param(
    [string]$OutputDir = "",
    [string]$DnsName = "localhost",
    [switch]$Force
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-RepoRoot {
    try {
        $gitRoot = git rev-parse --show-toplevel 2>$null
        if ($LASTEXITCODE -eq 0 -and $gitRoot) {
            return $gitRoot.Trim()
        }
    } catch {
    }
    return (Get-Location).Path
}

function Convert-ToPem {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Label,
        [Parameter(Mandatory = $true)]
        [byte[]]$Bytes
    )

    $base64 = [Convert]::ToBase64String($Bytes)
    $chunks = for ($i = 0; $i -lt $base64.Length; $i += 64) {
        $length = [Math]::Min(64, $base64.Length - $i)
        $base64.Substring($i, $length)
    }
    $body = ($chunks -join "`n")
    return "-----BEGIN $Label-----`n$body`n-----END $Label-----`n"
}

$repoRoot = Resolve-RepoRoot
if (-not $OutputDir) {
    $OutputDir = Join-Path $repoRoot "artifacts\certs\quic-dev"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$certPath = Join-Path $OutputDir "server-cert.pem"
$keyPath = Join-Path $OutputDir "server-key.pem"

if (((Test-Path $certPath) -or (Test-Path $keyPath)) -and -not $Force) {
    throw "Certificate files already exist. Re-run with -Force to overwrite."
}

$rsa = [System.Security.Cryptography.RSA]::Create(2048)
$dn = [System.Security.Cryptography.X509Certificates.X500DistinguishedName]::new("CN=$DnsName")
$req = [System.Security.Cryptography.X509Certificates.CertificateRequest]::new(
    $dn,
    $rsa,
    [System.Security.Cryptography.HashAlgorithmName]::SHA256,
    [System.Security.Cryptography.RSASignaturePadding]::Pkcs1
)

$san = [System.Security.Cryptography.X509Certificates.SubjectAlternativeNameBuilder]::new()
$san.AddDnsName($DnsName)
$san.AddDnsName("localhost")
$san.AddIpAddress([System.Net.IPAddress]::Parse("127.0.0.1"))
$req.CertificateExtensions.Add($san.Build())
$req.CertificateExtensions.Add(
    [System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension]::new($false, $false, 0, $true)
)
$req.CertificateExtensions.Add(
    [System.Security.Cryptography.X509Certificates.X509KeyUsageExtension]::new(
        [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::DigitalSignature,
        $true
    )
)
$eku = [System.Security.Cryptography.OidCollection]::new()
$eku.Add([System.Security.Cryptography.Oid]::new("1.3.6.1.5.5.7.3.1", "Server Authentication")) | Out-Null
$req.CertificateExtensions.Add(
    [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension]::new($eku, $true)
)

$notBefore = [DateTimeOffset]::UtcNow.AddDays(-1)
$notAfter = [DateTimeOffset]::UtcNow.AddYears(2)
$cert = $req.CreateSelfSigned($notBefore, $notAfter)

$privateKeyBytes = $null
if ($rsa -is [System.Security.Cryptography.RSACng]) {
    $privateKeyBytes = $rsa.Key.Export([System.Security.Cryptography.CngKeyBlobFormat]::Pkcs8PrivateBlob)
} else {
    try {
        $privateKeyBytes = $rsa.ExportPkcs8PrivateKey()
    } catch {
        throw "Unable to export private key in PKCS#8 format on this PowerShell/.NET runtime."
    }
}

$certPem = Convert-ToPem -Label "CERTIFICATE" -Bytes $cert.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert)
$keyPem = Convert-ToPem -Label "PRIVATE KEY" -Bytes $privateKeyBytes

Set-Content -Path $certPath -Value $certPem -Encoding ascii
Set-Content -Path $keyPath -Value $keyPem -Encoding ascii

Write-Host "Generated dev QUIC certificate:"
Write-Host "  CertFile=$certPath"
Write-Host "  PrivateKeyFile=$keyPath"
Write-Host ""
Write-Host "Set these values under [Quic] in server/ChatServer/config.ini or chatserver*.ini before using --enable-quic."
