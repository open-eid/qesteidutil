@echo off

set target_dir=target

IF EXIST build GOTO :BuildExists
md build
cd build
cmake .. -G "NMake Makefiles" ^
         -DCMAKE_BUILD_TYPE=Release ^
         -DCMAKE_INSTALL_PREFIX=""
nmake
nmake install DESTDIR="%target_dir%"

goto :EOF

:BuildExists
echo Directory "build" already exists, quitting.
