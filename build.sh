#!/bin/bash

WD="${WD:-$(pwd)}"
build_proj="${build_proj:-false}"
build_libs="${1:-false}"

# try getting 2nd argument, then the first one
build_app="${2:-$1}"
build_app="${build_app:-false}"
BUILD_TARGET="${BUILD_TARGET:-INVALID}"


local_install=$WD/build/local_install

# exit on error
set -e

if [ $build_libs = "libs" ]
then

    ivyqt_src=$WD/ext/IvyQt
    ivyqt_build=$WD/build/ext/IvyQt
    
    pprzlinkqt_src=$WD/ext/pprzlinkQt/
    pprzlinkqt_build=$WD/build/ext/pprzlinkQt
    
    cmake -S $ivyqt_src -B $ivyqt_build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$local_install
    cmake --build $ivyqt_build
    cmake --install $ivyqt_build

    cmake -S $pprzlinkqt_src -B $pprzlinkqt_build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$local_install
    cmake --build $pprzlinkqt_build
    cmake --install $pprzlinkqt_build

    if [ $build_proj = "true" ]
    then
        PROJ_src=$WD/ext/PROJ
        PROJ_build=$WD/build/ext/PROJ
        mkdir -p $PROJ_build
        cmake -S $PROJ_src -B $PROJ_build -DPROJ_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$local_install"
        cmake --build $PROJ_build
        cmake --install $PROJ_build
    fi

fi

if [ $build_app = "app" ]
then
    cmake -S . -B $WD/build -DCMAKE_PREFIX_PATH="$local_install" -DBUILD_TARGET=$BUILD_TARGET
    export MAKEFLAGS=-j$(nproc)
    cmake --build $WD/build
fi
