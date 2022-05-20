.. developer_guide widgets

Widgets
=======

Widgets can be related to a particular AC, or not.

If related to an AC, it will be intanciated for each AC, and grouped under a ``list`` or a ``stack`` container.

To access the Aircraft's data, use:

``Aircraft* ac = pprzApp()->toolbox()->aircraftManager()->getAircraft(ac_id);``

See :ref:`aircraft`.


Creating a new widget
---------------------

Create your widget, inheriting ``QWidget``.

For an AC related widget, the constructor must be of this form:

    ``MyAwesomeWidget::MyAwesomeWidget(QString ac_id, QWidget *parent)``


Put your files in the ``src/widgets/`` directory, and add the cpp filename to ``src/widgets/CMakeLists.txt``

Add your widget type in the appropriate lists in the ``src/widgets/widget_utils.cpp`` file:

+ the **ac_widgets** list is forwidgets related to an AC
+ **simple_widgets** is for widgets not related to a specific AC.
+ **containers** is for container widgets (containing AC widgets)

You need to add it in the type definition (the ``using...`` lines), and in the map definition.
The first element in the map definition is the designator of the widget in layout files.


Typical widget architecture
---------------------------

typically, a widget will connect to some signals to update its data accordingly.

With AircraftStatus
___________________

Binding to AircraftStatus's signal is the easiest way to get AC data.

In this extract of the MiniStrip widget constructor, it connects to 3 signals and update widget's data in the corresponding slots:

.. code-block:: cpp

    auto ac_status = AircraftManager::get()->getAircraft(ac_id)->getStatus();
    connect(ac_status, &AircraftStatus::engine_status, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::flight_param, this, &MiniStrip::updateData);
    connect(ac_status, &AircraftStatus::telemetry_status, this, &MiniStrip::updateData);

With PprzDispatcher
___________________

The signals emitted by AircraftsStatus are also emitted by the PprzDispatcher.
It is usefull to bind to these signals to receive the corresponding message for any AC.
This is the case for the map, that is not related to a particular AC:

.. code-block:: cpp

    connect(pprzApp()->toolbox()->pprzDispatcher(), &PprzDispatcher::circle_status, this, &MapWidget::updateNavShape);

Connect to any message
______________________

Finally, you can bind a callback to any message with the PprzDispatcher's ``bind`` method that returns the bind id, needed to unbind.

.. code-block:: cpp

    using messageCallback_t = std::function<void(QString, Message)>;
    long PprzDispatcher::bind(QString msg_name, QObject* context, pprzlink::messageCallback_t cb);

.. code-block:: cpp

    pprzApp()->toolbox()->pprzDispatcher()->bind("INTRUDER", this,
            [=](QString sender, pprzlink::Message msg) {
                onIntruder(sender, msg);
            });


.. warning::

    When using lambdas, always specify a context object (often ``this``),
    otherwise Qt won't be able to disconnect the signal when the object is destroyed,
    leading to a use after free.


