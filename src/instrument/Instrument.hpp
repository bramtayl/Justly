#pragma once

#include <QByteArray>
#include <QMetaType>
#include <set>
#include <QList>  // IWYU pragma: keep
#include <QString>

#include <cstdint>
#include <fluidsynth.h>

[[nodiscard]] auto
get_soundfont_id(fluid_synth_t *synth_pointer) -> unsigned int;

[[nodiscard]] auto get_skip_names() -> const std::set<QString> &;

[[nodiscard]] auto get_percussion_set_names() -> const std::set<QString> &;

struct Instrument {
  QString name;
  int16_t bank_number = -1;
  int16_t preset_number = -1;
};

[[nodiscard]] auto
get_instrument_pointer(const QString &name) -> const Instrument *;

[[nodiscard]] auto get_all_instruments() -> const QList<Instrument> &;

Q_DECLARE_METATYPE(const Instrument *);
