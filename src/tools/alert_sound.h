#ifndef ALERT_SOUND_H
#define ALERT_SOUND_H

#include <QString>
#include <QList>

/**
 * The alert sounds (a chime for a warning, a beep for an alarm), played by the player of the system
 * (paplay, pw-play, aplay or afplay: no Qt Multimedia needed), on the output of the settings
 * (notify/output, empty: the default of the system, e.g. an intercom such as the Vokkero for the crew)
 * at its volume (notify/volume, 0 to 100 %).
 */
namespace AlertSound {

struct Output {
    QString name;           // the PulseAudio/PipeWire sink
    QString description;    // shown
};

/// The outputs of this computer (empty without pactl)
QList<Output> outputs();

/// Played with the settings (an output that is not connected: the default)
void play(bool alarm);
/// Played on this output (empty: the default) at this volume (0 to 100 %), e.g. the test of the settings
void play(bool alarm, QString output, int volume);

}

#endif // ALERT_SOUND_H
