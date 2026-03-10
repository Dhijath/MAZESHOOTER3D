@echo off
echo Shift-JIS を UTF-8 BOM に変換します...
powershell -ExecutionPolicy Bypass -File "%~dp0convert_to_utf8bom.ps1"
echo.
echo 完了。このウィンドウを閉じてください。
pause
