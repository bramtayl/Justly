#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <string>
#include <vector>

struct PercussionInstrument {
  std::string name;
  int16_t midi_number = -1;
};

[[nodiscard]] auto get_percussion_instrument_pointer(const std::string &name)
    -> const PercussionInstrument *;

[[nodiscard]] auto
get_all_percussion_instruments() -> const std::vector<PercussionInstrument> &;

Q_DECLARE_METATYPE(const PercussionInstrument *);
