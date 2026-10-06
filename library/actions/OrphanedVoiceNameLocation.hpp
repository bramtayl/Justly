#pragma once

#include <QtWidgets/QMessageBox>

#include "actions/NoteLocation.hpp"
#include "rows/Voice.hpp"

// a note reassigned from a removed voice to the first remaining voice, so the
// old voice name must be stored to be restorable on undo
struct OrphanedVoiceNameLocation {
  NoteLocation location;
  QString old_voice_name;
};

// shared by RemoveVoiceRows and pasting so a live note reassignment and a
// pasted note whose voice no longer exists are reported identically, aside
// from is_clipboard calling out that the latter only affects notes coming
// from the OS clipboard
template <NoteInterface SubNote>
void warn_reassigned_voices(QWidget& parent, const int reassigned_count,
                            const QString& first_voice_name,
                            const bool is_clipboard = false) {
  QString message;
  QTextStream stream(&message);
  stream << QObject::tr("Reassigning ") << reassigned_count
         << (is_clipboard ? QObject::tr(" clipboard ") : QObject::tr(" "))
         << QObject::tr(SubNote::get_pitched()) << QObject::tr(" note")
         << (reassigned_count == 1 ? QObject::tr(" voice")
                                   : QObject::tr(" voices"))
         << QObject::tr(" to the first voice \"") << first_voice_name
         << QObject::tr("\"");
  QMessageBox::warning(&parent,
                       is_clipboard ? QObject::tr("Unknown voice")
                                    : QObject::tr("Voice removed"),
                       message);
}
