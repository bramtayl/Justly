#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <string>
#include <vector>

struct Percussion {
  std::string name;
  int16_t midi_number = -1;
};

[[nodiscard]] auto get_percussion_pointer(
    const std::string &name) -> const Percussion *;

[[nodiscard]] auto get_all_percussions() -> const std::vector<Percussion> &;

Q_DECLARE_METATYPE(const Percussion *);
