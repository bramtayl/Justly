#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Instrument {
  const std::string instrument_name;
  const int16_t bank_number;
  const int16_t preset_number;
  explicit Instrument(std::string name_input = "",
                      int16_t bank_number_input = -1,
                      int16_t preset_number_input = -1);

  [[nodiscard]] auto is_default() const -> bool;
};

Q_DECLARE_METATYPE(const Instrument *);

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;

[[nodiscard]] JUSTLY_EXPORT auto get_instrument_pointer(
    const std::string &instrument_name) -> const Instrument *;

[[nodiscard]] auto get_instrument_names() -> const std::vector<std::string> &;

[[nodiscard]] auto get_instrument_schema() -> nlohmann::json &;
