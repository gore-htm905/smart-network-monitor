# Integration Test: Real-Time Node Failure, Packet Rerouting & Shortest Path

$api = "http://localhost:8085/api"
$passed = 0
$failed = 0

function Assert-Test($condition, $name) {
    if ($condition) {
        Write-Host " [PASS] $name" -ForegroundColor Green
        $global:passed++
    } else {
        Write-Host " [FAIL] $name" -ForegroundColor Red
        $global:failed++
    }
}

Write-Host "--- Test 1: Reset Network Simulation ---"
Invoke-RestMethod -Uri "$api/simulation/reset" -Method Post | Out-Null
Start-Sleep -Milliseconds 500
$state = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($state.totalPackets -eq 0) "Network reset clean (0 packets)"

Write-Host "`n--- Test 2: Generate Packet with Online Path ---"
$gen = Invoke-RestMethod -Uri "$api/packets/generate" -Method Post -Body '{"source":"N1","destination":"N3","priority":2,"size":512}' -ContentType "application/json"
$pktId = $gen.packetId
Assert-Test ($pktId -ne $null -and $pktId.StartsWith("P")) "Packet $pktId generated"

$search = Invoke-RestMethod -Uri "$api/packets/search?id=$pktId"
$origRoute = $search.route
Write-Host "  Initial route: $origRoute"
Assert-Test ($origRoute.Length -gt 0) "Route computed to destination"

Write-Host "`n--- Test 3: Failure of Intermediate Router & Rerouting ---"
# Pick the first intermediate router in route
$routeParts = $search.routeArr
$intermediateRouter = $null
foreach ($hop in $routeParts) {
    if ($hop.StartsWith("R")) {
        $intermediateRouter = $hop
        break
    }
}

if ($intermediateRouter) {
    Write-Host "  Failing router $intermediateRouter..."
    Invoke-RestMethod -Uri "$api/network/failure" -Method Post -Body (@{type="node"; nodeId=$intermediateRouter; active=$false} | ConvertTo-Json) -ContentType "application/json" | Out-Null
    
    # Check router status
    $topo = Invoke-RestMethod -Uri "$api/network/topology"
    $rNode = $topo.nodes | Where-Object { $_.id -eq $intermediateRouter }
    Assert-Test ($rNode.status -eq "Inactive") "Router $intermediateRouter is now Inactive"

    # Start simulation and allow reroute to trigger
    Invoke-RestMethod -Uri "$api/simulation/start" -Method Post | Out-Null
    Start-Sleep -Seconds 2

    $after = Invoke-RestMethod -Uri "$api/packets/search?id=$pktId"
    Write-Host "  New status: $($after.status)"
    Write-Host "  New route: $($after.route)"
    Write-Host "  Reroute count: $($after.rerouteCount)"
    
    # Verify packet was rerouted or avoided the offline router
    $containsOffline = $after.routeArr -contains $intermediateRouter
    Assert-Test (-not $containsOffline) "Updated route completely bypasses failed router $intermediateRouter"
} else {
    Write-Host "  No router found in route, skipping hop failure"
}

Write-Host "`n--- Test 4: Generation to Offline Destination Fails Gracefully ---"
# Take destination N4 offline
Invoke-RestMethod -Uri "$api/network/failure" -Method Post -Body (@{type="node"; nodeId="N4"; active=$false} | ConvertTo-Json) -ContentType "application/json" | Out-Null
$genOffline = Invoke-RestMethod -Uri "$api/packets/generate" -Method Post -Body '{"source":"N1","destination":"N4","priority":1,"size":256}' -ContentType "application/json"
$offlinePktId = $genOffline.packetId
$offlinePkt = Invoke-RestMethod -Uri "$api/packets/search?id=$offlinePktId"
Assert-Test ($offlinePkt.status -eq "Dropped") "Packet to offline destination dropped immediately"
Assert-Test ($offlinePkt.dropReason.Length -gt 0) "Packet has drop reason: $($offlinePkt.dropReason)"

Write-Host "`n--- Test 5: Re-enabling Router Restores Paths ---"
if ($intermediateRouter) {
    Invoke-RestMethod -Uri "$api/network/failure" -Method Post -Body (@{type="node"; nodeId=$intermediateRouter; active=$true} | ConvertTo-Json) -ContentType "application/json" | Out-Null
    $topo2 = Invoke-RestMethod -Uri "$api/network/topology"
    $rNode2 = $topo2.nodes | Where-Object { $_.id -eq $intermediateRouter }
    Assert-Test ($rNode2.status -eq "Active") "Router $intermediateRouter restored to Active"
}

Write-Host "`n=========================================="
Write-Host "Test Results: $passed Passed, $failed Failed"
Write-Host "=========================================="
