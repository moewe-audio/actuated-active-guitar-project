#!/bin/bash

START_DIR=$PWD
LIBDAISY_DIR=$PWD/lib/libDaisy
DAISYSP_DIR=$PWD/lib/DaisySP

# Function to build a library
build_lib() {
    local DIR="$1"
    local NAME="$2"
    echo "building $NAME . . ."
    cd "$DIR" || { echo "Failed to cd to $DIR"; exit 1; }
    make -s clean
    make -j -s
    if [ $? -ne 0 ]; then
        echo "Failed to compile $NAME"
        exit 1
    fi
    echo "done."
}

# Build the libraries
build_lib "$LIBDAISY_DIR" "libDaisy"
build_lib "$DAISYSP_DIR" "DaisySP"

# Return to the project root
cd "$START_DIR" || { echo "Failed to return to $START_DIR"; exit 1; }

echo "cleaning project . . ."
make -s clean
if [ $? -ne 0 ]; then
    echo "Failed to clean project"
    exit 1
fi

echo "building project . . ."
make -j -s
if [ $? -ne 0 ]; then
    echo "Failed to build project"
    exit 1
fi

echo "flashing program-dfu . . ."
make program-dfu
if [ $? -ne 0 ]; then
    echo "Failed to flash program-dfu"
    exit 1
fi

echo "all done."
