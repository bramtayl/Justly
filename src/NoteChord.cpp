#include "NoteChord.h"

#include <QtCore/qglobal.h>  // for qCritical

NoteChord::NoteChord(const std::set<std::string>& instruments) : instruments(instruments) {
  
};

auto NoteChord::error_column(int column) -> void {
  qCritical("No column %d", column);
}
