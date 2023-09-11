#pragma once

#include <qjsonobject.h>         // for QJsonObject
#include <qregularexpression.h>  // for QRegularExpressionMatch
#include <qstring.h>             // for QString

#include <nlohmann/json_fwd.hpp>  // for jsom

[[nodiscard]] auto get_json_double(const QJsonObject &object,
                                   const QString &field_name, double a_default)
    -> double;
[[nodiscard]] auto get_json_int(const QJsonObject &object,
                                const QString &field_name, int a_default)
    -> int;
[[nodiscard]] auto get_json_string(const QJsonObject &object,
                                   const QString &field_name,
                                   const QString &a_default = "") -> QString;

void cannot_open_error(const QString &filename);

[[nodiscard]] auto get_capture_int(const QRegularExpressionMatch &match,
                     const QString &field_name, int default_value) -> int;

auto parse_json(nlohmann::json& parsed_json, const QString& song_text) -> bool;


