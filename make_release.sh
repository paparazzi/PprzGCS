#!/bin/bash

pprzgcs=$GITHUB_WORKSPACE/misc/debian/pprzgcs

mv $pprzgcs/usr/bin/PprzGCS $pprzgcs/usr/bin/pprzgcs
strip --strip-unneeded $pprzgcs/usr/bin/pprzgcs

dpkg-deb --root-owner-group --build $pprzgcs

cp $pprzgcs.deb ./
echo "should be ok, let see what is here:"
ls -l

