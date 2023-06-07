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

auto json_field_error(const QString &error, const QString &field_name) {
  QMessageBox::warning(nullptr, "JSON parsing error",
                       error + " " + field_name + "!");
}

auto verify_json_string(const QJsonObject &object, const QString &field_name)
    -> bool {
  auto json_field = object[field_name];
  if (!json_field.isString()) {
    json_field_error("Non-string", field_name);
    return false;
  }
  return true;
}

auto verify_json_double(const QJsonObject &object, const QString &field_name)
    -> bool {
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_field_error("Non-double", field_name);
    return false;
  }
  return true;
}

auto verify_positive_json_double(const QJsonObject &object,
                                 const QString &field_name) -> bool {
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_field_error("Non-double", field_name);
    return false;
  }
  if (json_field.toDouble() <= 0) {
    json_field_error("Non positive", field_name);
    return false;
  }
  return true;
}

auto verify_non_negative_json_double(const QJsonObject &object,
                                     const QString &field_name) -> bool {
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_field_error("Non-double", field_name);
    return false;
  }
  if (json_field.toDouble() < 0) {
    json_field_error("Non positive", field_name);
    return false;
  }
  return true;
}

auto verify_positive_percent_json_double(const QJsonObject &object,
                                         const QString &field_name) -> bool {
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_field_error("Non-double", field_name);
    return false;
  }
  if (json_field.toDouble() <= 0) {
    json_field_error("Non negative", field_name);
    return false;
  }
  if (json_field.toDouble() > 100) {
    json_field_error("Percent must be <= 100", field_name);
    return false;
  }
  return true;
}

auto verify_json_int(const QJsonObject &object, const QString &field_name)
    -> bool {
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_field_error("Non-double", field_name);
    return false;
  }
  auto double_field = json_field.toDouble();
  auto int_field = static_cast<int>(double_field);
  if (trunc(double_field) != double_field) {
    json_field_error("Non-whole", field_name);
    return false;
  }
  return true;
}

auto verify_json_positive_int(const QJsonObject &object,
                              const QString &field_name) -> bool {
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_field_error("Non-double", field_name);
    return false;
  }
  auto double_field = json_field.toDouble();
  auto int_field = static_cast<int>(double_field);
  if (trunc(double_field) != double_field) {
    json_field_error("Non-whole", field_name);
    return false;
  }
  if (int_field < 0) {
    json_field_error("Negative", field_name);
    return false;
  }
  return true;
}

auto verify_json_non_negative_int(const QJsonObject &object,
                                  const QString &field_name) -> bool {
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_field_error("Non-double", field_name);
    return false;
  }
  auto double_field = json_field.toDouble();
  auto int_field = static_cast<int>(double_field);
  if (trunc(double_field) != double_field) {
    json_field_error("Non-whole", field_name);
    return false;
  }
  if (int_field <= 0) {
    json_field_error("Non-negative", field_name);
    return false;
  }
  return true;
}

auto get_json_string(const QJsonObject &object, const QString &field_name,
                     const QString &a_default) -> QString {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toString();
}

auto get_json_double(const QJsonObject &object, const QString &field_name,
                     double a_default) -> double {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toDouble();
}

auto get_json_int(const QJsonObject &object, const QString &field_name,
                  int a_default) -> int {
  auto double_field = get_json_double(object, field_name, a_default * 1.0);
  auto int_field = static_cast<int>(double_field);
  if (!(abs(double_field - int_field) <=
        std::numeric_limits<double>::epsilon())) {
    json_field_error("Non-integer", field_name);
    return a_default;
  }
  return static_cast<int>(std::round(double_field));
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

auto verify_json_instrument(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &instrument) -> bool {
  if (!has_instrument(instrument_pointers, instrument)) {
    QMessageBox::warning(nullptr, "JSON parsing error",
                         QString("Cannot find instrument %1").arg(instrument));
    return false;
  }
  return true;
}

auto verify_json(const QJsonObject &json_song) -> bool {
  std::vector<std::unique_ptr<const QString>> instrument_pointers;
  if (!(json_song.contains("orchestra_text"))) {
    QMessageBox::warning(nullptr, "JSON parsing error", "No orchestra!");
    return false;
  }
  const auto orchestra_object = json_song["orchestra_text"];
  if (!orchestra_object.isString()) {
    QMessageBox::warning(nullptr, "JSON parsing error",
                         "Non-string orchestra!");
    return false;
  }
  const auto orchestra_text = orchestra_object.toString();
  if (!(json_song.contains("default_instrument"))) {
    QMessageBox::warning(nullptr, "JSON parsing error",
                         "No default instrument!");
    return false;
  }
  const auto default_instrument_object = json_song["default_instrument"];
  if (!(default_instrument_object.isString())) {
    QMessageBox::warning(nullptr, "JSON parsing error",
                         "Non-string default instrument!");
    return false;
  }
  const auto default_instrument = default_instrument_object.toString();
  extract_instruments(instrument_pointers,
                      json_song["orchestra_text"].toString());

  if (!verify_json_instrument(instrument_pointers, default_instrument)) {
    return false;
  }

  for (const auto &field_name : json_song.keys()) {
    if (field_name == "frequency") {
      if (!(verify_positive_json_double(json_song, field_name))) {
        return false;
      }
    } else if (field_name == "volume_percent") {
      if (!(verify_positive_percent_json_double(json_song, field_name))) {
        return false;
      }
    } else if (field_name == "tempo") {
      if (!(verify_positive_json_double(json_song, field_name))) {
        return false;
      }
    } else if (field_name == "default_instrument") {
      if (!(verify_json_string(json_song, field_name))) {
        return false;
      }
    } else if (field_name == "children") {
      auto chords_object = json_song[field_name];
      if (!chords_object.isArray()) {
        QMessageBox::warning(nullptr, "JSON parsing error",
                             QString("Chords must be an array"));
        return false;
      }
      const auto json_chords = chords_object.toArray();
      for (const auto &json_chord_thing : json_chords) {
        if (!(json_chord_thing.isObject())) {
          QMessageBox::warning(nullptr, "JSON parsing error",
                               "Non-object chord!");
          return false;
        }
        const auto json_chord = json_chord_thing.toObject();
        for (const auto &field_name : json_chord.keys()) {
          if (field_name == "numerator") {
            if (!(verify_json_positive_int(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "denominator") {
            if (!(verify_json_positive_int(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "octave") {
            if (!(verify_json_int(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "beats") {
            if (!(verify_json_non_negative_int(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "volume_percent") {
            if (!(verify_positive_percent_json_double(json_chord,
                                                      field_name))) {
              return false;
            }
          } else if (field_name == "tempo_percent") {
            if (!(verify_positive_percent_json_double(json_chord,
                                                      field_name))) {
              return false;
            }
          } else if (field_name == "words") {
            if (!(verify_json_string(json_chord, field_name))) {
              return false;
            }
          } else if (field_name == "children") {
            const auto notes_object = json_chord[field_name];
            if (!notes_object.isArray()) {
              QMessageBox::warning(nullptr, "JSON parsing error",
                                   QString("Notes must be an array"));
              return false;
            }
            const auto json_notes = notes_object.toArray();
            for (const auto &json_note_thing : json_notes) {
              if (!(json_note_thing.isObject())) {
                QMessageBox::warning(nullptr, "JSON parsing error",
                                     "Non-object note!");
                return false;
              }
              const auto json_note = json_note_thing.toObject();

              for (const auto &field_name : json_note.keys()) {
                if (field_name == "numerator") {
                  if (!(verify_json_positive_int(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "denominator") {
                  if (!(verify_json_positive_int(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "octave") {
                  if (!(verify_json_int(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "beats") {
                  if (!(verify_json_non_negative_int(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "volume_percent") {
                  if (!(verify_positive_percent_json_double(json_note,
                                                            field_name))) {
                    return false;
                  }
                } else if (field_name == "tempo_percent") {
                  if (!(verify_positive_percent_json_double(json_note,
                                                            field_name))) {
                    return false;
                  }
                } else if (field_name == "words") {
                  if (!(verify_json_string(json_note, field_name))) {
                    return false;
                  }
                } else if (field_name == "instrument") {
                  const auto instrument_object = json_note["instrument"];
                  if (!instrument_object.isString()) {
                    QMessageBox::warning(nullptr, "JSON parsing error",
                                         "Non-string default instrument!");
                    return false;
                  }
                  const auto instrument = instrument_object.toString();
                  if (!verify_json_instrument(instrument_pointers,
                                              default_instrument)) {
                    return false;
                  }
                } else {
                  QMessageBox::warning(
                      nullptr, "JSON parsing error",
                      QString("Unrecognized note field %1").arg(field_name));
                  return false;
                }
              }
            }
          } else {
            QMessageBox::warning(
                nullptr, "JSON parsing error",
                QString("Unrecognized chord field %1").arg(field_name));
            return false;
          }
        }
      }
    } else if (!(field_name == "orchestra_text" ||
                 field_name == "default_instrument")) {
      QMessageBox::warning(
          nullptr, "JSON parsing error",
          QString("Unrecognized song field %1").arg(field_name));
      return false;
    }
  }
  return true;
};