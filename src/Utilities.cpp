#include "Utilities.h"

#include <QtCore/qglobal.h>       // for qCritical
#include <QtCore/qtcoreexports.h> // for qUtf8Printable
#include <qbytearray.h>           // for QByteArray
#include <qcombobox.h>            // for QComboBox
#include <qjsonobject.h>          // for QJsonObject
#include <qjsonvalue.h>           // for QJsonValue
#include <qmessagebox.h>          // for QMessageBox
#include <qregularexpression.h>   // for QRegularExpressionMatchIteratorRan...
#include <qstring.h>              // for QString, operator+, operator==

#include <algorithm> // for any_of, max
#include <limits>    // for numeric_limits
#include <utility>   // for move

void json_parse_error(const QString &error_text) {
  QMessageBox::warning(nullptr, "JSON parsing error", error_text);
}

auto verify_json_string(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isString()) {
    json_parse_error(
        QString("Non-string %1: type %2!").arg(field_name).arg(json_value.type()));
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
    const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QJsonObject &json_object, const QString &field_name)
    -> bool {
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

auto verify_bounded(double value, const QString &field_name, double minimum,
                    double maximum) -> bool {
  if (value < minimum) {
    json_parse_error(QString("%1 of %2 less than minimum %3!")
                         .arg(field_name)
                         .arg(value)
                         .arg(minimum));
    return false;
  }
  if (value > maximum) {
    json_parse_error(QString("%1 of %2 greater than maximum %3!")
                         .arg(field_name)
                         .arg(value)
                         .arg(maximum));
    return false;
  }
  return true;
}

auto verify_bounded_double(const QJsonObject &json_object,
                           const QString &field_name, double minimum,
                           double maximum) -> bool {
  auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  auto value = json_value.toDouble();
  return verify_bounded(value, field_name, minimum, maximum);
}

auto verify_bounded_int(const QJsonObject &json_object,
                        const QString &field_name, double minimum,
                        double maximum) -> bool {
  auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  auto value = json_value.toDouble();
  if (!(verify_bounded(value, field_name, minimum, maximum))) {
    return false;
  }
  if ((value - static_cast<int>(value)) >
      std::numeric_limits<double>::epsilon()) {
    json_parse_error(QString("Non-whole %1 of %2!").arg(field_name).arg(value));
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

void error_empty(const QString &action) {
  qCritical("Nothing to %s!", qUtf8Printable(action));
}

void extract_instruments(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &orchestra_code) {
  QRegularExpression const instrument_pattern(R"(\binstr\s+\b(\w+)\b)");
  QRegularExpressionMatchIterator const instrument_matches =
      instrument_pattern.globalMatch(orchestra_code);
  for (const QRegularExpressionMatch &match : instrument_matches) {
    instrument_pointers.push_back(
        std::move(std::make_unique<QString>(match.captured(1))));
  }
}

void fill_combo_box(
    QComboBox &combo_box,
    const std::vector<std::unique_ptr<const QString>> &text_pointers,
    bool include_empty) {
  if (include_empty) {
    combo_box.addItem("");
  }
  for (const auto &text_pointer : text_pointers) {
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

void error_instrument(const QString &instrument) {
  QMessageBox::warning(
      nullptr, "Instrument warning",
      QString("Cannot find instrument %1! Not changing orchestra text")
          .arg(instrument));
  // qCritical("Cannot find instrument %s", qUtf8Printable(instrument));
}

auto require_json_field(const QJsonObject &json_object,
                        const QString &field_name) -> bool {
  if (!(json_object.contains(field_name))) {
    QMessageBox::warning(nullptr, "JSON parsing error",
                         QString("No field %1!").arg(field_name));
    return false;
  }
  return true;
}

void warn_unrecognized_field(const QString &level, const QString &field) {
  json_parse_error(QString("Unrecognized %1 field %2!").arg(level).arg(field));
}

void error_level(TreeLevel level) { qCritical("Invalid level %d!", level); }

auto get_capture_int(const QRegularExpressionMatch &match,
                     const QString &field_name, int default_value) -> int {
  auto text = match.captured(field_name);
  if (text.isNull()) {
    return default_value;
  }
  return text.toInt();
}

auto verify_regex_int(const QRegularExpressionMatch &match,
                      const QString &field_name, int minimum, int maximum) -> bool {
  auto text = match.captured(field_name);
  if (!(text.isNull())) {
    auto an_int = text.toInt();
    if (an_int < minimum) {
      json_parse_error(QString("%1 %2 is less than minimum %3!")
                           .arg(field_name)
                           .arg(an_int)
                           .arg(minimum));
      return false;
    }
    if (an_int > maximum) {
      json_parse_error(QString("%1 %2 is greater than maximum %3!")
                           .arg(field_name)
                           .arg(an_int)
                           .arg(minimum));
      return false;
    }
  }
  return true;
}
