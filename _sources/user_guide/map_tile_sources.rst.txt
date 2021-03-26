.. developer_guide index_developer

Map tile sources
================

You can easily add or remove tile sources from the layer tab.
It can be public tile sources or you can even generate your own tiles !

Edit tile sources
_________________

You need to copy ``tile_sources.xml`` in your user directory, and edit this file.

To do so, you can start PprzGCS, then go to ``File->Open user directory`` and ``File->Open app directory``. Then just copy ``tile_sources.xml`` from the app directory to the user directory.

.. note::

    The files in the user directory take precedence over those in the app directory.


A tile source is identified by an xml node of this form:

.. code-block:: xml

    <source
        name="OpenStreetMap" dir="OSM_CLASSIC"
        addr="http://tile.openstreetmap.org/{z}/{x}/{y}.png"
        zoomMin="0" zoomMax="19"
        tileSize="256"
        format=".png"
    />

+ **name** is the name of the layer in PprzGCS.
+ **dir** is the name of the directory in which the tiles will be saved. In this example, the tiles are saved in ``map/OSM_CLASSIC``.
+ **addr** is the address from which the tiles will be fetched.
    + Use ``{x}``, ``{y}`` and ``{z}`` as placeholders for tiles coordinates.
    + Don't forget to escape characters according to XML format (e.g. ``&`` become ``&amp;``).
    + Set ``addr=""`` if don't want to fetch tiles from internet but only from local directory.
+ **zoomMin** and **zoomMax** define the min and max available zoom levels.
+ **tileSize** is the size in pixel of the tiles. A current size is 256.
+ **format** is the file extension of the tiles. A current format is `.png`.



Make your own tiles
___________________

You can make your own tiles with QGIS using the processing tool ``Raster tools -> Generate XYZ tiles (Directory)``.

Lets do a rather complicated example: I want to make a map for local ENU/NED coordinates from an image.

First, let create a georeferenced image:

+ In QGIS, go to *Project->Properties->SRC* and select the CRS in which your coordinates are.

.. note::
    
    In my case, I want them to be in my local coordinate system, so I first have to define my own CRS:
    
    - Exit the properties and go to *Settings->Custom projections*.
    - Add a projection with the *Proj String* format, and the following parameters: ``+proj=ortho +lat_0=43.564092 +lon_0=1.48292``, replacing the origin coordinates.
    - Select the newly created CRS in you project properties.

+ Go to *Plugins->Manage and Install Plugins*  and activate *Georeferencer GDAL*.
+ Open the *Raster->Georeferencer* tool, and open you image with *Open raster*.
+ Open the *Transformation settings* and set the *Target CRS*, and the Output raster.
+ Click on a point you know the coordinates, and enter the coordinates.
+ Repeat the previous step for multiple points.
+ Run *Start Georeferencing*. You're done!

Now, lets create the tiles:

+ Import the georeferenced image you created back in QGIS.
+ Run  the processing tool ``Raster tools -> Generate XYZ tiles (Directory)``
+ Select the extent and the minimun and maximum zoom
+ Select the output directory
+ Run the tool

Now, lets import theses tiles in PprzGCS:

+ Open the PprzGCS user directory, and copy the tiles directory in ``map/<YOUR_MAP_NAME>``
+ Add the source to the ``tile_sources.xml`` file, as described above.
+ You're done ! Open PprzGCS and enjoy !


