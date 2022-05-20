.. developer_guide aircraft

Aircrafts
=========

AircraftManager
---------------

Aircrafts are uniquely identified by  their ID. 

Get an aircraft with the ``Aircraft* AircraftManager::getAircraft(QString id)`` method.

.. _aircraft:

Aircraft
--------

The ``Aircraft`` class is central in PprzGCS. It holds all the data relative to a given aircraft:

+ AC ID
+ name
+ flight plan
+ airframe
+ settings
+ position
+ status
+ config



Status
------

Get the status with the ``Aircraft::getStatus()`` method.

The ``AircraftStatus`` class store some of the most used aircraft's messages:

+ AP_STATUS
+ NAV_STATUS
+ CIRCLE_STATUS
+ SEGMENT_STATUS
+ ENGINE_STATUS
+ DL_VALUES
+ TELEMETRY_STATUS
+ FLY_BY_WIRE
+ FLIGHT_PARAM
+ SVSINFO

Use the ``getMessage(QString name)`` method to get the last received message.

This class also emit a signal for each of the above messages when a new one is received.


