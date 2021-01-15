
# A new Ground Control Station for Paparazzi UAV

What is Paparazzi? => See the Wiki: http://wiki.paparazziuav.org/wiki/Main_Page

Paparazzi has an historical GCS made with OCaml/GTK2. OCaml is not the most popular language out there, and support for some components of GTK2 has been dropped in last Ubuntu releases. That are th main reasons this GCS started to be develloped in a popular technology choice: C++/QT5.

This GCS aims at replacing the historical GCS in a near future.

## Some screenshots

![GCS clear](screenshots/GCS_clear.png)

**Map overlay with transparency**

![GCS map](screenshots/GCS_map.png)

**Settings and GPS widgets**

![GCS settings GPS](screenshots/GCS_settings_GPS.png)

**Flight Plan, Strip and "PFD" widgets**

![GCS FP strip PFD](screenshots/GCS_fp_strip_PFD.png)



## INSTALLATION


### dependencies
This software depend on Qt5.12 or later, tinyxml2, Ivy and boost libraries. Make sure they are installed on you system.

__WARNING__: Some path are still harcoded, this is work in progress. If you want to contribute on making the build process smoother (or anything else), you are very welcome!


### Fetch submodules:

 `git submodule update --init --recursive`
 

### Build the GCS

Set the `Qt5_DIR` environnement variable to the appropriate path. Example:

`export Qt5_DIR="$HOME/Qt/5.12.0/gcc_64/lib/cmake/Qt5"`

Run the build script from the root of the repository:

`./build.sh`

Now you can launch it:

`.build/PprzGCS`

