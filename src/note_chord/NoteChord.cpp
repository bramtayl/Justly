#include "justly/NoteChord.hpp"

#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QString>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <iomanip>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"
#include "other/private.hpp"

auto get_data_type(NoteChordField note_chord_field) -> DataType {
  if (note_chord_field == type_column) {
    return type_type;
  }
  if (note_chord_field == instrument_column) {
    return instrument_type;
  }
  if (note_chord_field == interval_column) {
    return interval_type;
  }
  if (note_chord_field == beats_column ||
      note_chord_field == volume_ratio_column ||
      note_chord_field == tempo_ratio_column) {
    return rational_type;
  }
  if (note_chord_field == words_column) {
    return words_type;
  }
  Q_ASSERT(false);
  return other_type;
};

auto get_mime_type(DataType data_type) -> QString {
  switch (data_type) {
  case instrument_type:
    return INSTRUMENT_MIME;
  case interval_type:
    return INTERVAL_MIME;
  case rational_type:
    return RATIONAL_MIME;
  case words_type:
    return WORDS_MIME;
  case notes_type:
    return NOTES_MIME;
  case chords_type:
    return CHORDS_MIME;
  default:
    Q_ASSERT(false);
    return {};
  }
}

void copy_json(const nlohmann::json &copied, const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;
  copy_text(json_text.str(), mime_type);
}

auto get_words_schema() -> const nlohmann::json & {
  static const nlohmann::json words_schema(
      {{"type", "string"}, {"description", "the words"}});
  return words_schema;
}

auto get_note_chord_fields_schema() -> const nlohmann::json & {
  static const nlohmann::json note_chord_fields_schema(
      {{"instrument", get_instrument_schema()},
       {"interval", get_interval_schema()},
       {"beats", get_rational_schema("the number of beats")},
       {"volume_percent", get_rational_schema("volume ratio")},
       {"tempo_percent", get_rational_schema("tempo ratio")},
       {"words", get_words_schema()}});
  return note_chord_fields_schema;
}

void copy_text(const std::string &text, const QString &mime_type) {
  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData(mime_type, text.c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

NoteChord::NoteChord() : instrument_pointer(get_instrument_pointer("")) {}

NoteChord::NoteChord(const nlohmann::json &json_note_chord)
    : instrument_pointer(
          get_instrument_pointer(json_note_chord.value("instrument", ""))),
      interval(json_note_chord.contains("interval")
                   ? Interval(json_note_chord["interval"])
                   : Interval()),
      beats(json_note_chord.contains("beats")
                ? Rational(json_note_chord["beats"])
                : Rational()),
      volume_ratio(json_note_chord.contains("volume_ratio")
                       ? Rational(json_note_chord["volume_ratio"])
                       : Rational()),
      tempo_ratio(json_note_chord.contains("tempo_ratio")
                      ? Rational(json_note_chord["tempo_ratio"])
                      : Rational()),
      words(QString::fromStdString(json_note_chord.value("words", ""))) {}

auto NoteChord::json() const -> nlohmann::json {
  auto json_note_chord = nlohmann::json::object();
  if (!(interval.is_default())) {
    json_note_chord["interval"] = interval.json();
  }
  if (!(beats.is_default())) {
    json_note_chord["beats"] = beats.json();
  }
  if (!(volume_ratio.is_default())) {
    json_note_chord["volume_ratio"] = volume_ratio.json();
  }
  if (!(tempo_ratio.is_default())) {
    json_note_chord["tempo_ratio"] = tempo_ratio.json();
  }
  if (!words.isEmpty()) {
    json_note_chord["words"] = words.toStdString().c_str();
  }
  Q_ASSERT(instrument_pointer != nullptr);
  if (!instrument_pointer->is_default()) {
    json_note_chord["instrument"] = instrument_pointer->instrument_name;
  }
  return json_note_chord;
}

auto NoteChord::data(NoteChordField note_chord_field,
                     int role) const -> QVariant {
  auto is_display = role == Qt::DisplayRole;
  auto is_display_or_edit = is_display || role == Qt::EditRole;
  switch (note_chord_field) {
  case type_column:
    if (is_display) {
      return symbol();
    }
    return {};
  case interval_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(interval);
    }
    return {};
  case (beats_column):
    if (is_display_or_edit) {
      return QVariant::fromValue(beats);
    }
    return {};
  case volume_ratio_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(volume_ratio);
    }
    return {};
  case tempo_ratio_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(tempo_ratio);
    }
    return {};
  case words_column:
    if (is_display_or_edit) {
      return words;
    }
    return {};
  case instrument_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(instrument_pointer);
    }
    return {};
  default:
    Q_ASSERT(false);
    return {};
  }
};

auto NoteChord::setData(NoteChordField note_chord_field,
                        const QVariant &new_value) -> bool {
  switch (note_chord_field) {
  case interval_column:
    Q_ASSERT(new_value.canConvert<Interval>());
    interval = new_value.value<Interval>();
    return true;
  case beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    beats = new_value.value<Rational>();
    return true;
  case volume_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    volume_ratio = new_value.value<Rational>();
    return true;
  case tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    tempo_ratio = new_value.value<Rational>();
    return true;
  case words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    words = new_value.toString();
    return true;
  case instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    instrument_pointer = new_value.value<const Instrument *>();
    return true;
  default:
    Q_ASSERT(false);
    return false;
  }
};

auto NoteChord::copy_cell(NoteChordField note_chord_field) const -> bool {
  Q_ASSERT(instrument_pointer != nullptr);
  auto data_type = get_data_type(note_chord_field);
  nlohmann::json json_cell;
  switch (note_chord_field) {
  case instrument_column: {
    json_cell = nlohmann::json(instrument_pointer->instrument_name);
    break;
  }
  case interval_column: {
    json_cell = interval.json();
    break;
  };
  case beats_column: {
    json_cell = beats.json();
    break;
  };
  case volume_ratio_column: {
    json_cell = volume_ratio.json();
    break;
  };
  case tempo_ratio_column: {
    json_cell = tempo_ratio.json();
    break;
  };
  case words_column: {
    json_cell = nlohmann::json(words.toStdString().c_str());
    break;
  };
  default:
    Q_ASSERT(false);
    return false;
  }
  copy_json(json_cell, get_mime_type(data_type));
  return true;
}
