@echo off
setlocal

:: --- Config ---
set DRIVE=%1
if "%DRIVE%"=="" (
    echo Erreur : Spécifie une lettre de lecteur (ex: build_and_run.bat C)
    exit /b 1
)

set ROOT=%~dp0
set DLL_SRC=%ROOT%MFTIndexer\x64\Release\MFTIndexer.dll
set DLL_DEST=%ROOT%MFTIndexerCLI\bin\Release\net8.0\MFTIndexer.dll
set CLI_DIR=%ROOT%MFTIndexerCLI

:: --- Build .NET ---
echo [1/4] Build du projet .NET...
cd /d "%CLI_DIR%"
dotnet build -c Release
if errorlevel 1 (
    echo Build .NET a échoué.
    exit /b 1
)

:: --- Copier DLL native ---
echo [2/4] Copie de la DLL native...
copy "%DLL_SRC%" "%DLL_DEST%" /Y >nul
if not exist "%DLL_DEST%" (
    echo Erreur : la DLL native n'a pas été copiée !
    exit /b 1
)

:: --- Exécution ---
echo [3/4] Exécution depuis le bon dossier...
cd /d "%CLI_DIR%\bin\Release\net8.0"
echo [4/4] Lancement de MFTIndexerCLI.exe sur %DRIVE%:
MFTIndexerCLI.exe %DRIVE%

endlocal
