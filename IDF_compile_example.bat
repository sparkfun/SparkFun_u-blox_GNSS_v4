set COMPONENT=sparkfun_u-blox_gnss_v4
set EXAMPLE=PollingExample1_PositionVelocityTime

:: First arg: COMPONENT. Second arg: EXAMPLE. Both are optional
if not [%1]==[] set COMPONENT=%1
if not [%2]==[] set EXAMPLE=%2

::Uncomment "docker builder prune -f" below to clear the build cache
::docker builder prune -f
docker build -f IDF.Dockerfile -t idf_example_container --progress=plain --no-cache-filter deployment^
 --build-arg COMPONENT=%COMPONENT% --build-arg EXAMPLE=%EXAMPLE% .
docker create --name=idf_example idf_example_container:latest
docker cp idf_example:/%EXAMPLE%.bin IDF_%EXAMPLE%.bin
docker cp idf_example:/%EXAMPLE%.elf IDF_%EXAMPLE%.elf
docker cp idf_example:/bootloader.bin IDF_%EXAMPLE%.bootloader.bin
docker cp idf_example:/partition-table.bin IDF_%EXAMPLE%.partitions.bin
docker container rm idf_example
