#! /bin/bash
set -x
set -e

BUILD_DIR=$1
REPO_ROOT=$2

mkdir -p "$BUILD_DIR"
rm -rf "$BUILD_DIR/*"

# switch to build dir
pushd "$BUILD_DIR"

# configure build files with CMake
# we need to explicitly set the install prefix, as CMake's default is /usr/local for some reason...
cmake "$REPO_ROOT" -DCMAKE_INSTALL_PREFIX=/usr

# build project and install files into AppDir
cmake --build . -j $(nproc)
make install DESTDIR=AppDir

# now, build AppImage using linuxdeploy and linuxdeploy-plugin-qt
# download linuxdeploy and its Qt plugin
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

ln -s $(which qmake6) qmake
export PATH="$(pwd):$PATH"

# make them executable
chmod +x linuxdeploy*.AppImage

# initialize AppDir, bundle shared libraries for QtQuickApp, use Qt plugin to bundle additional resources, and build AppImage, all in one single command
./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage
