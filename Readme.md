
# A new Ground Control Station for Paparazzi UAV

What is Paparazzi? => See the Wiki: http://wiki.paparazziuav.org/wiki/Main_Page


## INSTALLATION


### dependencies
This software depend on Qt5.12 or later, tinyxml2, Ivy and boost libraries. Make sure they are installed on you system.

__WARNING__: Some path are still harcoded, this is work in progress. If you want to contribute on making the build process smoother (or anything else), you are very welcome!


### Fetch submodules:

 `git submodule update --init --recursive`
 
### Build and install the PROJ libraries:

`cd ext/PROJ`

`mkdir build && cd build`

`cmake ..`

`make`

`sudo make install`

### Build the GCS

If you just build PROj, go back to the project root:

`cd ../../..`

`mkdir build && cd build`

`cmake ..`

`make`

Now you can launch it:

`./PprzGCS`

