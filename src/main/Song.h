#pragma once

#include <qvariant.h>

#include <gsl/pointers>  // for not_null
#include <memory>
#include <nlohmann/json_fwd.hpp>  // for json

#include "main/TreeNode.h"         // for TreeNode
#include "metatypes/Instrument.h"  // for Instrument

enum StartingFieldId {
  starting_key_id = 0,
  starting_volume_id = 1,
  starting_tempo_id = 2,
  starting_instrument_id = 3
};

const auto MINIMUM_STARTING_KEY = 60;
const auto DEFAULT_STARTING_KEY = 220;
const auto MAXIMUM_STARTING_KEY = 440;

const auto MINIMUM_STARTING_VOLUME = 1;
const auto DEFAULT_STARTING_VOLUME = 90;
const auto MAXIMUM_STARTING_VOLUME = 100;

const auto MINIMUM_STARTING_TEMPO = 100;
const auto DEFAULT_STARTING_TEMPO = 200;
const auto MAXIMUM_STARTING_TEMPO = 800;

const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

class Song {
 public:
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_volume = DEFAULT_STARTING_VOLUME;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  gsl::not_null<const Instrument*> starting_instrument_pointer =
      &(Instrument::get_instrument_by_name(DEFAULT_STARTING_INSTRUMENT));
  TreeNode root;

  [[nodiscard]] auto to_json() const -> nlohmann::json;
  [[nodiscard]] static auto verify_json(const nlohmann::json& parsed_json)
      -> bool;

  void load_controls(const nlohmann::json& parsed_json);
  [[nodiscard]] auto get_starting_value(StartingFieldId value_type) const
      -> QVariant;
  void set_starting_value(StartingFieldId value_type,
                          const QVariant& new_value);
};
