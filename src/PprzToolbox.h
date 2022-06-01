#ifndef PPRZTOOLBOX_H
#define PPRZTOOLBOX_H

#include <QObject>
#include <QApplication>

class PprzApplication;
class AircraftManager;
class SRTMManager;
class PprzDispatcher;
class CoordinatesTransform;
class Units;
class DispatcherUi;
class Watcher;
#if defined(SPEECH_ENABLED)
class Speaker;
#endif

class PprzToolbox : public QObject
{
    Q_OBJECT
public:
    explicit PprzToolbox(PprzApplication* app);

    AircraftManager*      aircraftManager      () {return _aircraftManager     ;}
    SRTMManager*          srtmManager          () {return _srtmManager         ;}
    PprzDispatcher*       pprzDispatcher       () {return _pprzDispatcher      ;}
    CoordinatesTransform* coordinatesTransform () {return _coordinatesTransform;}
    Units*                units                () {return _units               ;}
    DispatcherUi*         dispatcherUi         () {return _dispatcherUi        ;}
    Watcher*              watcher              () {return _watcher             ;}
#if defined(SPEECH_ENABLED)
    Speaker*              speaker              () {return _speaker             ;}
#endif

private:
    void setChildToolboxes(void);
    AircraftManager*      _aircraftManager      = nullptr;
    SRTMManager*          _srtmManager          = nullptr;
    PprzDispatcher*       _pprzDispatcher       = nullptr;
    CoordinatesTransform* _coordinatesTransform = nullptr;
    Units*                _units                = nullptr;
    DispatcherUi*         _dispatcherUi         = nullptr;
    Watcher*              _watcher              = nullptr;
#if defined(SPEECH_ENABLED)
    Speaker* _speaker                           = nullptr;
#endif

    friend class PprzApplication;

};


/// This is the base class for all tools
class PprzTool : public QObject {
    Q_OBJECT

public:

    PprzTool(PprzApplication* app, PprzToolbox* toolbox);

    // If you override this method, you must call the base class.
    virtual void setToolbox(PprzToolbox* toolbox);

protected:
    PprzApplication* _app;
    PprzToolbox*     _toolbox;
};

#endif // PPRZTOOLBOX_H
