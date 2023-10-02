#pragma once

#include <qstring.h>  // for QString

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for jsom

void show_open_error(const QString& filename);

void show_parse_error(const nlohmann::json::parse_error& parse_error);

enum StartingFieldId {
  starting_key_id = 0,
  starting_volume_id = 1,
  starting_tempo_id = 2,
  starting_instrument_id = 3
};
