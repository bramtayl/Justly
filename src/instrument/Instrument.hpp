#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <set>

#include <fluidsynth.h>

template <typename T> class QList;

[[nodiscard]] auto
get_soundfont_id(fluid_synth_t *synth_pointer) -> unsigned int;

[[nodiscard]] auto get_skip_names() -> const std::set<QString> &;

[[nodiscard]] auto get_percussion_set_names() -> const std::set<QString> &;

struct Instrument {
  QString name;
  short bank_number = -1;
  short preset_number = -1;
};

[[nodiscard]] auto get_all_instruments() -> const QList<Instrument> &;

[[nodiscard]] auto get_instrument_pointer(const QString &name) -> const Instrument *;

Q_DECLARE_METATYPE(const Instrument *);
