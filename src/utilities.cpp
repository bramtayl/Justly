#include "utilities.h"

#include <qjsonobject.h>  // for QJsonObject
#include <qjsonvalue.h>   // for QJsonValue
#include <qmessagebox.h>  // for QMessageBox
#include <qobject.h>
#include <qstring.h>             // for QString, operator==, operator+

#include <nlohmann/json.hpp>

auto get_json_string(const QJsonObject& object, const QString& field_name,
                     const QString& a_default) -> QString {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toString();
}

auto get_json_double(const QJsonObject& object, const QString& field_name,
                     double a_default) -> double {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toDouble();
}

auto get_json_int(const QJsonObject& object, const QString& field_name,
                  int a_default) -> int {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toInt();
}

void show_open_error(const QString& filename) {
  QMessageBox::warning(nullptr, QObject::tr("File error"),
                       QObject::tr("Cannot open file \"%1\"!").arg(filename));
}

void show_parse_error(const nlohmann::json::parse_error& parse_error) {
  QMessageBox::warning(nullptr, QObject::tr("Parsing error"),
                         parse_error.what());
}
