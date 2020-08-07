@echo off 
del /q binaries
for /r %%i in (*.frag, *.vert) do %VK_SDK_PATH%/Bin/glslangValidator.exe -g -V %%i -o %%~dpibinaries\%%~nxi.spv
rem -g -V %%i -o %%~dpibinaries\%%~nxi.spv
pause