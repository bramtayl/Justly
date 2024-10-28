#pragma once

#include "named/Named.hpp"
#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <fluidsynth.h>
#include <nlohmann/json.hpp>
#include <set>

template <typename T> class QList;

[[nodiscard]] auto get_soundfont_id(fluid_synth_t *synth_pointer) -> int;

[[nodiscard]] auto get_skip_names() -> const std::set<QString> &;

[[nodiscard]] auto get_percussion_set_names() -> const std::set<QString> &;

struct Instrument : public Named {
  short bank_number;
  short preset_number;
  Instrument(const QString &name, short bank_number_input,
             short preset_number_input);
};

[[nodiscard]] auto
variant_to_instrument(const QVariant &variant) -> const Instrument *;

[[nodiscard]] auto get_all_instruments() -> const QList<Instrument> &;

[[nodiscard]] auto get_instrument_schema() -> nlohmann::json;

Q_DECLARE_METATYPE(const Instrument *);
