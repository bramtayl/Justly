#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qstring.h>      // for QString

auto get_double(const QJsonObject &object, const QString &name,
                double a_default) -> double;
auto get_positive_double(const QJsonObject &object, const QString &name,
                         double a_default) -> double;
auto get_int(const QJsonObject &object, const QString &name, int a_default)
    -> int;
auto get_positive_int(const QJsonObject &object, const QString &name,
                      int a_default) -> int;
auto get_non_negative_int(const QJsonObject &object, const QString &name,
                          int a_default) -> int;
auto get_string(const QJsonObject &object, const QString &name,
                const QString &a_default = "") -> QString;