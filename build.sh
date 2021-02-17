#!/bin/bash

wd=$(pwd)

git submodule update --recursive

tinymxl2_src=$wd/ext/tinyxml2
tinymxl2_build=$wd/build/ext/tinymxl2

pprzlink_src=$wd/ext/pprzlink/lib/v2.0/C++
pprzlink_build=$wd/build/ext/pprzlink

PROJ_src=$wd/ext/PROJ
PROJ_build=$wd/build/ext/PROJ

local_install=$wd/build/local_install

mkdir -p $tinymxl2_build
mkdir -p $pprzlink_build
mkdir -p $PROJ_build
mkdir -p $local_install

cmake -S $tinymxl2_src -B $tinymxl2_build -DCMAKE_INSTALL_PREFIX="$local_install"
cmake --build $tinymxl2_build
cmake --build $tinymxl2_build --target install


cmake -S $pprzlink_src -B $pprzlink_build -DCMAKE_INSTALL_PREFIX="$local_install" -DCMAKE_PREFIX_PATH="$local_install"
cmake --build $pprzlink_build
cmake --build $pprzlink_build --target install

cmake -S $PROJ_src -B $PROJ_build -DPROJ_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$local_install"
cmake --build $PROJ_build
cmake --build $PROJ_build --target install

cmake -S . -B build -DCMAKE_PREFIX_PATH="$local_install"
cmake --build build

