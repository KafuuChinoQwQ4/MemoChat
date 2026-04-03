$ports = @(
    @{N='GateServer';H='127.0.0.1';P=8080},
    @{N='Status';H='127.0.0.1';P=50052},
    @{N='CS1-TCP';H='127.0.0.1';P=8090},
    @{N='CS1-QUIC';H='127.0.0.1';P=8191}
)
foreach($s in $ports) {
    $c = New-Object System.Net.Sockets.TcpClient
    $r = $c.BeginConnect($s.H, $s.P, $null, $null)
    $w = $r.AsyncWaitHandle.WaitOne(1000, $false)
    if ($w) {
        try {
            $c.EndConnect($r)
            Write-Host "[OK] $($s.N) ($($s.H):$($s.P))"
        } catch {
            Write-Host "[ERR] $($s.N): $($_.Exception.Message)"
        }
    } else {
        Write-Host "[TO] $($s.N)"
    }
    $c.Close()
}
