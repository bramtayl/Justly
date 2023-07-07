#include "utilities.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qcombobox.h>             // for QComboBox
#include <qjsondocument.h>
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValue
#include <qmessagebox.h>           // for QMessageBox
#include <qregularexpression.h>    // for QRegularExpressionMatch
#include <qstring.h>               // for QString, operator==, operator+

#include <limits>     // for numeric_limits

void json_parse_error(const QString &error_text) {
  QMessageBox::warning(nullptr, "JSON parsing error", error_text);
}

auto verify_json_string(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isString()) {
    json_parse_error(QString("Non-string %1: type %2!")
                         .arg(field_name)
                         .arg(json_value.type()));
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

void error_row(int row) { qCritical("Invalid row %d", static_cast<int>(row)); };

void error_column(int column) { qCritical("No column %d", column); }

void error_empty(const QString &action) {
  qCritical("Nothing to %s!", qUtf8Printable(action));
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

auto get_capture_int(const QRegularExpressionMatch &match,
                     const QString &field_name, int default_value) -> int {
  auto text = match.captured(field_name);
  if (text.isNull()) {
    return default_value;
  }
  return text.toInt();
}

auto verify_regex_int(const QRegularExpressionMatch &match,
                      const QString &field_name, int minimum, int maximum)
    -> bool {
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

auto verify_json_document(const QJsonDocument& document) -> bool {
  if (document.isNull()) {
    json_parse_error("Cannot parse JSON!");
    return false;
  }
  return true;
}