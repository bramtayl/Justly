#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVariant>

#include "other/AbstractInstrument.hpp"

template <typename T> class QList;

struct PercussionSet : public AbstractInstrument {
  PercussionSet(QString name, short bank_number, short preset_number);
};

[[nodiscard]] auto
variant_to_percussion_set(const QVariant &variant) -> const PercussionSet *;

[[nodiscard]] auto get_all_percussion_sets() -> const QList<PercussionSet> &;

Q_DECLARE_METATYPE(const PercussionSet *);
