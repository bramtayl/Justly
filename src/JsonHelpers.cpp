#include "JsonHelpers.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/std_abs.h>          // for abs
#include <qbytearray.h>            // for QByteArray
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValue
#include <qstring.h>               // for QString

#include <cstdlib>  // for abs
#include <limits>   // for numeric_limits

auto get_string(const QJsonObject &object, const QString &name,
                const QString &a_default) -> QString {
  if (!object.contains(name)) {
    return a_default;
  }
  auto json_field = object[name];
  if (!json_field.isString()) {
    qCritical("Non-string field %s!", qUtf8Printable(name));
  }
  return json_field.toString();
}

auto get_double(const QJsonObject &object, const QString &name,
                double a_default) -> double {
  if (!object.contains(name)) {
    return a_default;
  }
  auto json_field = object[name];
  if (!json_field.isDouble()) {
    qCritical("Non-numeric field %s!", qUtf8Printable(name));
  }
  return json_field.toDouble();
}

auto get_positive_double(const QJsonObject &object, const QString &name,
                         double a_default) -> double {
  auto double_field = get_double(object, name, a_default);
  if (!(double_field > 0)) {
    qCritical("Non-positive %s: %f!", qUtf8Printable(name), double_field);
  }
  return double_field;
}

auto get_int(const QJsonObject &object, const QString &name, int a_default)
    -> int {
  auto double_field = get_double(object, name, a_default * 1.0);
  auto int_field = static_cast<int>(double_field);
  if (!(abs(double_field - int_field) <=
        std::numeric_limits<double>::epsilon())) {
    qCritical("Non-integer field %s: %f!", qUtf8Printable(name), double_field);
  }
  return static_cast<int>(double_field);
}

auto get_positive_int(const QJsonObject &object, const QString &name,
                      int a_default) -> int {
  auto int_field = get_int(object, name, a_default);
  if (!(int_field > 0)) {
    qCritical("Non-positive %s: %i!", qUtf8Printable(name), int_field);
  }
  return int_field;
}

auto get_non_negative_int(const QJsonObject &object, const QString &name,
                          int a_default) -> int {
  auto int_field = get_int(object, name, a_default);
  if (!(int_field >= 0)) {
    qCritical("Negative %s: %i!", qUtf8Printable(name), int_field);
  }
  return int_field;
}
