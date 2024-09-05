#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <fluidsynth.h>

[[nodiscard]] auto
get_soundfont_id(fluid_synth_t *synth_pointer) -> unsigned int;

[[nodiscard]] auto get_skip_names() -> const std::set<std::string> &;

[[nodiscard]] auto get_percussion_set_names() -> const std::set<std::string> &;

struct Instrument {
  std::string name;
  int16_t bank_number = -1;
  int16_t preset_number = -1;
};

[[nodiscard]] auto
get_instrument_pointer(const std::string &name) -> const Instrument *;

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;

Q_DECLARE_METATYPE(const Instrument *);
