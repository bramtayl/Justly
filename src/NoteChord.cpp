#include "NoteChord.h"

#include <QtCore/qglobal.h>  // for qCritical

#include <set>

NoteChord::NoteChord(const std::vector<std::unique_ptr<const QString>>* instruments_pointer, const QString& default_instrument)
    : instruments_pointer(instruments_pointer), default_instrument(default_instrument) {

      };

auto NoteChord::error_column(int column) -> void {
  qCritical("No column %d", column);
}
