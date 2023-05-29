#include "NoteChord.h"

#include <QtCore/qglobal.h>  // for qCritical

#include <set>

NoteChord::NoteChord(const std::vector<std::unique_ptr<const QString>>& instruments, const QString& default_instrument)
    : instruments(instruments), default_instrument(default_instrument) {

      };

auto NoteChord::error_column(int column) -> void {
  qCritical("No column %d", column);
}

auto NoteChord::has_instrument(QString maybe_instrument) const -> bool {
  for (int index = 0; index < instruments.size(); index = index + 1) {
    if (instruments.at(index)->compare(maybe_instrument) == 0) {
      return true;
    }
  }
  return false;
}
