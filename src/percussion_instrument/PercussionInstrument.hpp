#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <nlohmann/json.hpp>

template <typename T> class QList;

struct PercussionInstrument {
  QString name;
  short midi_number;
};

[[nodiscard]] auto
get_all_percussion_instruments() -> const QList<PercussionInstrument> &;

[[nodiscard]] auto get_percussion_instrument_schema() -> nlohmann::json;

Q_DECLARE_METATYPE(const PercussionInstrument *);
