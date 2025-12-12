@echo off
setlocal

:: --- Config ---
set DRIVE=%1
set OUTPUT=%2
if "%DRIVE%"=="" set DRIVE=C
if "%OUTPUT%"=="" set OUTPUT=output.json

set ROOT=%~dp0
set DLL_SRC=%ROOT%MFTIndexer\x64\Release\MFTIndexer.dll
set CLI_DIR=%ROOT%MFTIndexerCLI
set CLI_BIN=%CLI_DIR%\bin\Release\net8.0

echo [INFO] Note: This script assumes the Native DLL has been built manually via Visual Studio.
echo [INFO] If you modified MFTIndexer.cpp, please rebuild the solution 'MFTIndexer.sln' in Release mode first.
echo.

:: --- Build .NET ---
echo [1/3] Build .NET project...
cd /d "%CLI_DIR%"
dotnet build -c Release
if errorlevel 1 (
    echo [ERROR] .NET Build failed.
    exit /b 1
)

:: --- Copy Native DLL ---
echo [2/3] Copying Native DLL...
if exist "%DLL_SRC%" (
    copy "%DLL_SRC%" "%CLI_BIN%\MFTIndexer.dll" /Y >nul
    echo [OK] DLL copied.
) else (
    echo [WARNING] Native DLL not found at %DLL_SRC%
    echo [WARNING] Please build the native C++ project first!
)

:: --- Run ---
echo [3/3] Running MFTIndexerCLI...
set EXE_PATH=%CLI_BIN%\MFTIndexerCLI.exe

if exist "%EXE_PATH%" (
    "%EXE_PATH%" -d %DRIVE% -o "%OUTPUT%"
) else (
    echo [ERROR] Executable not found at %EXE_PATH%
)

endlocal
