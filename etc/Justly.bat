@ECHO OFF
for %%F in (%0) do set dirname=%%~dpF
set OPCODE6DIR64 = "%dirname%\csound_plugins"
set rtaudio = "pa"
%dirname%\Justly.exe