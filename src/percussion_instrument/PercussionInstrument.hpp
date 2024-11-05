#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>

#include "named/Named.hpp"

template <typename T> class QList;

struct PercussionInstrument : public Named {
  short midi_number;
  PercussionInstrument(const QString &name, short midi_number);
  [[nodiscard]] static auto
  get_all_nameds() -> const QList<PercussionInstrument> &;
};

Q_DECLARE_METATYPE(const PercussionInstrument *);
