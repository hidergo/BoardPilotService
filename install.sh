#!/bin/bash

# Creating a user variable for possible sudo
USER=$(who am i | awk '{print $1}')

SERVICE_PATH="/etc/systemd/system/hidergod.service"

#if [ -z "$USER" ]
#then
#    echo "Could not find user, ./bin and ./build directories ownerships may not be correct"
#else
#    echo "Running with user: $USER"
#fi

echo "Installing hidergod..."

echo "Building..."
/usr/bin/cmake --build ./build --config Release --target all --

if [ $? -eq 0 ]; then
    echo "OK"
else
    echo "Build failed"
    exit 1
fi

echo "Build finished. Installing program and service"

# Following needs sudo
if [[ $EUID -ne 0 ]]; then
    echo "[ERROR] This script requires root access! Run it with sudo to install!"
    exit 1
fi

# Move package in to /usr/bin
echo "Moving built package into /usr/bin/hidergodm..."
cp ./build/src/hidergodm /usr/bin/hidergodm
if [ $? -eq 0 ]; then
    echo "OK"
else
    echo "Failed to move hidergodm to /usr/bin/hidergodm"
    exit 1
fi

# Install service
cp ./misc/hidergod-service.service $SERVICE_PATH

echo "Installation finished, run \`sudo systemctl start hidergod\` or \`sudo service hidergod start\`"
echo "Enable the service at startup: \`sudo systemctl enable hidergod\`"

