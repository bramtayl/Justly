#pragma once

#include <qjsonobject.h>         // for QJsonObject
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


auto parse_json(nlohmann::json &parsed_json, const QString &song_text) -> bool;

enum CommandIds {
  starting_key_change_id = 0,
  starting_volume_change_id = 1,
  starting_tempo_change_id = 2,
  starting_instrument_change_id = 3
};

