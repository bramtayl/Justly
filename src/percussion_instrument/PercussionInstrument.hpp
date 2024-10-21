#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <nlohmann/json.hpp>

template <typename T> class QList;

struct PercussionInstrument {
  QString name = "";
  short midi_number = -1;
};

[[nodiscard]] auto
get_all_percussion_instruments() -> const QList<PercussionInstrument> &;

[[nodiscard]] auto get_percussion_instrument_pointer(const QString &name = "")
    -> const PercussionInstrument *;

[[nodiscard]] auto percussion_instrument_pointer_is_default(const PercussionInstrument* percussion_instrument_pointer) -> bool;
[[nodiscard]] auto get_percussion_instrument_schema() -> nlohmann::json;
[[nodiscard]] auto percussion_instrument_pointer_to_json(const PercussionInstrument* percussion_instrument_pointer) -> nlohmann::json;
[[nodiscard]] auto json_to_percussion_instrument_pointer(const nlohmann::json &json_percussion_instrument) -> const PercussionInstrument*;

Q_DECLARE_METATYPE(const PercussionInstrument *);
