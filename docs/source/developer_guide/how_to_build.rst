.. developer_guide how_to_build

How to build from sources
=========================

Ubuntu 22.04 and 24.04
______________________

Clone the repository with its submodules, and run the build script:

.. code-block:: bash

    git clone --recursive https://github.com/paparazzi/PprzGCS.git
    cd PprzGCS
    ./build.sh

``build.sh`` first checks what the build needs: it fetches the submodules if they are missing, and
offers to install the missing packages. The program is built in ``build/pprzgcs``.

The packages are installed by ``install_deps.sh``, which can also be run alone:

.. code-block:: bash

    ./install_deps.sh            # to build and run PprzGCS
    ./install_deps.sh appimage   # also to package the AppImage
    ./install_deps.sh deb        # also to package the .deb
    ./install_deps.sh --check    # only list the missing packages

Other build options (``./build.sh --help``):

- ``./build.sh --debug``: a debug build (warnings are errors).
- ``./build.sh -DGRPC=ON -DCMAKE_PREFIX_PATH=<path/to/gRPC>``: with gRPC.

Other systems, or another Qt
____________________________

With a Qt from the `Qt installer <https://www.qt.io/download-open-source>`_ instead of the system's,
give its path to CMake (the system packages are then not checked):

.. code-block:: bash

    CMAKE_PREFIX_PATH=/path/to/Qt/6.x.y/gcc_64 ./build.sh

On other systems, install the equivalents of the packages listed in ``install_deps.sh``, then:

.. code-block:: bash

    ./build.sh --skip-checks

Running PprzGCS
_______________


To be able to run it from the paparazzi center, you need to add the ``build`` directory to your ``$PATH``. Adapt this command and add it to your .bashrc:

.. code-block:: bash

    export PATH="/path/to/PprzGCS/build:$PATH"

You should now be able to launch it from the Paparazzi center.

If you want to run it from the terminal, set the ``PAPARAZZI_HOME`` and the ``PAPARAZZI_SRC`` environment variables to your paparazzi path, then just run ``./build/pprzgcs``
