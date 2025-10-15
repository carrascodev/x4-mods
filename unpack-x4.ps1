
param(
    [string]$game_dir = "D:\Games\X4 Foundations"
)

Write-Host "Unpacking X4 files..." -ForegroundColor Green
Write-Host "Game directory: $game_dir" -ForegroundColor Cyan

if (-Not (Test-Path $game_dir)) {
    Write-Host "Game directory not found: $game_dir" -ForegroundColor Red
    exit 1
}
# .\XCat\XRCatTool.exe -in $game_dir\01.cat -in $game_dir\02.cat -in $game_dir\03.cat -in $game_dir\04.cat -in $game_dir\05.cat -in $game_dir\06.cat -in $game_dir\07.cat -in $game_dir\08.cat -in $game_dir\09.cat -out .\unpacked
.\XCat\XRCatTool.exe -in $game_dir\ext_01.cat -in $game_dir\ext_02.cat -out .\unpacked_mods\vro