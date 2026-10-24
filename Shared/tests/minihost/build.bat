@echo off
rem Builds minihost.exe with the VST3 SDK that JUCE carries. Pass the JUCE source folder of any plug-in build:
rem   build.bat "D:\RONE PLUGINS\rone-plugins\RoneThrow\build-vs2022\_deps\juce-src"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /O2 /utf-8 /I "%~1\modules\juce_audio_processors\format_types\VST3_SDK" "%~dp0minihost.cpp" /Fe:"%~dp0minihost.exe" /link user32.lib ole32.lib gdi32.lib /MANIFEST:NO
