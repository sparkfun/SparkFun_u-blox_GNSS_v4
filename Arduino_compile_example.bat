if [%1]==[] goto useDefault

set EXAMPLE=%1
goto dockerBuild

:useDefault

set EXAMPLE=PollingExample1_PositionVelocityTime

:dockerBuild

::Uncomment "docker builder prune -f" below to clear the build cache
::docker builder prune -f
docker build -f Arduino.Dockerfile -t arduino_example_container --progress=plain --no-cache-filter deployment^
 --build-arg EXAMPLE=%EXAMPLE% .
docker create --name=arduino_example arduino_example_container:latest
docker cp arduino_example:/%EXAMPLE%.ino.bin Arduino_%EXAMPLE%.ino.bin
docker cp arduino_example:/%EXAMPLE%.ino.elf Arduino_%EXAMPLE%.ino.elf
docker cp arduino_example:/%EXAMPLE%.ino.bootloader.bin Arduino_%EXAMPLE%.ino.bootloader.bin
docker cp arduino_example:/%EXAMPLE%.ino.partitions.bin Arduino_%EXAMPLE%.ino.partitions.bin
docker cp arduino_example:/boot_app0.bin Arduino_%EXAMPLE%.ino.boot_app0.bin
docker container rm arduino_example
