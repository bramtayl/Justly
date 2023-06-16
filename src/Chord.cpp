#include "Chord.h"

#include <QtCore/qglobal.h>  // for QFlags
#include <qcolor.h>          // for QColor
#include <qjsonvalue.h>      // for QJsonValueRef
#include <qstring.h>         // for QString

#include "Utilities.h"  // for get_json_int, get_json_positive_double, get_positi...
#include "Note.h"

Chord::Chord(const QString &default_instrument)
    : NoteChord(default_instrument){};

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::flags(int column) const -> Qt::ItemFlags {
  auto generic_flags = NoteChord::flags(column);
  if (generic_flags != Qt::NoItemFlags) {
    return generic_flags;
  }
  if (column == instrument_column) {
    return Qt::NoItemFlags;
  }
  error_column(column);
  return Qt::NoItemFlags;
}

auto Chord::data(int column, int role) const -> QVariant {
  if (role == Qt::DisplayRole) {
    if (column == symbol_column) {
      return "♫";
    }
    auto generic_value = NoteChord::get_value(column);
    if (generic_value != QVariant()) {
      return generic_value;
    }
    if (column == instrument_column) {
      // need to return empty even if its inaccessible
      return {};
    }
    error_column(column);
  }
  if (role == Qt::ForegroundRole) {
    auto generic_color = NoteChord::get_color(column);
    if (generic_color != QVariant()) {
      return generic_color;
    }
    if (column == instrument_column) {
      // need to return empty even if its inaccessible
      return {};
    }
    error_column(column);
  }
  // no data for other roles
  return {};
}

auto Chord::setData(int column, const QVariant &new_value) -> bool {
  if (NoteChord::setData(column, new_value)) {
    return true;
  }
  error_column(column);
  return false;
}

auto Chord::copy_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Chord>(*this);
}

auto Chord::get_instrument() -> QString { return {}; }

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>(default_instrument);
} 

