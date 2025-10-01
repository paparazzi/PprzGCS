#!/bin/bash

WD="${WD:-$(pwd)}"
build_libs="${1:-false}"

# try getting 2nd argument, then the first one
build_app="${2:-$1}"
build_app="${build_app:-false}"

install_prefix="${install_prefix:-$WD/build/install}"

# C++ standard version to use for all builds
CXX_STANDARD_VERSION=17

echo "install_prefix set to $install_prefix."
echo "Using C++ standard version: C++${CXX_STANDARD_VERSION}"

# exit on error
set -e

#export QT_DIR="/path/to/Qt/5.12.5/gcc_64/lib/cmake/Qt5"

export MAKEFLAGS=-j$(nproc)

if [ $build_libs = "libs" ]
then
    ivyqt_src=$WD/ext/IvyQt
    ivyqt_build=$WD/build/ext/IvyQt
    
    pprzlinkqt_src=$WD/ext/pprzlinkQt/
    pprzlinkqt_build=$WD/build/ext/pprzlinkQt
    
    cmake -S $ivyqt_src -B $ivyqt_build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$install_prefix -DCMAKE_CXX_STANDARD=$CXX_STANDARD_VERSION
    cmake --build $ivyqt_build
    cmake --install $ivyqt_build

    cmake -S $pprzlinkqt_src -B $pprzlinkqt_build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$install_prefix -DCMAKE_CXX_STANDARD=$CXX_STANDARD_VERSION

    cmake --build $pprzlinkqt_build
    cmake --install $pprzlinkqt_build
fi

if [ $build_app = "app" ]
then
    cmake -S . -B $WD/build/pprzgcs -DCMAKE_INSTALL_PREFIX="$install_prefix" -DDEFAULT_APP_DATA_PATH="$WD/data" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_STANDARD=$CXX_STANDARD_VERSION

    cmake --build $WD/build/pprzgcs
fi

