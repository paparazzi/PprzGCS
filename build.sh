#!/bin/bash

WD="${WD:-$(pwd)}"
build_proj="${build_proj:-false}"
build_libs="${1:-false}"

# try getting 2nd argument, then the first one
build_app="${2:-$1}"
build_app="${build_app:-false}"
BUILD_TARGET="${BUILD_TARGET:-INVALID}"


local_install=$WD/build/local_install

if [ $build_libs = "libs" ]
then

    tinymxl2_src=$WD/ext/tinyxml2
    tinymxl2_build=$WD/build/ext/tinymxl2

    pprzlink_src=$WD/ext/pprzlink/lib/v2.0/C++
    pprzlink_build=$WD/build/ext/pprzlink

    mkdir -p $tinymxl2_build
    mkdir -p $pprzlink_build
    mkdir -p $local_install

    cmake -S $tinymxl2_src -B $tinymxl2_build -DCMAKE_INSTALL_PREFIX="$local_install"
    cmake --build $tinymxl2_build -j$(nproc)
    cmake --build $tinymxl2_build --target install


    cmake -S $pprzlink_src -B $pprzlink_build -DCMAKE_PREFIX_PATH="$local_install" -DCMAKE_INSTALL_PREFIX="$local_install"
    cmake --build $pprzlink_build -j$(nproc)
    cmake --build $pprzlink_build --target install

    if [ $build_proj = "true" ]
    then
        PROJ_src=$WD/ext/PROJ
        PROJ_build=$WD/build/ext/PROJ
        mkdir -p $PROJ_build
        cmake -S $PROJ_src -B $PROJ_build -DPROJ_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$local_install"
        cmake --build $PROJ_build
        cmake --build $PROJ_build --target install
    fi

fi

if [ $build_app = "app" ]
then
    cmake -S . -B $WD/build -DCMAKE_PREFIX_PATH="$local_install" -DBUILD_TARGET=$BUILD_TARGET
    export MAKEFLAGS=-j$(nproc)
    cmake --build $WD/build
fi
