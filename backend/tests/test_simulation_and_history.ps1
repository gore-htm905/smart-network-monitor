# Integration Test Suite for Simulation Controls, Search/Sort, and Complete Route History

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

Write-Host "=========================================================="
Write-Host "TEST 1: Issue 1 - Packet Generation Separate from Start"
Write-Host "=========================================================="
Invoke-RestMethod -Uri "$api/network/generate" -Method Post -Body '{"endpoints":10,"routers":4,"packets":0,"queueCapacity":20}' -ContentType "application/json" | Out-Null
$stateAfterGen = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateAfterGen.simulationState -eq "STOPPED") "State after network generation is STOPPED"
Assert-Test ($stateAfterGen.isRunning -eq $false) "isRunning is false after network generation"

# Generate 20 packets
$bulk = Invoke-RestMethod -Uri "$api/packets/bulk-generate" -Method Post -Body '{"count":20}' -ContentType "application/json"
Assert-Test ($bulk.count -eq 20) "20 packets generated via bulk-generate"

$stateAfterPackets = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateAfterPackets.simulationState -eq "STOPPED") "Simulation remains STOPPED after bulk packet generation"
Assert-Test ($stateAfterPackets.isRunning -eq $false) "isRunning remains false after packet generation"
Assert-Test ($stateAfterPackets.tickCount -eq 0) "Tick count has NOT advanced (tickCount = 0)"

$allPkts = (Invoke-RestMethod -Uri "$api/packets/all").packets
$allQueued = ($allPkts | Where-Object { $_.status -eq "Queued" }).Count
Assert-Test ($allQueued -eq 20) "All 20 packets are Queued at source and have not departed"

Write-Host "`n=========================================================="
Write-Host "TEST 2: Issue 2 - Start, Pause, Resume, and Reset Controls"
Write-Host "=========================================================="
# START
Invoke-RestMethod -Uri "$api/simulation/start" -Method Post | Out-Null
$stateRunning = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateRunning.simulationState -eq "RUNNING") "Simulation state is RUNNING after START"
Assert-Test ($stateRunning.isRunning -eq $true) "isRunning is true"

Start-Sleep -Milliseconds 1200
$stateAdvanced = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateAdvanced.tickCount -gt 0) "Simulation ticks advancing (tickCount = $($stateAdvanced.tickCount))"

# PAUSE
Invoke-RestMethod -Uri "$api/simulation/pause" -Method Post | Out-Null
$statePaused = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($statePaused.simulationState -eq "PAUSED") "Simulation state is PAUSED"
Assert-Test ($statePaused.isPaused -eq $true) "isPaused is true"
Assert-Test ($statePaused.isRunning -eq $false) "isRunning is false while paused"

$pausedTick = $statePaused.tickCount
Start-Sleep -Milliseconds 1200
$stateStillPaused = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateStillPaused.tickCount -eq $pausedTick) "Tick count frozen while paused ($pausedTick == $($stateStillPaused.tickCount))"

# RESUME
Invoke-RestMethod -Uri "$api/simulation/resume" -Method Post | Out-Null
$stateResumed = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateResumed.simulationState -eq "RUNNING") "Simulation state is RUNNING after RESUME"
Assert-Test ($stateResumed.isRunning -eq $true) "isRunning is true after resume"

Start-Sleep -Milliseconds 1200
$stateResumedAdv = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateResumedAdv.tickCount -gt $pausedTick) "Simulation continues advancing after resume (from $pausedTick to $($stateResumedAdv.tickCount))"

# RESET
Invoke-RestMethod -Uri "$api/simulation/reset" -Method Post | Out-Null
$stateReset = Invoke-RestMethod -Uri "$api/network/state"
Assert-Test ($stateReset.simulationState -eq "STOPPED") "Simulation state reset to STOPPED"
Assert-Test ($stateReset.isRunning -eq $false) "isRunning is false after reset"
Assert-Test ($stateReset.tickCount -eq 0) "Tick count reset to 0"
Assert-Test ($stateReset.totalPackets -eq 0) "Total packets cleared to 0 on reset"

Write-Host "`n=========================================================="
Write-Host "TEST 3: Issue 3 - Search by Packet ID"
Write-Host "=========================================================="
# Generate 15 packets
$bulk2 = Invoke-RestMethod -Uri "$api/packets/bulk-generate" -Method Post -Body '{"count":15}' -ContentType "application/json"
$targetId = $bulk2.packetIds[4] # e.g. P105
Write-Host "  Searching for target: $targetId"

# Exact search
$searchExact = Invoke-RestMethod -Uri "$api/packets/search?id=$targetId"
Assert-Test ($searchExact.id -eq $targetId) "Exact search matches $targetId"

# Lowercase search
$targetLower = $targetId.ToLower()
$searchLower = Invoke-RestMethod -Uri "$api/packets/search?id=$targetLower"
Assert-Test ($searchLower.id -eq $targetId) "Case-insensitive search ($targetLower) matches $targetId"

# Search with leading/trailing whitespace
$targetWhitespace = "  $targetId  "
$searchSpace = Invoke-RestMethod -Uri "$api/packets/search?id=$targetWhitespace"
Assert-Test ($searchSpace.id -eq $targetId) "Whitespace trimmed search matches $targetId"

# Search for nonexistent ID
$notFound = $false
try {
    Invoke-RestMethod -Uri "$api/packets/search?id=P99999" -ErrorAction Stop
} catch {
    $notFound = $true
}
Assert-Test ($notFound -eq $true) "Nonexistent ID returns error (not found)"

Write-Host "`n=========================================================="
Write-Host "TEST 4: Issue 4 - Sorting by Real Parameters"
Write-Host "=========================================================="
# Sort by ID ascending (natural ordering)
$sortIdAsc = (Invoke-RestMethod -Uri "$api/packets/sort?by=id&order=asc").packets
$id1 = [int]($sortIdAsc[0].id -replace '\D')
$id2 = [int]($sortIdAsc[1].id -replace '\D')
$idLast = [int]($sortIdAsc[-1].id -replace '\D')
Assert-Test ($id1 -le $id2 -and $id2 -le $idLast) "Natural numeric sort by ID ascending ($id1 <= $id2 <= $idLast)"

# Sort by size descending
$sortSizeDesc = (Invoke-RestMethod -Uri "$api/packets/sort?by=size&order=desc").packets
$s1 = $sortSizeDesc[0].size
$s2 = $sortSizeDesc[1].size
$sLast = $sortSizeDesc[-1].size
Assert-Test ($s1 -ge $s2 -and $s2 -ge $sLast) "Sort by size descending ($s1 >= $s2 >= $sLast)"

# Sort by priority descending
$sortPrioDesc = (Invoke-RestMethod -Uri "$api/packets/sort?by=priority&order=desc").packets
$p1 = $sortPrioDesc[0].priorityInt
$pLast = $sortPrioDesc[-1].priorityInt
Assert-Test ($p1 -ge $pLast) "Sort by priority descending ($p1 >= $pLast)"

Write-Host "`n=========================================================="
Write-Host "TEST 5: Issue 5 - Show All Packet Route Stacks"
Write-Host "=========================================================="
# Query datastructures stacks count
$ds = Invoke-RestMethod -Uri "$api/datastructures"
$stacks = $ds.stacks
$stackCount = ($stacks.psobject.Properties | Measure-Object).Count
Write-Host "  Retrieved $stackCount route stacks for 15 generated packets"
Assert-Test ($stackCount -eq 15) "All 15 packet route stacks returned without truncation (count = $stackCount, not capped at 8)"

Write-Host "`n=========================================================="
Write-Host "TEST 6: Issue 6 - Complete Route History for Every Packet"
Write-Host "=========================================================="
$testPkt = (Invoke-RestMethod -Uri "$api/packets/all").packets[0]
Assert-Test ($testPkt.initialRoute.Length -gt 0) "Packet has initialRoute recorded: $($testPkt.initialRoute)"
Assert-Test ($testPkt.traversedNodes.Count -ge 1) "Packet has traversedNodes recorded (starts at source $($testPkt.traversedNodes[0]))"

# Test failed packet history: fail destination N5 and generate packet to N5
Invoke-RestMethod -Uri "$api/network/failure" -Method Post -Body '{"type":"node","nodeId":"N5","active":false}' -ContentType "application/json" | Out-Null
$failGen = Invoke-RestMethod -Uri "$api/packets/generate" -Method Post -Body '{"source":"N1","destination":"N5","priority":1,"size":128}' -ContentType "application/json"
$failPkt = Invoke-RestMethod -Uri "$api/packets/search?id=$($failGen.packetId)"
Assert-Test ($failPkt.status -eq "Dropped") "Packet to offline node is Dropped"
Assert-Test ($failPkt.dropReason.Length -gt 0) "Drop reason preserved: $($failPkt.dropReason)"
Assert-Test ($failPkt.lastValidNode.Length -gt 0) "Last valid node recorded: $($failPkt.lastValidNode)"

Write-Host "`n=========================================================="
Write-Host "SUMMARY: $passed Passed, $failed Failed"
Write-Host "=========================================================="
if ($failed -eq 0) {
    Write-Host "ALL TESTS PASSED SUCCESSFULLY!" -ForegroundColor Green
} else {
    Write-Host "SOME TESTS FAILED!" -ForegroundColor Red
}
