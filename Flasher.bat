:: PLATFORM is IDF or Arduino
set PLATFORM=IDF
set EXAMPLE=PollingExample1_PositionVelocityTime

:: First arg: PLATFORM. Second arg: EXAMPLE. Third: COMPORT. All are optional
if not [%1]==[] set PLATFORM=%1
if not [%2]==[] set EXAMPLE=%2

if [%3]==[] goto findPort

set COMPORT=%3
goto program

:findPort

for /f "delims=" %%A in ('powershell -Command "Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM[0-9]+\)' } | Where-Object { $_.Name -match 'CH340' } | ForEach-Object { if ($_ -match '\(COM[0-9]+\)') { $matches[0].Trim('()') } }"') do (
    set COMPORT=%%A
)

:program

set SUFFIX=
if %PLATFORM%==Arduino set SUFFIX=.ino

set BOOTAPP=
if %PLATFORM%==Arduino set BOOTAPP=0xe000 %PLATFORM%_%EXAMPLE%%SUFFIX%.boot_app0.bin

:: Replace "python -m esptool" with "esptool.exe" if needed
python -m esptool --chip esp32 -p %COMPORT% -b 460800^
 --before default-reset^
 --after hard-reset^
 write-flash -z^
 --flash-mode dio^
 --flash-size detect^
 --flash-freq 80m^
 0x1000 %PLATFORM%_%EXAMPLE%%SUFFIX%.bootloader.bin^
 0x8000 %PLATFORM%_%EXAMPLE%%SUFFIX%.partitions.bin^
 %BOOTAPP%^
 0x10000 %PLATFORM%_%EXAMPLE%%SUFFIX%.bin
