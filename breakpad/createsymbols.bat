REM http://212.56.88.69/mozilla-central/toolkit/crashreporter/tools/win32/dump_syms_vc1600.exe
@echo off

dump_syms_vc1600.exe %1 > %~n1.sym

for /f "tokens=4,5" %%a in (%~n1.sym) do (
  mkdir symbols\%%b\%%a
  move %~n1.sym symbols\%%b\%%a\
  exit /b
)
