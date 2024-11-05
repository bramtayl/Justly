#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>

#include "other/AbstractInstrument.hpp"

template <typename T> class QList;

struct PercussionSet : public AbstractInstrument {
  PercussionSet(QString name, short bank_number, short preset_number);
  [[nodiscard]] static auto get_all_nameds() -> const QList<PercussionSet> &;
};

Q_DECLARE_METATYPE(const PercussionSet *);
