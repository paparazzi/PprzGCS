
Settings and config
===================

Settings and global configurations are both global and accessible from anywhere in the application. They differ only by the intended lifespan.

Settings
--------

Settings are stored in a file and are saved accross sessions.

It uses the QSettings interface, with a file named ``settings.conf`` at the standard path ``QStandardPaths::AppDataLocation``.
Thus, it's most probably corresponds to ``$HOME/.local/share/pprzgcs/settings.conf``.


Use the helper function ``QSettings getAppSettings()`` from ``gcs_utils.h`` to start using them.


GlobalConfig
------------

The global config is a singleton holding a key/value configuration.

It lives only for the current session.

Use the helper function ``GlobalConfig* appConfig()`` from ``gcs_utils.h`` to start using it.

Like the settings, it uses QVariant type to hold its items.



