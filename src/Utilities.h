#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qstring.h>      // for QString

auto get_json_double(const QJsonObject &object, const QString &field_name,
                double a_default) -> double;
auto get_json_positive_double(const QJsonObject &object, const QString &field_name,
                         double a_default) -> double;
auto get_json_int(const QJsonObject &object, const QString &field_name, int a_default)
    -> int;
auto get_json_positive_int(const QJsonObject &object, const QString &field_name,
                      int a_default) -> int;
auto get_json_non_negative_int(const QJsonObject &object, const QString &field_name,
                          int a_default) -> int;
auto get_json_string(const QJsonObject &object, const QString &field_name,
                const QString &a_default = "") -> QString;

void error_not_json_object();