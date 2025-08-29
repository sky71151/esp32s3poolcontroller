@echo off
REM Batchbestand om PlatformIO build en Firebase update te automatiseren

REM Stap 1: Build met PlatformIO
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" run

REM Stap 2: Firmwareversie naar Firebase uploaden
python update_firebase_version_admin.py

pause
