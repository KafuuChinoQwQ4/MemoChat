param(
    [string]$VarName = ""
)

if ([string]::IsNullOrWhiteSpace($VarName)) {
    exit 1
}

$value = [Environment]::GetEnvironmentVariable($VarName)
if ([string]::IsNullOrWhiteSpace($value)) {
    exit 1
}
exit 0
