#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>

template <typename T> class QList;

struct PercussionInstrument {
  QString name;
  short midi_number = -1;
};

[[nodiscard]] auto
get_all_percussion_instruments() -> const QList<PercussionInstrument> &;

[[nodiscard]] auto get_percussion_instrument_pointer(const QString &name)
    -> const PercussionInstrument *;

Q_DECLARE_METATYPE(const PercussionInstrument *);
