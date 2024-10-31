#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVariant>

#include "other/AbstractInstrument.hpp"

template <typename T> class QList;

struct Instrument : public AbstractInstrument {
  Instrument(QString name, short bank_number, short preset_number);
};

[[nodiscard]] auto
variant_to_instrument(const QVariant &variant) -> const Instrument *;

[[nodiscard]] auto get_all_instruments() -> const QList<Instrument> &;

Q_DECLARE_METATYPE(const Instrument *);
