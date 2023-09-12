#include "utilities.h"

#include <qjsonobject.h>  // for QJsonObject
#include <qjsonvalue.h>   // for QJsonValue
#include <qmessagebox.h>  // for QMessageBox
#include <qobject.h>
#include <qregularexpression.h>  // for QRegularExpressionMatch
#include <qstring.h>             // for QString, operator==, operator+

#include <initializer_list>  // for initializer_list
#include <map>               // for operator!=, operator==
#include <nlohmann/json.hpp>
#include <vector>  // for vector

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

void cannot_open_error(const QString& filename) {
  QMessageBox::warning(nullptr, QObject::tr("File error"),
                       QObject::tr("Cannot open file \"%1\"!").arg(filename));
}

auto get_capture_int(const QRegularExpressionMatch& match,
                     const QString& field_name, int default_value) -> int {
  auto text = match.captured(field_name);
  if (text.isNull()) {
    return default_value;
  }
  return text.toInt();
}

auto parse_json(nlohmann::json& parsed_json, const QString& song_text) -> bool {
  try {
    parsed_json = nlohmann::json::parse(song_text.toStdString());
  } catch (const nlohmann::json::parse_error& parse_error) {
    QMessageBox::warning(nullptr, QObject::tr("Parsing error"),
                         parse_error.what());
    return false;
  }
  return true;
}
