REM no GUI, so GuiModule excluded from build (se se_scope for GUI example)
REM Boland 5.5 don't support templates well enough to compile SDK Version 3


cls

del *.obj

del module.dll

bcc32 -I..\SE_SDK3;c:\borland\bcc55\include -DBORLAND_FREE=true -DWIN32=1 -u- -VC -VM -tWD -O -O2 -P -c gain.cpp

bcc32 -I..\SE_SDK3;c:\borland\bcc55\include -DBORLAND_FREE=true -DWIN32=1 -VC -VM -tWD -O -O2 -P -c ..\se_sdk3\MP_SDK_Audio.cpp ..\se_sdk3\MP_SDK_common.cpp

ilink32 -v- -Tpd c0d32 SEModuleMain Module SEPin SEModule_Base , Module, Module, kernel32.lib user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid cw32 import32, bc_module

copy Module.dll,"C:\Program Files\SynthEdit\modules\MyModules\se_gain.sem"


