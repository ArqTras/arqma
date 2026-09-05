@echo off
REM Companion stack for Windows (same repo, separate PIDs). Unix: utils/arqma-stack.sh
setlocal
set "BIN_DIR=%ARQMA_BIN_DIR%"
if "%BIN_DIR%"=="" set "BIN_DIR=%~dp0..\build\upgrade-test\bin"
set "STORAGE_LISTEN=%~1"
if "%STORAGE_LISTEN%"=="" set "STORAGE_LISTEN=127.0.0.1:22021"
set "ROUTER_LISTEN=%~2"
if "%ROUTER_LISTEN%"=="" set "ROUTER_LISTEN=127.0.0.1:1090"
set "STACK_DIR=%ARQMA_STACK_DIR%"
if "%STACK_DIR%"=="" set "STACK_DIR=%TEMP%\arqma-stack"

if not exist "%BIN_DIR%\arqma-storage.exe" (
  echo Build companions first, e.g.:
  echo   cmake --build build\upgrade-test --parallel --target arqma_storage arqma_router arqma_msg daemon
  exit /b 1
)

mkdir "%STACK_DIR%\storage" 2>nul
mkdir "%STACK_DIR%\router" 2>nul

start "arqma-storage" "%BIN_DIR%\arqma-storage.exe" --listen %STORAGE_LISTEN% --data-dir "%STACK_DIR%\storage"
start "arqma-router" "%BIN_DIR%\arqma-router.exe" --listen %ROUTER_LISTEN% --data-dir "%STACK_DIR%\router" --storage-url http://%STORAGE_LISTEN%

echo arqma-storage http://%STORAGE_LISTEN% data=%STACK_DIR%\storage
echo arqma-router  http://%ROUTER_LISTEN% data=%STACK_DIR%\router
echo.
echo Point the consensus daemon at the companions (still a separate process):
echo   "%BIN_DIR%\arqmad.exe" --storage-client-url=http://%STORAGE_LISTEN% --arq-router
echo.
echo Messenger CLI:
echo   "%BIN_DIR%\arqma-msg.exe" gen
echo   "%BIN_DIR%\arqma-msg.exe" send --url http://%STORAGE_LISTEN% --to ^<64-hex^> --text hello
echo   "%BIN_DIR%\arqma-msg.exe" send --router http://%ROUTER_LISTEN% --to ^<64-hex^> --text hello
echo   rem repeat --router (outermost first, max 3) for extra hops
echo Close the companion console windows to stop the stack.
endlocal
