#pragma once

#include <QtWidgets/QMessageBox>

#include "rows/Chord.hpp"
#include "rows/Voice.hpp"

// a note whose voice_number was overwritten with a value that doesn't encode
// the original (e.g. reassigned to the first remaining voice), so the old
// voice number must be stored to be restorable on undo
template <VoiceInterface SubVoice>
struct OrphanedVoiceNumberLocation {
  int chord_number;
  int note_number;
  int old_voice_number;
};

// shared by RemoveVoiceRows and pasting so a live note reassignment and a
// pasted note whose voice no longer exists are reported identically, aside
// from is_clipboard calling out that the latter only affects notes coming
// from the OS clipboard
template <NoteInterface SubNote>
static void warn_reassigned_voices(QWidget& parent, const int reassigned_count,
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
