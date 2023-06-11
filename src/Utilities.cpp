#include "Utilities.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/std_abs.h>          // for abs
#include <qbytearray.h>            // for QByteArray
#include <qcombobox.h>             // for QComboBox
#include <qcoreapplication.h>      // for QCoreApplication
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValue
#include <qmessagebox.h>           // for QMessageBox
#include <qregularexpression.h>    // for QRegularExpressionMatchIteratorRan...
#include <qstring.h>               // for QString, operator+, operator==

#include <QJsonArray>
#include <algorithm>  // for any_of, max
#include <cmath>      // for round
#include <cstdlib>    // for abs
#include <limits>     // for numeric_limits
#include <utility>    // for move

void json_parse_error(const QString &error_text) {
  QMessageBox::warning(nullptr, "JSON parsing error",
                       error_text + " Cannot load");
}

auto verify_json_string(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isString()) {
    json_parse_error(
        QString("Non-string %1: %2!").arg(field_name).arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_double(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isDouble()) {
    json_parse_error(
        QString("Non-double %1: %2!").arg(field_name).arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_array(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isArray()) {
    json_parse_error(QString("%1 must be an array: %2!")
                         .arg(field_name)
                         .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_object(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isObject()) {
    json_parse_error(QString("%1 must be an object: %2!")
                         .arg(field_name)
                         .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_instrument(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QJsonObject &json_object, const QString &field_name) -> bool {
  const auto json_value = json_object[field_name];
  if (!(verify_json_string(json_value, field_name))) {
    return false;
  }
  const auto instrument = json_value.toString();
  if (!has_instrument(instrument_pointers, instrument)) {
    json_parse_error(
        QString("Cannot find %1 %2").arg(field_name).arg(instrument));
    return false;
  }
  return true;
}

auto verify_whole(double value, const QString &field_name) -> bool {
  if ((value - static_cast<int>(value)) >
      std::numeric_limits<double>::epsilon()) {
    json_parse_error(QString("Non-whole %1: %2").arg(field_name).arg(value));
    return false;
  }
  return true;
}

auto verify_whole_object(const QJsonObject json_object,
                         const QString &field_name) -> bool {
  auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  return verify_whole(json_value.toDouble(), field_name);
}

auto verify_positive(double value, const QString &field_name) -> bool {
  if (value <= 0) {
    json_parse_error(QString("Non-positive %1: %2").arg(field_name).arg(value));
    return false;
  }
  return true;
}

auto verify_json_positive(const QJsonObject &json_object,
                          const QString &field_name) -> bool {
  const auto json_value = json_object[field_name];
  if (!verify_json_double(json_value, field_name)) {
    return false;
  }
  auto value = json_value.toDouble();
  return verify_positive(value, field_name);
}

auto verify_positive_int(const QJsonObject &json_object,
                         const QString &field_name) -> bool {
  const auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  auto value = json_value.toDouble();
  return verify_positive(value, field_name) &&
         verify_whole(json_value.toDouble(), field_name);
}

auto verify_non_negative_int(const QJsonObject &json_object,
                             const QString &field_name) -> bool {
  const auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  auto value = json_value.toDouble();
  if (value < 0) {
    json_parse_error(QString("Negative %1: %2").arg(field_name).arg(value));
    return false;
  }
  return verify_whole(value, field_name);
}

auto verify_percent(double value, const QString &field_name) -> bool {
  if (value > 100) {
    json_parse_error(
        QString("%1 must be <= 100: is %2").arg(field_name).arg(value));
    return false;
  }
  return true;
}

auto verify_positive_percent(const QJsonObject &json_object,
                             const QString &field_name) -> bool {
  auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  auto value = json_value.toDouble();
  return verify_percent(value, field_name) &&
         verify_positive(value, field_name);
}

auto get_json_string(const QJsonObject &object, const QString &field_name,
                     const QString &a_default) -> QString {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toString();
}

auto get_json_int(const QJsonObject &object, const QString &field_name,
                  int a_default) -> int {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toInt();
}

void cannot_open_error(const QString &filename) {
  qCritical("Cannot open file %s", qUtf8Printable(filename));
}

auto has_instrument(
    const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &maybe_instrument) -> bool {
  return std::any_of(instrument_pointers.cbegin(), instrument_pointers.cend(),
                     [&maybe_instrument](const auto &instrument_pointer) {
                       return *instrument_pointer == maybe_instrument;
                     });
}

void error_row(size_t row) {
  qCritical("Invalid row %d", static_cast<int>(row));
};

void error_column(int column) { qCritical("No column %d", column); }

void error_empty() { qCritical("Nothing selected!"); }

void extract_instruments(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &orchestra_text) {
  QRegularExpression const instrument_pattern(R"(\binstr\s+\b(\w+)\b)");
  QRegularExpressionMatchIterator const instrument_matches =
      instrument_pattern.globalMatch(orchestra_text);
  for (const QRegularExpressionMatch &match : instrument_matches) {
    instrument_pointers.push_back(
        std::move(std::make_unique<QString>(match.captured(1))));
  }
}

void fill_combo_box(
    QComboBox &combo_box,
    std::vector<std::unique_ptr<const QString>> &text_pointers) {
  for (auto &text_pointer : text_pointers) {
    combo_box.addItem(*text_pointer);
  }
}

void set_combo_box(QComboBox &combo_box, const QString &value) {
  const int combo_box_index = combo_box.findText(value);
  // if it is valid, adjust the combobox
  if (combo_box_index < 0) {
    qCritical("Cannot find ComboBox value %s", qUtf8Printable(value));
    return;
  }
  combo_box.setCurrentIndex(combo_box_index);
}

void error_instrument(const QString &instrument, bool interactive) {
  if (interactive) {
    QMessageBox::warning(
        nullptr, "Instrument warning",
        QString("Cannot find instrument %1! Not changing orchestra text")
            .arg(instrument));
  } else {
    qCritical("Cannot find instrument %s", qUtf8Printable(instrument));
  }
}

void error_root() { qCritical("Is root!"); }

auto require_json_field(const QJsonObject &json_object,
                        const QString &field_name) -> bool {
  if (!(json_object.contains(field_name))) {
    QMessageBox::warning(nullptr, "JSON parsing error",
                         QString("No field %1!").arg(field_name));
    return false;
  }
  return true;
}

auto warn_unrecognized_field(const QString &level, const QString &field) {
  json_parse_error(QString("Unrecognized %1 field %2").arg(level).arg(field));
}

auto verify_json(const QJsonObject &json_song) -> bool {
  if (!(require_json_field(json_song, "orchestra_text") &&
        require_json_field(json_song, "default_instrument") &&
        require_json_field(json_song, "frequency") &&
        require_json_field(json_song, "volume_percent") &&
        require_json_field(json_song, "tempo"))) {
    return false;
  }

  const auto orchestra_value = json_song["orchestra_text"];
  if (!verify_json_string(orchestra_value, "orchestra_text")) {
    return false;
  }
  std::vector<std::unique_ptr<const QString>> instrument_pointers;
  extract_instruments(instrument_pointers, orchestra_value.toString());

  for (const auto &field_name : json_song.keys()) {
    if (field_name == "default_instrument") {
      if (!(require_json_field(json_song, field_name) &&
            verify_json_instrument(instrument_pointers, json_song,
                                   field_name))) {
        return false;
      }
    } else if (field_name == "frequency") {
      if (!(require_json_field(json_song, field_name) &&
            verify_json_positive(json_song, field_name))) {
        return false;
      }
    } else if (field_name == "volume_percent") {
      if (!(require_json_field(json_song, field_name) &&
            verify_positive_percent(json_song, field_name))) {
        return false;
      }
    } else if (field_name == "tempo") {
      if (!(require_json_field(json_song, field_name) &&
            verify_json_positive(json_song, field_name))) {
        return false;
      }
    } else if (field_name == "children") {
      auto chords_value = json_song[field_name];
      if (!(verify_json_array(chords_value, "chords"))) {
        return false;
      }
      for (const auto &chord_value : chords_value.toArray()) {
        if (!(verify_json_object(chord_value, "chord"))) {
          return false;
        }
        const auto json_chord = chord_value.toObject();
        for (const auto &field_name : json_chord.keys()) {
          if (field_name == "numerator" || field_name == "denominator") {
            if (!(verify_positive_int(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "octave") {
            if (!(verify_whole_object(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "beats") {
            if (!(verify_non_negative_int(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "volume_percent" ||
                     field_name == "tempo_percent") {
            if (!(verify_positive_percent(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "words") {
            if (!(verify_json_string(json_chord["words"], field_name))) {
              return false;
            }
          } else if (field_name == "children") {
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
              for (const auto &field_name : json_note.keys()) {
                if (field_name == "numerator" || field_name == "denominator") {
                  if (!(verify_positive_int(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "octave") {
                  if (!(verify_whole_object(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "beats") {
                  if (!(verify_non_negative_int(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "volume_percent" ||
                           field_name == "tempo_percent") {
                  if (!(verify_positive_percent(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "words") {
                  if (!(verify_json_string(json_note["words"], field_name))) {
                    return false;
                  }
                } else if (field_name == "instrument") {
                  if (!verify_json_instrument(instrument_pointers, json_note,
                                              "instrument")) {
                    return false;
                  }
                } else {
                  warn_unrecognized_field("note", field_name);
                  return false;
                }
              }
            }
          } else {
            warn_unrecognized_field("chord", field_name);
            return false;
          }
        }
      }
    } else if (!(field_name == "orchestra_text")) {
      warn_unrecognized_field("song", field_name);
      return false;
    }
  }
  return true;
};