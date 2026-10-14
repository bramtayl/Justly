#pragma once

#include <QtWidgets/QMessageBox>

#include "actions/NoteLocation.hpp"
#include "rows/Voice.hpp"

struct OrphanedVoiceNameLocation {
  NoteLocation location;
  QString old_voice_name;
};

template <NoteInterface SubNote>
void warn_reassigned_voices(QWidget& parent, const int reassigned_count,
                            const QString& first_remaining_voice_name,
                            const bool is_clipboard = false) {
  QString message;
  QTextStream stream(&message);
  stream << QObject::tr("Reassigning ") << reassigned_count
         << (is_clipboard ? QObject::tr(" clipboard ") : QObject::tr(" "))
         << QObject::tr(SubNote::get_pitched()) << QObject::tr(" note")
         << (reassigned_count == 1 ? QObject::tr(" voice")
                                   : QObject::tr(" voices"))
         << QObject::tr(" to the first remaining voice \"")
         << first_remaining_voice_name << QObject::tr("\"");
  QMessageBox::warning(&parent,
                       is_clipboard ? QObject::tr("Unknown voice")
                                    : QObject::tr("Voice removed"),
                       message);
}
