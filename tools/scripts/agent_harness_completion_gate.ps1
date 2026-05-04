[CmdletBinding()]
param(
    [string]$RepoRoot = "",
    [ValidateSet("Quick", "Full")]
    [string]$Mode = "Quick",
    [string]$OutputJson = "",
    [switch]$FailOnGitDirty,
    [switch]$RunOfflineTests,
    [switch]$StrictObservability
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    param([string]$InputRoot)

    if ($InputRoot -and (Test-Path $InputRoot)) {
        return (Resolve-Path $InputRoot).Path
    }

    try {
        $gitRoot = git rev-parse --show-toplevel 2>$null
        if ($LASTEXITCODE -eq 0 -and $gitRoot) {
            return $gitRoot.Trim()
        }
    } catch {
    }

    return (Get-Location).Path
}

function New-GateCheck {
    param(
        [string]$Name,
        [string]$Category,
        [ValidateSet("pass", "warn", "fail", "block", "skip")]
        [string]$Status,
        [string]$Summary,
        [string]$Detail = "",
        [object]$Data = $null
    )

    return [ordered]@{
        name = $Name
        category = $Category
        status = $Status
        summary = $Summary
        detail = $Detail
        data = $Data
    }
}

function Add-GateCheck {
    param(
        [string]$Name,
        [string]$Category,
        [ValidateSet("pass", "warn", "fail", "block", "skip")]
        [string]$Status,
        [string]$Summary,
        [string]$Detail = "",
        [object]$Data = $null
    )

    $script:Checks.Add((New-GateCheck -Name $Name -Category $Category -Status $Status -Summary $Summary -Detail $Detail -Data $Data)) | Out-Null
}

function Test-CommandAvailable {
    param([string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Invoke-JsonGet {
    param(
        [string]$Url,
        [int]$TimeoutSec = 10
    )

    return Invoke-RestMethod -Uri $Url -Method GET -TimeoutSec $TimeoutSec
}

function Invoke-DockerLines {
    param([string[]]$Arguments)

    $output = & docker @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    return [ordered]@{
        exit_code = $exitCode
        output = @($output)
    }
}

function Get-FileTextOrEmpty {
    param([string]$Path)
    if (-not (Test-Path $Path)) {
        return ""
    }
    return Get-Content -Raw -Encoding UTF8 $Path
}

function Test-FileContains {
    param(
        [string]$Path,
        [string]$Pattern
    )
    $text = Get-FileTextOrEmpty -Path $Path
    if (-not $text) {
        return $false
    }
    return $text -match $Pattern
}

$root = Resolve-RepoRoot -InputRoot $RepoRoot
Set-Location $root

if (-not $OutputJson) {
    $OutputJson = Join-Path $root ".ai/harness-completion-gate/latest-gate.json"
}

$script:Checks = New-Object System.Collections.Generic.List[object]
$startedAt = (Get-Date).ToUniversalTime().ToString("o")

Add-GateCheck -Name "repo_root" -Category "setup" -Status "pass" -Summary "Resolved repository root" -Data @{ repo_root = $root }

try {
    $gitStatus = @(git status --short 2>$null)
    if ($LASTEXITCODE -ne 0) {
        Add-GateCheck -Name "git_status" -Category "workspace" -Status "warn" -Summary "Could not read git status"
    } elseif ($gitStatus.Count -eq 0) {
        Add-GateCheck -Name "git_status" -Category "workspace" -Status "pass" -Summary "Working tree is clean"
    } else {
        $status = if ($FailOnGitDirty) { "fail" } else { "warn" }
        Add-GateCheck -Name "git_status" -Category "workspace" -Status $status -Summary "Working tree has changes" -Data @{
            changed_count = $gitStatus.Count
            sample = @($gitStatus | Select-Object -First 20)
        }
    }
} catch {
    Add-GateCheck -Name "git_status" -Category "workspace" -Status "warn" -Summary "Git status check raised an exception" -Detail $_.Exception.Message
}

if (-not (Test-CommandAvailable -Name "docker")) {
    Add-GateCheck -Name "docker_command" -Category "docker" -Status "block" -Summary "docker command is not available"
} else {
    Add-GateCheck -Name "docker_command" -Category "docker" -Status "pass" -Summary "docker command is available"

    $ps = Invoke-DockerLines -Arguments @("ps", "--format", "{{.Names}}|{{.Status}}|{{.Ports}}")
    if ($ps.exit_code -ne 0) {
        Add-GateCheck -Name "docker_ps" -Category "docker" -Status "block" -Summary "docker ps failed" -Detail (($ps.output) -join "`n")
    } else {
        $containers = @{}
        foreach ($line in $ps.output) {
            $parts = ([string]$line).Split("|", 3)
            if ($parts.Count -eq 3) {
                $containers[$parts[0]] = [ordered]@{
                    status = $parts[1]
                    ports = $parts[2]
                }
            }
        }

        $requiredContainers = @(
            @{ name = "memochat-ai-orchestrator"; port = "8096->8096" },
            @{ name = "memochat-ollama"; port = "11434->11434" },
            @{ name = "memochat-qdrant"; port = "6333-6334->6333-6334" },
            @{ name = "memochat-neo4j"; port = "7474->7474" },
            @{ name = "memochat-postgres"; port = "15432->5432" },
            @{ name = "memochat-prometheus"; port = "9090->9090" },
            @{ name = "memochat-grafana"; port = "3000->3000" }
        )

        $missingContainers = New-Object System.Collections.Generic.List[string]
        $badContainers = New-Object System.Collections.Generic.List[string]
        foreach ($item in $requiredContainers) {
            if (-not $containers.ContainsKey($item.name)) {
                $missingContainers.Add($item.name) | Out-Null
                continue
            }
            $info = $containers[$item.name]
            if (-not ([string]$info.status).StartsWith("Up")) {
                $badContainers.Add("$($item.name): $($info.status)") | Out-Null
            }
            if ($item.port -and ([string]$info.ports) -notlike "*$($item.port)*") {
                $badContainers.Add("$($item.name): missing stable port $($item.port)") | Out-Null
            }
        }

        if ($missingContainers.Count -gt 0 -or $badContainers.Count -gt 0) {
            Add-GateCheck -Name "docker_required_containers" -Category "docker" -Status "fail" -Summary "Required AI harness containers are not clean" -Data @{
                missing = @($missingContainers)
                bad = @($badContainers)
            }
        } else {
            Add-GateCheck -Name "docker_required_containers" -Category "docker" -Status "pass" -Summary "Required AI harness containers are running with stable published ports"
        }
    }
}

try {
    $health = Invoke-JsonGet -Url "http://127.0.0.1:8096/health"
    if ($health.status -eq "ok") {
        Add-GateCheck -Name "ai_orchestrator_health" -Category "runtime" -Status "pass" -Summary "AIOrchestrator health endpoint is ok"
    } else {
        Add-GateCheck -Name "ai_orchestrator_health" -Category "runtime" -Status "fail" -Summary "AIOrchestrator health endpoint returned a non-ok status" -Data $health
    }
} catch {
    Add-GateCheck -Name "ai_orchestrator_health" -Category "runtime" -Status "block" -Summary "AIOrchestrator health endpoint is unavailable" -Detail $_.Exception.Message
}

try {
    $layers = Invoke-JsonGet -Url "http://127.0.0.1:8096/agent/layers"
    $layerNames = @($layers.layers | ForEach-Object { $_.name })
    $expectedLayers = @("orchestration", "execution", "feedback", "memory", "runtime")
    $missingLayers = @($expectedLayers | Where-Object { $layerNames -notcontains $_ })
    if ($layers.code -eq 0 -and $missingLayers.Count -eq 0) {
        Add-GateCheck -Name "agent_layers_api" -Category "runtime" -Status "pass" -Summary "Harness layer API exposes core layers" -Data @{ layer_count = @($layers.layers).Count }
    } else {
        Add-GateCheck -Name "agent_layers_api" -Category "runtime" -Status "fail" -Summary "Harness layer API is missing expected layers" -Data @{ missing = $missingLayers; layers = $layerNames }
    }
} catch {
    Add-GateCheck -Name "agent_layers_api" -Category "runtime" -Status "block" -Summary "Harness layer API is unavailable" -Detail $_.Exception.Message
}

try {
    $skills = Invoke-JsonGet -Url "http://127.0.0.1:8096/agent/skills"
    if ($skills.code -eq 0 -and @($skills.skills).Count -gt 0) {
        Add-GateCheck -Name "agent_skills_api" -Category "runtime" -Status "pass" -Summary "Harness skill API exposes skills" -Data @{ skill_count = @($skills.skills).Count }
    } else {
        Add-GateCheck -Name "agent_skills_api" -Category "runtime" -Status "fail" -Summary "Harness skill API returned no skills" -Data $skills
    }
} catch {
    Add-GateCheck -Name "agent_skills_api" -Category "runtime" -Status "block" -Summary "Harness skill API is unavailable" -Detail $_.Exception.Message
}

try {
    $tools = Invoke-JsonGet -Url "http://127.0.0.1:8096/agent/tools"
    if ($tools.code -eq 0 -and @($tools.tools).Count -gt 0) {
        Add-GateCheck -Name "agent_tools_api" -Category "runtime" -Status "pass" -Summary "Harness tool API exposes tools" -Data @{ tool_count = @($tools.tools).Count }
    } else {
        Add-GateCheck -Name "agent_tools_api" -Category "runtime" -Status "fail" -Summary "Harness tool API returned no tools" -Data $tools
    }
} catch {
    Add-GateCheck -Name "agent_tools_api" -Category "runtime" -Status "block" -Summary "Harness tool API is unavailable" -Detail $_.Exception.Message
}

try {
    $query = [uri]::EscapeDataString('up{job="ai_orchestrator"}')
    $prom = Invoke-JsonGet -Url "http://127.0.0.1:9090/api/v1/query?query=$query"
    $value = $null
    if ($prom.status -eq "success" -and @($prom.data.result).Count -gt 0) {
        $value = [string]$prom.data.result[0].value[1]
    }
    if ($value -eq "1") {
        Add-GateCheck -Name "prometheus_ai_scrape" -Category "observability" -Status "pass" -Summary "Prometheus scrapes AIOrchestrator"
    } else {
        Add-GateCheck -Name "prometheus_ai_scrape" -Category "observability" -Status "fail" -Summary "Prometheus does not report AIOrchestrator as up" -Data $prom
    }
} catch {
    Add-GateCheck -Name "prometheus_ai_scrape" -Category "observability" -Status "block" -Summary "Prometheus query endpoint is unavailable" -Detail $_.Exception.Message
}

try {
    $targets = Invoke-JsonGet -Url "http://127.0.0.1:9090/api/v1/targets?state=active"
    $downTargets = @($targets.data.activeTargets | Where-Object { $_.health -ne "up" } | ForEach-Object {
        [ordered]@{
            job = $_.labels.job
            scrape_url = $_.scrapeUrl
            last_error = $_.lastError
        }
    })
    if ($downTargets.Count -eq 0) {
        Add-GateCheck -Name "prometheus_all_targets" -Category "observability" -Status "pass" -Summary "All Prometheus targets are up"
    } else {
        $status = if ($StrictObservability) { "fail" } else { "warn" }
        Add-GateCheck -Name "prometheus_all_targets" -Category "observability" -Status $status -Summary "Some Prometheus targets are down" -Data @{ down_targets = $downTargets }
    }
} catch {
    Add-GateCheck -Name "prometheus_all_targets" -Category "observability" -Status "warn" -Summary "Could not inspect Prometheus target health" -Detail $_.Exception.Message
}

if (Test-CommandAvailable -Name "docker") {
    $tableCheck = Invoke-DockerLines -Arguments @(
        "exec", "memochat-postgres", "psql", "-U", "memochat", "-d", "memo_pg",
        "-t", "-A", "-F", ",",
        "-c", "select to_regclass('public.ai_agent_run_trace') is not null, to_regclass('public.ai_agent_run_step') is not null, to_regclass('public.ai_agent_feedback') is not null;"
    )
    if ($tableCheck.exit_code -ne 0) {
        Add-GateCheck -Name "postgres_trace_tables" -Category "persistence" -Status "block" -Summary "Could not inspect Postgres trace tables" -Detail (($tableCheck.output) -join "`n")
    } else {
        $line = [string](@($tableCheck.output | Where-Object { $_ })[0])
        if ($line -eq "t,t,t") {
            Add-GateCheck -Name "postgres_trace_tables" -Category "persistence" -Status "pass" -Summary "Postgres trace and feedback tables exist"
        } else {
            Add-GateCheck -Name "postgres_trace_tables" -Category "persistence" -Status "fail" -Summary "Missing one or more Postgres trace tables" -Data @{ raw = $line }
        }
    }

    $traceCount = Invoke-DockerLines -Arguments @(
        "exec", "memochat-postgres", "psql", "-U", "memochat", "-d", "memo_pg",
        "-t", "-A", "-F", ",",
        "-c", "select count(*) from ai_agent_run_trace; select count(*) from ai_agent_run_step; select count(*) from ai_agent_feedback;"
    )
    if ($traceCount.exit_code -ne 0) {
        Add-GateCheck -Name "postgres_trace_activity" -Category "persistence" -Status "warn" -Summary "Could not count trace activity" -Detail (($traceCount.output) -join "`n")
    } else {
        $counts = @($traceCount.output | Where-Object { [string]$_ -match '^\d+$' } | ForEach-Object { [int]$_ })
        $runCount = if ($counts.Count -ge 1) { $counts[0] } else { 0 }
        $stepCount = if ($counts.Count -ge 2) { $counts[1] } else { 0 }
        $feedbackCount = if ($counts.Count -ge 3) { $counts[2] } else { 0 }
        if ($runCount -gt 0 -and $stepCount -gt 0) {
            Add-GateCheck -Name "postgres_trace_activity" -Category "persistence" -Status "pass" -Summary "Trace activity exists in Postgres" -Data @{
                run_trace_count = $runCount
                step_count = $stepCount
                feedback_count = $feedbackCount
            }
        } else {
            Add-GateCheck -Name "postgres_trace_activity" -Category "persistence" -Status "warn" -Summary "No trace activity found yet" -Data @{
                run_trace_count = $runCount
                step_count = $stepCount
                feedback_count = $feedbackCount
            }
        }

        if ($feedbackCount -eq 0) {
            Add-GateCheck -Name "feedback_loop_activity" -Category "feedback" -Status "warn" -Summary "No user feedback rows exist yet" -Detail "Feedback persistence exists, but the loop is not active until feedback rows are recorded."
        } else {
            Add-GateCheck -Name "feedback_loop_activity" -Category "feedback" -Status "pass" -Summary "User feedback rows exist" -Data @{ feedback_count = $feedbackCount }
        }
    }
}

$roadmapPath = Join-Path $root ".ai/memochat-agent-system-roadmap/tasks.json"
if (-not (Test-Path $roadmapPath)) {
    Add-GateCheck -Name "agent_roadmap_manifest" -Category "roadmap" -Status "warn" -Summary "Agent roadmap manifest does not exist" -Detail $roadmapPath
} else {
    try {
        $roadmap = Get-Content -Raw -Encoding UTF8 $roadmapPath | ConvertFrom-Json
        $completedIds = @($roadmap.tasks | Where-Object { $_.completed -eq $true } | ForEach-Object { $_.id })
        Add-GateCheck -Name "agent_roadmap_manifest" -Category "roadmap" -Status "pass" -Summary "Agent roadmap manifest parsed" -Data @{
            completed_count = $completedIds.Count
        }

        $expectations = @(
            @{ stage = "stage-1-contracts"; files = @("apps/server/core/AIOrchestrator/harness/contracts.py"); tokens = @("AgentSpec", "ToolSpec", "GuardrailResult", "RunNode", "RunGraph", "ContextPack", "AgentTask") },
            @{ stage = "stage-2-trace-tree-rungraph"; files = @("apps/server/core/AIOrchestrator/harness/contracts.py", "apps/server/core/AIOrchestrator/api/agent_router.py"); tokens = @("RunGraph", "graph") },
            @{ stage = "stage-3-tools-permissions"; files = @("apps/server/core/AIOrchestrator/harness/contracts.py", "apps/server/core/AIOrchestrator/harness/execution/tool_executor.py"); tokens = @("ToolSpec", "permission", "requires_confirmation") },
            @{ stage = "stage-4-guardrails"; files = @("apps/server/core/AIOrchestrator/harness/guardrails/service.py"); tokens = @("GuardrailResult") },
            @{ stage = "stage-5-contextpack-memory"; files = @("apps/server/core/AIOrchestrator/harness/contracts.py"); tokens = @("ContextPack") },
            @{ stage = "stage-6-agent-tasks-durable-execution"; files = @("apps/server/core/AIOrchestrator/harness/runtime/task_service.py", "apps/client/desktop/MemoChat-qml/qml/agent/AgentTaskPane.qml"); tokens = @("AgentTask") },
            @{ stage = "stage-7-replay-evals"; files = @("apps/server/core/AIOrchestrator/harness/evals/service.py", "apps/server/core/AIOrchestrator/api/agent_router.py"); tokens = @("eval") },
            @{ stage = "stage-8-agent-spec-templates"; files = @("apps/server/core/AIOrchestrator/harness/skills/specs.py"); tokens = @("AgentSpec") },
            @{ stage = "stage-9-multi-agent-handoffs"; files = @("apps/server/core/AIOrchestrator/harness/handoffs/service.py"); tokens = @("AgentMessage", "HandoffStep", "AgentFlow") },
            @{ stage = "stage-10-a2a-ready-interop"; files = @("apps/server/core/AIOrchestrator/harness/interop/service.py"); tokens = @("AgentCard", "RemoteAgentRef") }
        )

        $roadmapMismatches = @()
        foreach ($expectation in $expectations) {
            if ($completedIds -notcontains $expectation.stage) {
                continue
            }

            $missingFiles = @()
            foreach ($relFile in $expectation.files) {
                $absFile = Join-Path $root $relFile
                if (-not (Test-Path $absFile)) {
                    $missingFiles += $relFile
                }
            }

            $missingTokens = @()
            foreach ($token in $expectation.tokens) {
                $found = $false
                foreach ($relFile in $expectation.files) {
                    $absFile = Join-Path $root $relFile
                    if (Test-FileContains -Path $absFile -Pattern ([regex]::Escape($token))) {
                        $found = $true
                        break
                    }
                }
                if (-not $found) {
                    $missingTokens += $token
                }
            }

            if ($missingFiles.Count -gt 0 -or $missingTokens.Count -gt 0) {
                $roadmapMismatches += [ordered]@{
                    stage = $expectation.stage
                    missing_files = $missingFiles
                    missing_tokens = $missingTokens
                }
            }
        }

        if ($roadmapMismatches.Count -eq 0) {
            Add-GateCheck -Name "roadmap_source_consistency" -Category "roadmap" -Status "pass" -Summary "Completed roadmap stages have matching source evidence"
        } else {
            Add-GateCheck -Name "roadmap_source_consistency" -Category "roadmap" -Status "fail" -Summary "Roadmap claims completed work that does not have matching source evidence" -Data @{
                mismatches = $roadmapMismatches
            }
        }
    } catch {
        Add-GateCheck -Name "agent_roadmap_manifest" -Category "roadmap" -Status "fail" -Summary "Could not parse roadmap manifest" -Detail $_.Exception.Message
    }
}

$shouldRunTests = $RunOfflineTests.IsPresent -or $Mode -eq "Full"
if (-not $shouldRunTests) {
    Add-GateCheck -Name "ai_orchestrator_offline_tests" -Category "tests" -Status "skip" -Summary "Offline tests skipped in quick mode"
} else {
    $aiRoot = Join-Path $root "apps/server/core/AIOrchestrator"
    $venvPython = Join-Path $aiRoot ".venv/Scripts/python.exe"
    $pythonExe = if (Test-Path $venvPython) { $venvPython } elseif (Test-CommandAvailable -Name "python") { "python" } else { "" }
    if (-not $pythonExe) {
        Add-GateCheck -Name "ai_orchestrator_offline_tests" -Category "tests" -Status "block" -Summary "Python is unavailable for offline tests"
    } else {
        Push-Location $aiRoot
        try {
            $previousErrorActionPreference = $ErrorActionPreference
            $ErrorActionPreference = "Continue"
            $testOutput = & $pythonExe -m unittest tests.test_ollama_recovery tests.test_harness_structure 2>&1
            $testExit = $LASTEXITCODE
            $ErrorActionPreference = $previousErrorActionPreference
            if ($testExit -eq 0) {
                Add-GateCheck -Name "ai_orchestrator_offline_tests" -Category "tests" -Status "pass" -Summary "AIOrchestrator offline tests passed" -Detail (($testOutput | Select-Object -Last 20) -join "`n")
            } else {
                Add-GateCheck -Name "ai_orchestrator_offline_tests" -Category "tests" -Status "fail" -Summary "AIOrchestrator offline tests failed" -Detail (($testOutput | Select-Object -Last 80) -join "`n")
            }
        } catch {
            $ErrorActionPreference = "Stop"
            Add-GateCheck -Name "ai_orchestrator_offline_tests" -Category "tests" -Status "fail" -Summary "AIOrchestrator offline tests raised an exception" -Detail $_.Exception.Message
        } finally {
            $ErrorActionPreference = "Stop"
            Pop-Location
        }
    }
}

$blockedCount = @($script:Checks | Where-Object { $_.status -eq "block" }).Count
$failedCount = @($script:Checks | Where-Object { $_.status -eq "fail" }).Count
$warnCount = @($script:Checks | Where-Object { $_.status -eq "warn" }).Count
$passCount = @($script:Checks | Where-Object { $_.status -eq "pass" }).Count
$skipCount = @($script:Checks | Where-Object { $_.status -eq "skip" }).Count

$overallStatus = if ($blockedCount -gt 0) {
    "blocked"
} elseif ($failedCount -gt 0) {
    "dirty"
} else {
    "clean"
}

$recommendations = New-Object System.Collections.Generic.List[string]
if ($failedCount -gt 0) {
    $recommendations.Add("Fix all fail checks before treating harness work as complete.") | Out-Null
}
if ($blockedCount -gt 0) {
    $recommendations.Add("Resolve block checks first; they prevent reliable assessment.") | Out-Null
}
if (@($script:Checks | Where-Object { $_.name -eq "roadmap_source_consistency" -and $_.status -eq "fail" }).Count -gt 0) {
    $recommendations.Add("Either implement the missing roadmap stages or mark those stages incomplete until source evidence exists.") | Out-Null
}
if (@($script:Checks | Where-Object { $_.name -eq "feedback_loop_activity" -and $_.status -eq "warn" }).Count -gt 0) {
    $recommendations.Add("Submit at least one real or scripted agent feedback record to prove the feedback loop is active.") | Out-Null
}
if (@($script:Checks | Where-Object { $_.name -eq "prometheus_all_targets" -and $_.status -in @("warn", "fail") }).Count -gt 0) {
    $recommendations.Add("Repair or intentionally scope Prometheus targets so observability has a clean source of truth.") | Out-Null
}

$checkArray = @()
foreach ($check in $script:Checks) {
    $checkArray += $check
}

$recommendationArray = @()
foreach ($recommendation in $recommendations) {
    $recommendationArray += $recommendation
}

$result = [ordered]@{
    status = $overallStatus
    mode = $Mode
    started_at = $startedAt
    finished_at = (Get-Date).ToUniversalTime().ToString("o")
    repo_root = $root
    summary = [ordered]@{
        pass = $passCount
        warn = $warnCount
        fail = $failedCount
        block = $blockedCount
        skip = $skipCount
    }
    checks = $checkArray
    recommendations = $recommendationArray
}

$outDir = Split-Path -Parent $OutputJson
if ($outDir -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$json = $result | ConvertTo-Json -Depth 20
Set-Content -Path $OutputJson -Value $json -Encoding UTF8
Write-Output $json

if ($overallStatus -eq "clean") {
    exit 0
}
if ($overallStatus -eq "blocked") {
    exit 2
}
exit 1
