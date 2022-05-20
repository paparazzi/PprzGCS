.. developer_guide widgets

Map
===

The map is by far the most complex widget. Let's see how is works!

Map2D
-----

First, let focus on the map itself, without all the elements that populates it.

The *Map2D* widget inherits *QGraphicsView*, and holds a *MapScene* (that inherits *QGraphicsScene*).

This widget only support maps in the Pseudo-Mercator projected coordinate system, a.k.a *EPSG:3857*. There are plans to support other coordinate systems in the future.

See `<https://www.maptiler.com/google-maps-coordinates-tile-bounds-projection/>`_ to learn how this kind of map works.

In our case, the QGraphicsScene coordinates are directly related to the tiles coordinates, multiplied by the size of the tiles.


.. admonition:: Example
    
    + At zoom level 0, the map's bottom right coordinate is (256, 256),
    + At zoom level 1, it's (512, 512),
    + At zoom level 1, it's (1024, 1024),
    + ...
    
    See ``src/widgets/map/maputils.cpp`` for some helper functions.

There is a *TileProvider*, associated with each map layers. New tile providers can be added in the ``tile_sources.xml`` file.

Tiles are fist opened from local cache directory, or downloaded from the net is not present locally. Remove local cache to get the latest tiles.


Downloading tile can take a few seconds, or can fail. Before a tile is downloaded succesfully, it is recreated by assembling its 4 child tiles (at zoom n+1), or by scaling up its parent's tile.



