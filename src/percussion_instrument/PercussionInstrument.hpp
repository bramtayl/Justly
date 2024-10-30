#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QStringListModel>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "named/Named.hpp"

template <typename T> class QList;

struct PercussionInstrument : public Named {
  short midi_number;
  PercussionInstrument(const QString &name, short midi_number);
};

[[nodiscard]] auto variant_to_percussion_instrument(const QVariant &variant)
    -> const PercussionInstrument *;

[[nodiscard]] auto
get_all_percussion_instruments() -> const QList<PercussionInstrument> &;

[[nodiscard]] auto get_percussion_instrument_schema() -> nlohmann::json;

[[nodiscard]] auto
get_percussion_instrument_names_model() -> QStringListModel &;

Q_DECLARE_METATYPE(const PercussionInstrument *);
