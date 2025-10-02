
# Ground Control Station for Paparazzi UAV

The new GCS for [Paparazzi UAV](http://wiki.paparazziuav.org/wiki/Main_Page) in C++/QT5!

![GCS FP strip PFD](screenshots/screenshot.png)

It should be straightforward to use it if you are familiar with paparazzi.
Check the [documentation](https://docs.paparazziuav.org/PprzGCS/) for more details.

If you did not installed Paparazzi yet, see the [quick Paparazzi installation guide](https://paparazzi-uav.readthedocs.io/en/latest/quickstart/install.html).


## INSTALLATION

### From Paparazzi PPA

Add the paparazzi PPA:

`sudo add-apt-repository -y ppa:paparazzi-uav/ppa`

And install Pprzgcs!

`sudo apt-get install pprzgcs`

If you don't launch it from the paparazzi center, set the `PAPARAZZI_HOME` and `PAPARAZZI_SRC` environment variables.


### Using the AppImage

If you are not on Ubuntu 20.04 or later, or if you want the latest changes, the best is to use the AppImage.

Get the [latest release](https://github.com/paparazzi/PprzGCS/releases/latest), and give the AppImage the execution rights, then launch it.

To make it work with the paparazzi center, make a symbolic link named `pprzgcs` in one of you `PATH` directories.

The developer version may have new features or bug fixes not landed in releases yet. Open an issue to ask a developer to make a new release.


### Build from sources

#### Ubuntu 20.04 prerequisite

Install the dependencies: 

`sudo apt install extra-cmake-modules libsqlite3-dev libzip-dev qtbase5-dev libqt5svg5-dev libqt5texttospeech5-dev libproj-dev mesa-common-dev libglu1-mesa-dev`


#### Ubuntu 18.04 prerequisite

Install the dependencies:

`sudo apt install extra-cmake-modules libsqlite3-dev libzip-dev mesa-common-dev libglu1-mesa-dev`

__QT5__

Install [Qt5.12.0 or above](https://www.qt.io/download-open-source).

Setup in your .bashrc the environnement variable Qt5_DIR for cmake to find Qt5:

`export Qt5_DIR="/path/to/Qt/5.12.0/gcc_64/lib/cmake/Qt5"`


__PROJ 6.3.1__

From the PprzGCS root:

_Hint: speed up build by running `export MAKEFLAGS=-j$(nproc)` before building._

```
curl -L https://github.com/OSGeo/PROJ/releases/download/6.3.1/proj-6.3.1.tar.gz | tar -xz -C ext
cmake -S ext/proj-6.3.1/ -B build/ext/proj -DPROJ_TESTS=OFF -DCMAKE_INSTALL_PREFIX=build/install
cmake --build build/ext/proj
cmake --install build/ext/proj
```

### Common instructions


cd to the PprzGCS root, and get the submodules:

`git submodule update --init --recursive`

Then just run the build script: `./build.sh`

To be able to run it from the paparazzi center, you need to add the `build/pprzgcs` directory to your *$PATH*. Adapt this command and add it to your .bashrc:

`export PATH="/path/to/PprzGCS/build:$PATH"`

You should now be able to launch it from the Paparazzi center.

If you want to run it from the terminal, set the `PAPARAZZI_HOME` and the `PAPARAZZI_SRC` environment variables to your paparazzi path, then just run `./build/pprzgcs`


#### gRPC

Add `-DCMAKE_PREFIX_PATH=<path/to/gRPC> -DGRPC=ON` to build with gRPC.

See instructions to buils gRPC from source here: [https://grpc.io/docs/languages/cpp/quickstart/].

#### Mac

PprzGCS have been successfully built for Mac, although is not officially supported. You are on you own ;-)

More details are given in the [documentation](https://docs.paparazziuav.org/PprzGCS/)


## CONTRIBUTING

Project home repository is at [https://github.com/paparazzi/PprzGCS](https://github.com/paparazzi/PprzGCS).

Make pull requests here to contribute.

