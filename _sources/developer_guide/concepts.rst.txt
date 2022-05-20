.. developer_guide concepts

Arborescence
============

Root directory
--------------

The essential directories are these ones :

.. code-block::

    PprzGCS
    ├── data
    ├── docs
    ├── ext
    ├── resources
    └── src

+ ``data`` holds all customizable resources. It will usually be installed in ``/usr/share/pprzgcs``.
  Files in this directory are overridden by files with similar path in user's directory ``$HOME/.local/share/pprzgcs`` (See AppDataLocation `QStandardPaths <https://doc.qt.io/qt-5/qstandardpaths.html>`_).
+ ``docs`` contains the documentation consultable at `https://docs.paparazziuav.org/PprzGCS/ <https://docs.paparazziuav.org/PprzGCS/>`_.
+ ``ext`` contains external dependencies
+ ``resources`` holds static resources. PprzGCS must be rebuild when changes are made in this directory.
+ ``src`` contains all the source code.


Sources
-------




