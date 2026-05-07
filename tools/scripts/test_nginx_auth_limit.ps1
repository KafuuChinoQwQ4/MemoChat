param(
    [string]$BaseUrl = "http://127.0.0.1",
    [string]$Route = "/user_login",
    [string]$Method = "POST",
    [int]$RequestCount = 18,
    [int]$TimeoutSec = 5,
    [string]$Body = '{"email":"limit-smoke@example.invalid","passwd":"limit-smoke","client_ver":"3.0.0"}',
    [string]$ContentType = "application/json",
    [switch]$AllowNo429,
    [switch]$CheckDockerLogs,
    [string]$NginxContainerName = "memochat-nginx-lb",
    [int]$DockerLogTail = 200
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Net.Http

if ($RequestCount -lt 1) {
    Write-Host "RequestCount must be at least 1."
    exit 2
}

if ($TimeoutSec -lt 1) {
    Write-Host "TimeoutSec must be at least 1."
    exit 2
}

function Join-Url {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Base,
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $normalizedBase = $Base.TrimEnd("/")
    if ($Path.StartsWith("/")) {
        return "$normalizedBase$Path"
    }
    return "$normalizedBase/$Path"
}

function New-SmokeRequest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RequestMethod,
        [Parameter(Mandatory = $true)]
        [string]$Uri,
        [string]$RequestBody,
        [string]$RequestContentType,
        [int]$RequestNumber
    )

    $request = [System.Net.Http.HttpRequestMessage]::new([System.Net.Http.HttpMethod]::new($RequestMethod.ToUpperInvariant()), $Uri)
    $request.Headers.TryAddWithoutValidation("X-MemoChat-Smoke", "nginx-auth-limit") | Out-Null
    $request.Headers.TryAddWithoutValidation("X-MemoChat-Smoke-Seq", [string]$RequestNumber) | Out-Null

    if ($null -ne $RequestBody -and $RequestBody.Length -gt 0) {
        $content = [System.Net.Http.StringContent]::new($RequestBody, [System.Text.Encoding]::UTF8, $RequestContentType)
        $request.Content = $content
    }

    return $request
}

function Get-StatusKey {
    param($Result)

    if ($Result.StatusCode) {
        return [string]$Result.StatusCode
    }
    return "ERROR"
}

function Test-NginxDocker429 {
    param(
        [string]$ContainerName,
        [int]$Tail
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $logs = & docker logs --tail $Tail $ContainerName 2>&1
        $dockerExitCode = $LASTEXITCODE
    } catch {
        Write-Host "Docker log 429 check unavailable: $($_.Exception.Message)"
        return
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($dockerExitCode -ne 0) {
        Write-Host "Docker log 429 check unavailable: docker logs exited with $dockerExitCode."
        return
    }

    $logText = $logs -join "`n"
    if ($logText -match '"status"\s*:\s*"?429"?|status=429|\s429\s') {
        Write-Host "Docker log 429 check: observed 429 in recent $ContainerName logs."
    } else {
        Write-Host "Docker log 429 check: no 429 found in last $Tail lines."
    }
}

$uri = Join-Url -Base $BaseUrl -Path $Route
$client = [System.Net.Http.HttpClient]::new()
$client.Timeout = [TimeSpan]::FromSeconds($TimeoutSec)
$tasks = New-Object 'System.Collections.Generic.List[System.Threading.Tasks.Task[System.Net.Http.HttpResponseMessage]]'
$results = New-Object 'System.Collections.Generic.List[object]'
$startedAt = Get-Date

Write-Host "=== Nginx auth limit smoke ==="
Write-Host "Target: $Method $uri"
Write-Host "Requests: $RequestCount"
Write-Host "TimeoutSec: $TimeoutSec"

try {
    for ($i = 1; $i -le $RequestCount; $i++) {
        $request = New-SmokeRequest `
            -RequestMethod $Method `
            -Uri $uri `
            -RequestBody $Body `
            -RequestContentType $ContentType `
            -RequestNumber $i

        $tasks.Add($client.SendAsync($request))
    }

    try {
        [System.Threading.Tasks.Task]::WaitAll($tasks.ToArray())
    } catch [System.AggregateException] {
        # Individual task states are classified below so one failed request does not hide status counts.
    }

    foreach ($task in $tasks) {
        if ($task.IsFaulted) {
            $results.Add([pscustomobject]@{
                StatusCode = $null
                Error      = $task.Exception.GetBaseException().Message
            })
            continue
        }

        if ($task.IsCanceled) {
            $results.Add([pscustomobject]@{
                StatusCode = $null
                Error      = "Request timed out or was canceled."
            })
            continue
        }

        $response = $task.Result
        try {
            $results.Add([pscustomobject]@{
                StatusCode = [int]$response.StatusCode
                Error      = $null
            })
        } finally {
            $response.Dispose()
        }
    }
} finally {
    $client.Dispose()
}

$elapsedMs = [int]((Get-Date) - $startedAt).TotalMilliseconds
$groups = $results | Group-Object -Property { Get-StatusKey -Result $_ } | Sort-Object Name
$observed429 = ($results | Where-Object { $_.StatusCode -eq 429 } | Select-Object -First 1) -ne $null
$errors = $results | Where-Object { $_.Error }

Write-Host "ElapsedMs: $elapsedMs"
Write-Host "=== Status counts ==="
foreach ($group in $groups) {
    Write-Host "$($group.Name): $($group.Count)"
}

if ($errors.Count -gt 0) {
    Write-Host "=== Request errors ==="
    $errors | Select-Object -First 5 | ForEach-Object {
        Write-Host $_.Error
    }
    if ($errors.Count -gt 5) {
        Write-Host "... $($errors.Count - 5) more error(s)"
    }
}

Write-Host "Observed429: $observed429"

if ($CheckDockerLogs) {
    Test-NginxDocker429 -ContainerName $NginxContainerName -Tail $DockerLogTail
}

if (-not $observed429 -and -not $AllowNo429) {
    exit 1
}
