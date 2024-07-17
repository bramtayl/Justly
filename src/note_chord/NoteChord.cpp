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

void copy_text(const std::string &text, const std::string &mime_type) {
  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData(QString::fromStdString(mime_type), text.c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

void copy_json(const nlohmann::json &copied, const char *mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;
  copy_text(json_text.str(), mime_type);
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

auto get_note_chord_fields_schema() -> const nlohmann::json & {
  static const nlohmann::json note_chord_fields_schema(
      {{"instrument", get_instrument_schema()},
       {"interval", get_interval_schema()},
       {"beats", get_rational_schema("the number of beats")},
       {"volume_percent", get_rational_schema("volume ratio")},
       {"tempo_percent", get_rational_schema("tempo ratio")},
       {"words", {{"type", "string"}, {"description", "the words"}}}});
  return note_chord_fields_schema;
}

auto NoteChord::data(NoteChordField note_chord_field,
                     int role) const -> QVariant {
  switch (note_chord_field) {
  case type_column:
    if (role == Qt::DisplayRole) {
      return symbol();
    }
    break;
  case interval_column:
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return QVariant::fromValue(interval);
    }
    break;
  case (beats_column):
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return QVariant::fromValue(beats);
    }
    break;
  case volume_ratio_column:
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return QVariant::fromValue(volume_ratio);
    }
    break;
  case tempo_ratio_column:
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return QVariant::fromValue(tempo_ratio);
    }
    break;
  case words_column:
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return words;
    }
    break;
  case instrument_column:
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return QVariant::fromValue(instrument_pointer);
    }
    break;
  default: {
    Q_ASSERT(false);
  }
  }
  return {};
};

void NoteChord::setData(NoteChordField note_chord_field,
                        const QVariant &new_value) {
  switch (note_chord_field) {
  case interval_column:
    Q_ASSERT(new_value.canConvert<Interval>());
    interval = new_value.value<Interval>();
    break;
  case beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    beats = new_value.value<Rational>();
    break;
  case volume_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    volume_ratio = new_value.value<Rational>();
    break;
  case tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    tempo_ratio = new_value.value<Rational>();
    break;
  case words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    words = new_value.toString();
    break;
  case instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    instrument_pointer = new_value.value<const Instrument *>();
    break;
  default:
    Q_ASSERT(false);
  }
};

void NoteChord::copy_cell(NoteChordField note_chord_field) const {
  Q_ASSERT(instrument_pointer != nullptr);

  switch (note_chord_field) {
  case instrument_column: {
    copy_json(nlohmann::json(instrument_pointer->instrument_name),
              INSTRUMENT_MIME);
    break;
  }
  case interval_column: {
    copy_json(interval.json(), INTERVAL_MIME);
    break;
  };
  case beats_column: {
    copy_json(beats.json(), RATIONAL_MIME);
    break;
  };
  case volume_ratio_column: {
    copy_json(volume_ratio.json(), RATIONAL_MIME);
    break;
  };
  case tempo_ratio_column: {
    copy_json(tempo_ratio.json(), RATIONAL_MIME);
    break;
  };
  case words_column: {
    copy_json(nlohmann::json(words.toStdString().c_str()), WORDS_MIME);
    break;
  };
  default:
    Q_ASSERT(false);
  }
}
