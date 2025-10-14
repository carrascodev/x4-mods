
Write-Host "Unpacking X4 files..." -ForegroundColor Green
$game_dir = "D:\Games\X4 Foundations"
.\XCat\XRCatTool.exe -in $game_dir\01.cat -in $game_dir\02.cat -in $game_dir\03.cat -in $game_dir\04.cat -in $game_dir\05.cat -in $game_dir\06.cat -in $game_dir\07.cat -in $game_dir\08.cat -in $game_dir\09.cat -out .\unpacked