Param(
    [Parameter(Mandatory=$false)]
    [Switch] $clean,

    [Parameter(Mandatory=$false)]
    [Switch] $log,

    [Parameter(Mandatory=$false)]
    [Switch] $useDebug,

    [Parameter(Mandatory=$false)]
    [Switch] $self,

    [Parameter(Mandatory=$false)]
    [Switch] $all,

    [Parameter(Mandatory=$false)]
    [String] $custom="",

    [Parameter(Mandatory=$false)]
    [String] $file="",

    [Parameter(Mandatory=$false)]
    [Switch] $help
)

if ($help -eq $true) {
    Write-Output "`"Copy`" - Builds and copies your mod to your quest, and also starts Beat Saber with optional logging"
    Write-Output "`n-- Arguments --`n"

    Write-Output "-Clean `t`t Performs a clean build (equvilant to running `"build -clean`")"
    Write-Output "-UseDebug `t Copies the debug version of the mod to your quest"
    Write-Output "-Log `t`t Logs Beat Saber using the `"Start-Logging`" command"

    Write-Output "`n-- Logging Arguments --`n"

    & $PSScriptRoot/start-logging.ps1 -help -excludeHeader

    exit
}

& $PSScriptRoot/build.ps1 -clean:$clean -hotReload

if ($LASTEXITCODE -ne 0) {
    Write-Output "Failed to build, exiting..."
    exit $LASTEXITCODE
}

& $PSScriptRoot/validate-modjson.ps1
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
$modJson = Get-Content "./mod.json" -Raw | ConvertFrom-Json

foreach ($fileName in $modJson.modFiles) {
    if ($useDebug -eq $true) {
        & adb push build/debug/$fileName /sdcard/ModData/com.beatgames.beatsaber/Modloader/early_mods/$fileName
    } else {
        & adb push build/$fileName /sdcard/ModData/com.beatgames.beatsaber/Modloader/early_mods/$fileName
    }
}

foreach ($fileName in $modJson.lateModFiles) {
    if ($useDebug -eq $true) {
        & adb push build/debug/$fileName /sdcard/ModData/com.beatgames.beatsaber/Modloader/mods/$fileName
    } else {
        & adb push build/$fileName /sdcard/ModData/com.beatgames.beatsaber/Modloader/mods/$fileName
    }
}

if ($clean -eq $true) {
    & $PSScriptRoot/push-bsml.ps1 -clear
}
& $PSScriptRoot/push-bsml.ps1

if ($log -eq $true) {
    & $PSScriptRoot/start-logging.ps1 -self:$self -all:$all -custom:$custom -file:$file
}
