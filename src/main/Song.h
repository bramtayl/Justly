#pragma once

#include <qvariant.h>

#include <gsl/pointers>           // for not_null
#include <nlohmann/json_fwd.hpp>  // for json

#include "main/TreeNode.h"         // for TreeNode
#include "metatypes/Instrument.h"  // for Instrument
#include "utilities/utilities.h"

class QByteArray;

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 50;
const auto DEFAULT_STARTING_TEMPO = 200;
const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";
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
  gsl::not_null<const Instrument*> starting_instrument_pointer =
      &(Instrument::get_instrument_by_name(DEFAULT_STARTING_INSTRUMENT));
  TreeNode root;

  [[nodiscard]] auto to_json() const -> nlohmann::json;

  [[nodiscard]] auto load_text(const QByteArray& song_text) -> bool;
  [[nodiscard]] auto get_starting_value(StartingFieldId value_type) const
      -> QVariant;
  void set_starting_value(StartingFieldId value_type,
                          const QVariant& new_value);
};
