@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set VCPKG_VISUAL_STUDIO_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
cd /d C:\Users\casse\github\pzep\vcpkg
vcpkg install duktape:x64-windows-static-md
