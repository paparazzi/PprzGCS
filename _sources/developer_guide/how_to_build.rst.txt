.. developer_guide how_to_build

How to build from sources
=========================

First, get all submodules:

.. code-block:: bash

    git submodule update --init --recursive

Add the Paparazzi PPA to ivy-c:

.. code-block:: bash

    sudo add-apt-repository -y ppa:paparazzi-uav/ppa

Ubuntu 20.04 and above
______________________
 
Install the dependencies:

.. code-block:: bash

    sudo apt install ivy-c-dev libboost-system-dev libsqlite3-dev libzip-dev qtbase5-dev libqt5svg5-dev libproj-dev mesa-common-dev libglu1-mesa-dev

cd to the PprzGCS root, and build the needed libraries:

.. code-block:: bash

    ./build.sh libs

Now you should be able to build the app with:

.. code-block:: bash

    BUILD_TARGET=DEV_20_04 ./build.sh app
    
Then just run it:

.. code-block:: bash
    
    ./build/PprzGCS


Ubuntu 18.04
____________

First, you need to `install Qt5.12.0 or above. <https://www.qt.io/download-open-source>`_

Setup in your `.bashr` the environnement variable `Qt5_DIR` for cmake to find Qt5:

.. code-block:: bash

    export Qt5_DIR="/path/to/Qt/5.12.0/gcc_64/lib/cmake/Qt5"

Install the dependencies:

.. code-block:: bash

    sudo apt install ivy-c-dev libboost-system-dev libsqlite3-dev libzip-dev mesa-common-dev libglu1-mesa-dev

cd to the PprzGCS root, and build the needed libraries:

.. code-block:: bash

    build_proj=true ./build.sh libs

Now you should be able to build the app with:

.. code-block:: bash

    BUILD_TARGET=DEV_18_04 ./build.sh app
    
Then just run it:

.. code-block:: bash
    
    ./build/PprzGCS




