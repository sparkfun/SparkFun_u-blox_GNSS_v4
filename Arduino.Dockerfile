FROM ubuntu:latest AS upstream

ARG DEBIAN_FRONTEND=noninteractive

ARG CORE_VERSION=3.3.11

# Get curl, python3 and git
RUN apt-get update \
    && apt-get install -y curl python3 python3-pip python3-venv git \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Avoid the externally managed environment constraint
RUN PYTHON_VER=$(ls /usr/lib | grep python3.) \
    && echo "Python version: ${PYTHON_VER}" \
    && rm /usr/lib/${PYTHON_VER}/EXTERNALLY-MANAGED

# Install Python dependencies - esptool needs pyserial
#RUN python3 -m pip install --upgrade pip && \
RUN pip install pyserial

# Setup Arduino CLI
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Start config file
RUN arduino-cli config init --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json,https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json

# Update core index
RUN arduino-cli core update-index

# Update library index
RUN arduino-cli lib update-index

# Install platform
RUN arduino-cli core install "esp32:esp32@${CORE_VERSION}"

# Enable external libs
RUN arduino-cli config set library.enable_unsafe_install true

# ===========================================================================================

# Copy source and build deployment image
FROM upstream AS deployment

# Put the ARGs here - so that changing them doesn't require upstream to be rebuilt

# The example to be compiled
ARG EXAMPLE=PollingExample1_PositionVelocityTime

#  The component name
ARG COMPONENT=sparkfun_u-blox_gnss_v4

# arduino-cli warnings: none default more all
ARG WARNINGS=default

# Create a folder for the library source
RUN cd /root \
    && mkdir Arduino \
    && cd Arduino \
    && mkdir libraries \
    && cd libraries \
    && mkdir SparkFun_u-blox_GNSS_v4 \
    && cd SparkFun_u-blox_GNSS_v4 \
    && mkdir src

# Copy the library source
COPY src /root/Arduino/libraries/SparkFun_u-blox_GNSS_v4/src

# Copy library.properties so arduino-cli recognizes the 1.5 (recursive) library
# format and searches src/ for headers. Without this file present at the
# library root, arduino-cli falls back to the legacy flat layout and never
# looks inside src/, causing "SparkFun_u-blox_GNSS_v4.h: No such file or directory".
COPY library.properties /root/Arduino/libraries/SparkFun_u-blox_GNSS_v4/library.properties

# Copy keywords.txt too (not required to compile, but keeps the library metadata complete)
COPY keywords.txt /root/Arduino/libraries/SparkFun_u-blox_GNSS_v4/keywords.txt

# Copy the example source file
COPY examples .

# Compile Sketch
RUN cd ${EXAMPLE} \
    && arduino-cli compile --fqbn "esp32:esp32:esp32" \
    --warnings ${WARNINGS} \
    ${EXAMPLE}.ino \
    --export-binaries

# ===========================================================================================

# Copy the compile output. List the files
FROM deployment AS output
COPY --from=deployment ${EXAMPLE}/build/esp32.esp32.esp32 /
CMD echo $(ls /*.*)
