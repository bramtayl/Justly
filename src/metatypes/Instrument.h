#pragma once

#include <string>
#include <vector>

#include <qobject.h>
#include "qstring.h"

class Instrument {
 public:
  QString instrument_name;
  int bank_number;
  int preset_number;
  int instrument_id;
  explicit Instrument(QString name_input = "",
                      int bank_number_input = -1, int preset_number_input = -1,
                      int instrument_id_input = -1);
  [[nodiscard]] auto operator==(const Instrument &other_interval) const -> bool;
  [[nodiscard]] static auto get_all_instruments() -> const std::vector<Instrument> &;
  [[nodiscard]] static auto get_all_instrument_names() -> const std::vector<std::string>&;
  [[nodiscard]] static auto get_instrument_by_name(const QString &instrument_name) -> Instrument;
  [[nodiscard]] static auto instrument_exists(const QString &maybe_instrument) -> bool;
  [[nodiscard]] auto get_text() const -> QString;
};

Q_DECLARE_METATYPE(Instrument)