# Local map tiles

Tiles for a place that no online map shows in detail (an indoor flight arena), shipped in
`data/map/<DIR>/<z>/<x>/<y>.png` with a source in `data/tile_sources.xml` (no `addr`). PprzGCS reads a
tile from the tiles folder of the settings first, else from `data/map`.

## Cyberzoo (TU Delft)

A top view drawn from the Gazebo model of the Cyberzoo in paparazzi (floor, corner towers, the trusses
of the sides; not the lamps, cameras and top net), with the tables and chairs along its south-south-east
side (`add_furniture.py`), placed with the Optitrack frame of TU Delft: centered
on the `lat0`/`lon0` of the Cyberzoo flight plans, the model x axis at a bearing of 247 deg
(`natnet2ivy.py -le right -xa -23.0`). The Gazebo world gives 244 deg, the flight area of the flight
plans 238 deg: to turn it, change `--x-bearing`.

```bash
./render_cyberzoo.py $PAPARAZZI_HOME/sw/ext/tudelft_gazebo_models/models/cyberzoo /tmp/cyberzoo.png 0.01 8
./add_furniture.py /tmp/cyberzoo.png /tmp/cyberzoo_tables.png 0.01 8
./make_tiles.py /tmp/cyberzoo_tables.png ../../data/map/CYBERZOO --res 0.01 \
    --lat0 51.990634 --lon0 4.376789 --x-bearing 247 --crop-box -5.3 5.3 -5.3 6.6 --zmin 18 --zmax 23
```

`make_tiles.py` places any top view (a photo, a plan) the same way: x to the right, y up, centered on
`--lat0`/`--lon0`, `--res` m per pixel.
