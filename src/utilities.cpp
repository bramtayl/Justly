#include "utilities.h"

#include <qjsondocument.h>
#include <qjsonobject.h>         // for QJsonObject
#include <qjsonvalue.h>          // for QJsonValue
#include <qobject.h>
#include <qmessagebox.h>         // for QMessageBox
#include <qregularexpression.h>  // for QRegularExpressionMatch
#include <qstring.h>             // for QString, operator==, operator+

#include <limits>  // for numeric_limits

void parse_error(const QString &error_text) {
  QMessageBox::warning(nullptr, QObject::tr("Parsing error"),
                       error_text);
}

auto verify_json_string(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isString()) {
    parse_error(QObject::tr("%1 of type %2 not a string!")
                    .arg(field_name)
                    .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_double(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isDouble()) {
    parse_error(QObject::tr("%1 of type %2 not a double!")
                    .arg(field_name)
                    .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_array(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isArray()) {
    parse_error(QObject::tr("%1 of type %2 not an array!")
                    .arg(field_name)
                    .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_object(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isObject()) {
    parse_error(QObject::tr("%1 of type %2 not an object!")
                    .arg(field_name)
                    .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_bounded(double value, const QString &field_name, double minimum,
                    double maximum) -> bool {
  if (value < minimum) {
    parse_error(QObject::tr("%1 of %2 less than the minimum of %3!")
                    .arg(field_name)
                    .arg(value)
                    .arg(minimum));
    return false;
  }
  if (value > maximum) {
    parse_error(QObject::tr("%1 of %2 greater than the maximum of %3!")
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
    parse_error(QObject::tr("%1 of %2 not an integer!").arg(field_name).arg(value));
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
  QMessageBox::warning(
      nullptr, QObject::tr("File error"),
      QObject::tr("Cannot open file \"%1\"!").arg(filename));
}

auto require_json_field(const QJsonObject &json_object,
                        const QString &field_name) -> bool {
  if (!(json_object.contains(field_name))) {
    parse_error(QObject::tr("No field %1!").arg(field_name));
    return false;
  }
  return true;
}

void warn_unrecognized_field(const QString &level, const QString &field) {
  parse_error(QObject::tr("Unrecognized %1 field \"%2\"!").arg(level).arg(field));
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
      parse_error(QObject::tr("%1 of %2 is less than the minimum of %3!")
                      .arg(field_name)
                      .arg(an_int)
                      .arg(minimum));
      return false;
    }
    if (an_int > maximum) {
      parse_error(QObject::tr("%1 of %2 is greater than the maximum of %3!")
                      .arg(field_name)
                      .arg(an_int)
                      .arg(minimum));
      return false;
    }
  }
  return true;
}

auto verify_json_document(const QJsonDocument &document) -> bool {
  if (document.isNull()) {
    parse_error(QObject::tr("Cannot parse JSON!"));
    return false;
  }
  return true;
}