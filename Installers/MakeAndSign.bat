@echo off
set PATH=C:\Program Files\NSIS;C:\Program Files (x86)\NSIS;%PATH%
makensis setup.nsi

:sign
setlocal ENABLEDELAYEDEXPANSION
for %%f in (*.p12) do (
  set /P password="Enter certificate password: "
  signtool.exe sign /f "%%f" /p "!password!" /t http://timestamp.digicert.com /d "Tread Marks" "out/Tread Marks Install.exe"
  goto :end
)

:end
pause
