#!/bin/bash

WD="${WD:-$(pwd)}"

install_prefix="${install_prefix:-$WD/build/install}"

echo "install_prefix set to $install_prefix."

# exit on error
set -e

export MAKEFLAGS=-j$(nproc)

cmake -S . -B $WD/build -DCMAKE_INSTALL_PREFIX="$install_prefix" -DDEFAULT_APP_DATA_PATH="$WD/data" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build $WD/build
