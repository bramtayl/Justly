#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <fluidsynth.h>
#include <nlohmann/json.hpp>
#include <set>

template <typename T> class QList;

[[nodiscard]] auto
get_soundfont_id(fluid_synth_t *synth_pointer) -> unsigned int;

[[nodiscard]] auto get_skip_names() -> const std::set<QString> &;

[[nodiscard]] auto get_percussion_set_names() -> const std::set<QString> &;

struct Instrument {
  QString name = "";
  short bank_number = -1;
  short preset_number = -1;
};

[[nodiscard]] auto get_all_instruments() -> const QList<Instrument> &;

[[nodiscard]] auto get_instrument_pointer(const QString &name = "") -> const Instrument *;

[[nodiscard]] auto instrument_pointer_is_default(const Instrument* instrument_pointer) -> bool;
[[nodiscard]] auto get_instrument_schema() -> nlohmann::json;
[[nodiscard]] auto instrument_pointer_to_json(const Instrument* instrument_pointer) -> nlohmann::json;
[[nodiscard]] auto json_to_instrument_pointer(const nlohmann::json &json_instrument) -> const Instrument*;

Q_DECLARE_METATYPE(const Instrument *);
