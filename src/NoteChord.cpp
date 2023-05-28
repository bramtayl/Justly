#include "NoteChord.h"

#include <QtCore/qglobal.h>  // for qCritical

#include <set>

NoteChord::NoteChord(const std::set<QString>& instruments)
    : instruments(instruments){

      };

auto NoteChord::error_column(int column) -> void {
  qCritical("No column %d", column);
}
