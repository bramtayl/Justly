#pragma once

#include <qjsonobject.h>         // for QJsonObject
#include <qstring.h>             // for QString

#include <nlohmann/json.hpp>
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

void show_open_error(const QString &filename);

void show_parse_error(const nlohmann::json::parse_error& parse_error);

enum CommandId {
  starting_key_change_id = 0,
  starting_volume_change_id = 1,
  starting_tempo_change_id = 2,
  starting_instrument_change_id = 3
};
