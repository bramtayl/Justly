#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <string>
#include <vector>

struct PercussionSet {
  std::string name;
  int16_t bank_number = -1;
  int16_t preset_number = -1;
  bool is_percussion = false;
};

[[nodiscard]] auto
get_percussion_set_pointer(const std::string &name) -> const PercussionSet *;

[[nodiscard]] auto
get_all_percussion_sets() -> const std::vector<PercussionSet> &;

Q_DECLARE_METATYPE(const PercussionSet *);
