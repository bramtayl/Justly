#include "justly/NoteChord.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMimeData>
#include <QString>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <string>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "justly/Instrument.hpp"
#include "justly/InstrumentEditor.hpp"
#include "justly/Interval.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"

auto get_words_size() -> QSize {
  static auto words_size = QLineEdit().sizeHint();
  return words_size;
};

void copy_text(const std::string &text, const std::string &mime_type) {
  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData(QString::fromStdString(mime_type),
                            QByteArray::fromStdString(text));

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
      words(json_note_chord.value("words", "")) {}

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
  if (!words.empty()) {
    json_note_chord["words"] = words;
  }
  const auto &instrument_name = instrument_pointer->instrument_name;
  if (!instrument_name.empty()) {
    json_note_chord["instrument"] = instrument_name;
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
  case symbol_column:
    switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdString(symbol());
    default:
      return {};
    }
    break;
  case interval_column:
    switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdString(interval.text());
    case Qt::EditRole:
      return QVariant::fromValue(interval);
    case Qt::SizeHintRole:
      return get_interval_size();
    default:
      return {};
    }
    break;
  case (beats_column):
    switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdString(beats.text());
    case Qt::EditRole:
      return QVariant::fromValue(beats);
    case Qt::SizeHintRole:
      return get_rational_size();
    default:
      return {};
    }
    break;
  case volume_ratio_column:
    switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdString(volume_ratio.text());
    case Qt::EditRole:
      return QVariant::fromValue(volume_ratio);
    case Qt::SizeHintRole:
      return get_rational_size();
    default:
      return {};
    }
    break;
  case tempo_ratio_column:
    switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdString(tempo_ratio.text());
    case Qt::EditRole:
      return QVariant::fromValue(tempo_ratio);
    case Qt::SizeHintRole:
      return get_rational_size();
    default:
      return {};
    }
    break;
  case words_column:
    switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdString(words);
    case Qt::EditRole:
      return QString::fromStdString(words);
    case Qt::SizeHintRole:
      return get_words_size();
    default:
      return {};
    }
    break;
  case instrument_column:
    switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdString(instrument_pointer->instrument_name);
    case Qt::EditRole:
      return QVariant::fromValue(instrument_pointer);
    case Qt::SizeHintRole:
      return get_instrument_size();
    default:
      return {};
    }
  default: {
    Q_ASSERT(false);
    return {};
  }
  }
};

void NoteChord::setData(NoteChordField note_chord_field,
                        const QVariant &new_value) {
  switch (note_chord_field) {
  case symbol_column:
    Q_ASSERT(false);
    break;
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
    words = new_value.toString().toStdString();
    break;
  case instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    instrument_pointer = new_value.value<const Instrument *>();
    break;
  }
};

void NoteChord::copy_cell(NoteChordField note_chord_field) const {
  Q_ASSERT(instrument_pointer != nullptr);

  switch (note_chord_field) {
  case symbol_column: {
    Q_ASSERT(false);
    return;
  };
  case instrument_column: {
    copy_json(nlohmann::json(instrument_pointer->instrument_name),
              INSTRUMENT_MIME);
    return;
  }
  case interval_column: {
    copy_json(interval.json(), INTERVAL_MIME);
    return;
  };
  case beats_column: {
    copy_json(beats.json(), RATIONAL_MIME);
    return;
  };
  case volume_ratio_column: {
    copy_json(volume_ratio.json(), RATIONAL_MIME);
    return;
  };
  case tempo_ratio_column: {
    copy_json(tempo_ratio.json(), RATIONAL_MIME);
    return;
  };
  case words_column: {
    copy_json(nlohmann::json(words), WORDS_MIME);
    return;
  };
  }
}
