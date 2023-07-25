@ECHO OFF
for %%F in (%0) do set bin_dir=%%~dpF
set OPCODE6DIR64 = "%bin_dir%\csound_plugins"
set AUDIO_DRIVER = "pa"
%bin_dir%\Justly.exe %*