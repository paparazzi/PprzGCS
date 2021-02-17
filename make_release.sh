#!/bin/bash

local_install=$GITHUB_WORKSPACE/build/local_install

pprzgcs=$GITHUB_WORKSPACE/misc/debian/pprzgcs

bin=$GITHUB_WORKSPACE/misc/debian/pprzgcs/usr/bin
share=$GITHUB_WORKSPACE/misc/debian/pprzgcs/usr/share/pprzgcs
lib=$GITHUB_WORKSPACE/misc/debian/pprzgcs/usr/lib/pprzgcs
PprzGCS=$GITHUB_WORKSPACE/build/PprzGCS

mkdir -p $bin
mkdir -p $share
mkdir -p $lib

cp -r $GITHUB_WORKSPACE/data/* $share
cp  $PprzGCS $bin/pprzgcs
strip --strip-unneeded $bin/pprzgcs

path_pprzlink=$(ldd $PprzGCS | cut -d " " -f 3 | grep libpprzlink)
path_tinyxml2=$(ldd $PprzGCS | cut -d " " -f 3 | grep tinyxml2)

cp $path_pprzlink $lib
cp $path_tinyxml2 $lib
strip --strip-unneeded $lib/*

patchelf --set-rpath '$ORIGIN/../lib/pprzgcs/' $bin/pprzgcs

dpkg-deb --root-owner-group --build $pprzgcs

cp $pprzgcs.deb ./
echo "should be ok, let see what is here:"
ls -l 
