#pragma once

#include <qabstractitemmodel.h>  // for QModelIndexList
#include <qjsonobject.h>         // for QJsonObject
#include <qstring.h>             // for QString
#include <stddef.h>              // for size_t

#include <memory>  // for unique_ptr
#include <vector>  // for vector

auto get_json_double(const QJsonObject &object, const QString &field_name,
                     double a_default) -> double;
auto get_json_positive_double(const QJsonObject &object,
                              const QString &field_name, double a_default)
    -> double;
auto get_json_int(const QJsonObject &object, const QString &field_name,
                  int a_default) -> int;
auto get_json_positive_int(const QJsonObject &object, const QString &field_name,
                           int a_default) -> int;
auto get_json_non_negative_int(const QJsonObject &object,
                               const QString &field_name, int a_default) -> int;
auto get_json_string(const QJsonObject &object, const QString &field_name,
                     const QString &a_default = "") -> QString;

void error_not_json_object();

void cannot_open_error(const QString &filename);

void no_instrument_error(const QString &instrument);

auto get_instruments(const QString &orchestra_text)
    -> std::vector<std::unique_ptr<const QString>>;

auto has_instrument(
    const std::vector<std::unique_ptr<const QString>> &instruments,
    const QString &maybe_instrument) -> bool;

void error_row(size_t row);

void error_column(int column);

void assert_not_empty(const QModelIndexList &selected);