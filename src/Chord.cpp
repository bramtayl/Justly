#include "Chord.h"

#include <QtCore/qglobal.h>  // for operator!=, QFlags
#include <qstring.h>         // for QString
#include <qjsonarray.h>

#include "Note.h"       // for Note
#include "Utilities.h"  // for error_column, TreeLevel, chord_level

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

auto Chord::verify_json(const QJsonObject &json_chord, const std::vector<std::unique_ptr<const QString>>& new_instrument_pointers) -> bool {
  for (const auto &field_name : json_chord.keys()) {
    if (field_name == "numerator") {
      if (!(verify_bounded_int(json_chord, field_name, MINIMUM_NUMERATOR,
                               MAXIMUM_NUMERATOR))) {
        return false;
      }
    } else if (field_name == "denominator") {
      if (!(verify_bounded_int(json_chord, field_name, MINIMUM_DENOMINATOR,
                               MAXIMUM_DENOMINATOR))) {
        return false;
      }
    } else if (field_name == "octave") {
      if (!(verify_bounded_int(json_chord, field_name, MINIMUM_OCTAVE,
                               MAXIMUM_OCTAVE))) {
        return false;
      }
    } else if (field_name == "beats") {
      if (!(verify_bounded_int(json_chord, field_name, MINIMUM_BEATS,
                               MAXIMUM_BEATS))) {
        return false;
      }
    } else if (field_name == "volume_percent") {
      if (!(verify_bounded_double(json_chord, field_name,
                                  MINIMUM_VOLUME_PERCENT,
                                  MAXIMUM_VOLUME_PERCENT))) {
        return false;
      }
    } else if (field_name == "tempo_percent") {
      if (!(verify_bounded_double(json_chord, field_name, MINIMUM_TEMPO_PERCENT,
                                  MAXIMUM_TEMPO_PERCENT))) {
        return false;
      }
    } else if (field_name == "words") {
      if (!(verify_json_string(json_chord["words"], field_name))) {
        return false;
      }
    } else if (field_name == "notes") {
      const auto notes_object = json_chord[field_name];
      if (!verify_json_array(notes_object, "notes")) {
        return false;
      }
      const auto json_notes = notes_object.toArray();
      for (const auto &note_value : json_notes) {
        if (!verify_json_object(note_value, "note")) {
          return false;
        }
        const auto json_note = note_value.toObject();
        if (!(Note::verify_json(json_note, new_instrument_pointers))) {
          return false;
        }
      }
    } else {
      warn_unrecognized_field("chord", field_name);
      return false;
    }
  }
  return true;
}
