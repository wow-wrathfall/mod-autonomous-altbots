[CmdletBinding()]
param(
    [Parameter()]
    [string]$CoreRoot,

    [Parameter()]
    [switch]$CheckOnly,

    [Parameter()]
    [switch]$Reverse
)

$ErrorActionPreference = 'Stop'
$moduleRoot = Split-Path -Parent $PSScriptRoot

if (-not $CoreRoot) {
    $CoreRoot = [System.IO.Path]::GetFullPath((Join-Path $moduleRoot '..\..'))
}

$playerbotsRoot = Join-Path $CoreRoot 'modules\mod-playerbots'
$corePatch = Join-Path $moduleRoot 'patches\azerothcore-playerbot.patch'
$playerbotsPatch = Join-Path $moduleRoot 'patches\mod-playerbots.patch'

if (-not (Test-Path -LiteralPath (Join-Path $CoreRoot 'CMakeLists.txt'))) {
    throw "AzerothCore root not found at '$CoreRoot'. Pass -CoreRoot explicitly."
}

if (-not (Test-Path -LiteralPath (Join-Path $playerbotsRoot 'src\Bot\PlayerbotAI.cpp'))) {
    throw "mod-playerbots not found at '$playerbotsRoot'."
}

function Test-GitApply {
    param(
        [string]$Repository,
        [string]$Patch,
        [switch]$AsReverse
    )

    $arguments = @('-C', $Repository, 'apply')
    if ($AsReverse) {
        $arguments += '--reverse'
    }
    $arguments += @('--check', '--whitespace=error-all', $Patch)

    # A failed probe is expected while determining whether a patch is already applied. Windows PowerShell promotes
    # native stderr to an ErrorRecord when ErrorActionPreference is Stop, so suppress it only for this check.
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'SilentlyContinue'
    & git @arguments 2>$null | Out-Null
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference
    return $exitCode -eq 0
}

$targets = @(
    [pscustomobject]@{ Name = 'AzerothCore'; Repository = $CoreRoot; Patch = $corePatch },
    [pscustomobject]@{ Name = 'mod-playerbots'; Repository = $playerbotsRoot; Patch = $playerbotsPatch }
)

foreach ($target in $targets) {
    $canApply = Test-GitApply -Repository $target.Repository -Patch $target.Patch
    $canReverse = Test-GitApply -Repository $target.Repository -Patch $target.Patch -AsReverse

    if ($Reverse) {
        if (-not $canReverse -and -not $canApply) {
            throw "$($target.Name) patch is neither cleanly applied nor cleanly reversible. Resolve the source mismatch manually."
        }
        $target | Add-Member -NotePropertyName Action -NotePropertyValue $(if ($canReverse) { 'reverse' } else { 'skip' })
    }
    else {
        if (-not $canApply -and -not $canReverse) {
            throw "$($target.Name) patch does not match this revision. Do not force it; refresh the patch for the new upstream source."
        }
        $target | Add-Member -NotePropertyName Action -NotePropertyValue $(if ($canApply) { 'apply' } else { 'skip' })
    }
}

foreach ($target in $targets) {
    if ($target.Action -eq 'skip') {
        $state = if ($Reverse) { 'already absent' } else { 'already applied' }
        Write-Host "$($target.Name): $state"
        continue
    }

    $verb = if ($Reverse) { 'reverse' } else { 'apply' }
    if ($CheckOnly) {
        Write-Host "$($target.Name): patch can $verb cleanly"
        continue
    }

    $arguments = @('-C', $target.Repository, 'apply', '--whitespace=error-all')
    if ($Reverse) {
        $arguments += '--reverse'
    }
    $arguments += $target.Patch

    & git @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to $verb the $($target.Name) patch."
    }
    $pastTense = if ($Reverse) { 'reversed' } else { 'applied' }
    Write-Host "$($target.Name): patch $pastTense"
}

if (-not $Reverse) {
    Write-Host ''
    Write-Host 'Next: set EnablePlayerSettings = 1 and AiPlayerbot.DisabledWithoutRealPlayer = 0, then rebuild.'
}
