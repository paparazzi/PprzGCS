.. developer_guide pprzapplication

PprzApplication
===============

``PprzApplication`` subclass ``QApplication`` to add two attributes: the toolbox and the main window.

There should be only one instance of ``PprzApplication``.

Get the PprzApplication instance from anywhere with the ``pprzApp()`` function.


Toolbox
-------

The toolbox holds these tools:

+ **AircraftManager**: manage the list of aircrafts
+ **SRTMManager**: holds STRM data (ground elevation data)
+ **PprzDispatcher**: Handle Ivy communication
+ **CoordinatesTransform**: Use PROJ to convert coordinates across different coordinates reference systems
+ **Units**: Manage units conversions
+ **DispatcherUi**: propagate specific events across all the application
+ **Speaker**: Handles text to speech.


Get the tools from the toolbox getters, e.g. : ``pprzApp()->toolbox()->aircraftManager()``


PprzMain
--------

PprzMain is the main window of the application.

