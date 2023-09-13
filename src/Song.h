#pragma once

#include <qjsondocument.h>  // for QJsonDocument
#include <qstring.h>        // for QString

#include <memory>
#include <vector>  // for vector

#include "Instrument.h"  // for Instrument
#include "TreeNode.h"    // for TreeNode

namespace nlohmann::json_schema {
class json_validator;
}

class QByteArray;

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 50;
const auto DEFAULT_STARTING_TEMPO = 200;
const auto MINIMUM_STARTING_KEY = 60;
const auto MAXIMUM_STARTING_KEY = 440;
const auto MINIMUM_STARTING_VOLUME = 1;
const auto MAXIMUM_STARTING_VOLUME = 100;
const auto MINIMUM_STARTING_TEMPO = 100;
const auto MAXIMUM_STARTING_TEMPO = 800;

const auto SECONDS_PER_MINUTE = 60;
const auto FULL_NOTE_VOLUME = 0.2;

class Song {
 public:
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_volume = DEFAULT_STARTING_VOLUME;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QString starting_instrument;
  const std::vector<std::unique_ptr<Instrument>>& instrument_pointers = Instrument::get_all_instrument_pointers();
  TreeNode root;

  explicit Song(
      const QString &starting_instrument_input = "Marimba");

  [[nodiscard]] auto to_json() const -> QJsonDocument;

  [[nodiscard]] auto load_text(const QByteArray &song_text) -> bool;

  [[nodiscard]] auto get_instrument_id(const QString &instrument_name) const -> int;

  [[nodiscard]] auto has_instrument(const QString &maybe_instrument) const
      -> bool;

  [[nodiscard]] static auto get_validator()
      -> nlohmann::json_schema::json_validator &;
};
