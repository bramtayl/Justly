#include "NoteChord.h"

#include <QtCore/qglobal.h>  // for qCritical

auto NoteChord::error_column(int column) -> void {
  qCritical("No column %d", column);
}
