# Changelog

## NEXT
+ ATC panel: the bearing and distance can be from any waypoint (click FROM to choose, per aircraft); by default ARP, else HOME
+ ATC panel: the QFE altitude replaced by the GPS height above MSL (hmsl of GPS_INT, GPS_LLA, GPS); a QFE can still be shown by setting the ground pressure as QNH
+ ATC panel: distances in NM (2 decimals) or in m (km from 1000 m), chosen with chips like the altitude reference; a narrow START/RESET TIMER button on two lines, with the time and the distance under each other
+ ATC panel: a timer per aircraft; a running timer keeps counting while another aircraft is selected
+ Video: the size menu and Ctrl + mouse wheel removed (drag the corner of the panel to resize it); right click: Video source... to choose a udp:// or tcp:// source in the GCS (saved, instead of the one of the layout)
+ GPS: a button next to the quality shows a sky plot of the satellites (azimuth, elevation) below the bars, in the color of their bar (green used, gray not used, orange no lock); SBAS satellites as an outline (bars and sky plot)
+ Builds on Ubuntu 26.04; extra-cmake-modules no longer needed (icons and desktop files installed with GNUInstallDirs)
+ Map: an aircraft icon that cannot be opened is reported instead of loaded as an empty image
+ Checklist: `optional`, `help` (tooltip) and `remember` attributes, `choice` items and a Reset button
+ On `AIRCRAFT_REBOOT` the aircraft is reloaded (waypoints, settings, checklist), with a notice in the chat
+ Wind indicator and GVF config cleared on `AIRCRAFT_DIE`
+ Airframe: follow `<include>` for the checklist and defines (read-only)
+ Map: position, reference and SRTM moved from the row above the map to the status bar; empty toolbar removed
+ Map: smaller side buttons (48 px) with a tooltip; the lock shows on hover or when locked, and a long press locks (touchscreens)
+ Map: compact wind indicator (compass and wind speed in one pill); only the compass when there is no wind data
+ Flight plan: blocks as full-width rows (touch friendly); the current block is highlighted with its time; tap a block, then Go, to jump
+ Layout: the strips take the height of their content, the rest goes to the flight plan
+ Strip: new layout (name, mode, block and time; then labeled battery, altitude, speed and throttle); status icons only when not ok, with a text; lost links frame the strip in amber or red, a low battery shows the voltage in amber or red
+ Aircraft selection: click a strip to select its aircraft (selected strip framed); the aircraft bar above the panels only shows when the layout has no strips; the first aircraft is selected at start
+ Map: aircraft icons whose color is in a style attribute (e.g. rotorcraft) now take the aircraft color instead of always red
+ Notifications at the top of the map (see-through): messages for 6 s (3 s when busy), low battery and lost links until fixed or dismissed; alarms first, at most three rows, repeats counted (×8), "+N more in the chat"; clicking selects and centers the aircraft. Replaces the battery and link icons next to the aircraft
+ Chat: a row per message with the sender name as a tag in its aircraft color, the time on hover, warnings highlighted, own messages in blue; filter row (All, each aircraft, Server, Mine) with more than one aircraft; message, recipient/log menu and Send on one row
+ Rail panels redesigned: PFD as an attitude indicator with pitch ladder, roll scale, a speed tape (ground speed, airspeed), a height (AGL) tape with the target height and climb, and a heading tape, plus roll/pitch/heading/MSL values; compact chat, cards without extra width or padding; GPS with accuracy, satellites and a quality pill above signal bars per satellite; chat bubbles (received left, sent in blue) and a Send button; link status as a row per link; map layers as a short list with a small thumbnail, a clear title (tile_sources title attribute), an on/off switch and the opacity behind a button
+ Map rail panels (layers, GPS, chat, PFD, link) open as cards with a title bar and a close button; GPS quality as a colored bar and text; commands menu as a card with the aircraft color and section captions
+ Panels: drag a tab (Flight plan, Settings, ...) or a rail button to another tab group, below a group (split), to the bottom of the map or to the rail; the arrangement is restored at start, File > Reset panels goes back to the layout file
+ Layout: Flight plan, Settings, Checklist and Plot as tabs on the right (new <tabs> layout element); a full-height rail with line icons on the left of the map; flat tabs; strips as cards with a thin aircraft-color bar; block names with their waypoints in gray; status bar with Server and the link of the selected aircraft
+ Robustness: an error while handling an incoming message is logged instead of stopping the GCS; an unknown block number (e.g. flight plan of another build) or an unsupported firmware in the PFD no longer stop the GCS
+ Settings: groups with the same name (e.g. System from several modules) are shown as one; compact rows (name, value on the aircraft with unit, send/undo/reset, editor below); a sent value is amber until the aircraft confirms it; search and save on one row, home at the start of the path; the group in search results
+ Checklist: progress bar and count of the required items, open items in amber, aligned rows
+ Style: the default theme is for sunlight; new night theme (night_style.qss), chosen from a list in Settings > General > Style
+ Chat: unread-message bubble on the chat button when an `INFO_MSG` arrives while the chat is closed

## v1.0.13
+ Add Chat widget for `INFO_MSG`
+ support for GVF bezier curves
+ preflight checks
+ and much more...

## v1.0.9
+ PFD: match map rotation
+ ListContainer popup : fit in the current screen
+ Add GVFViewer widget
+ Add map menu to the menubar, and a map context menu.
+ some bug fixes

## v1.0.8
+ PDF: support rover
+ map: add alarms
+ map: add arrow for drone outside the view

## v1.0.7
+ MiniStrip: AUTO2 button
+ flight plan parsing: handle bad DTD path
+ build system: make speech optional

## v1.0.6
+ Add GCS position
+ Fix flightplan editor bug
+ Fix compatibility with old paparazzi
+ Make deb package on launchpad
+ Fix Intruders update
+ remove PROJ
+ build AppImage on Github

## v1.0.5
+ Rename binary to lower case.
+ Fix bad UTM zone
+ Fix bug in FlightPlanEditor
+ Handle multiple links, and add a LinkStatus widget
+ Add intruders support

## v1.0.4
+ Improve touch
+ MiniStrip: handle bat level icon
+ Plotter: fix zoom with trackpad
+ Add wind and north indicator
+ Add AC tracking mode
+ MiniStrip: Add block time and stage time
+ FlightPlanEditor: add more details to 2nd column

## v1.0.3
+ Add map touch support (zoom and pan)
+ Add map rotation

## v1.0.2
+ Add text to speech
+ Fix strip mode colors
+ Make some setting non-persistant

## v1.0.1
+ Fix some bugs
+ Improve containers
+ Add plotter
+ Add some doc

## v1.0.0

+ Save settings in airframe file (to be improved)
+ Fix some bugs in the map
+ Add shapes
+ Add FlightPlanEditor
+ Better FlighPlanViewer
+ ...

## v1.0/alpha-3

+ handle LTP coordinates
+ refactor singletons away
+ replace pprzlink++ by a pure Qt version
+ reduce external dependencies
+ improve ministrips
+ ...

## v1.0/alpha-2

+ Add Papgets
+ Add "show hidden waypoints" functionnality
+ Fix nav circle flickering
+ Draw dynamic sectors
+ Add segment navigation shape

