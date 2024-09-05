#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QList> // IWYU pragma: keep
#include <cstdint>

struct PercussionInstrument {
  QString name;
  int16_t midi_number = -1;
};

[[nodiscard]] auto get_percussion_instrument_pointer(const QString &name)
    -> const PercussionInstrument *;

[[nodiscard]] auto
get_all_percussion_instruments() -> const QList<PercussionInstrument> &;

Q_DECLARE_METATYPE(const PercussionInstrument *);
