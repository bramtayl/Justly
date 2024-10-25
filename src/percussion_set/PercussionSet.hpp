#pragma once

#include "named/Named.hpp"
#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

template <typename T> class QList;

struct PercussionSet : public Named {
  short bank_number;
  short preset_number;
  PercussionSet(const QString& name, short bank_number_input, short preset_number_input);
};

[[nodiscard]] auto
variant_to_percussion_set(const QVariant &variant) -> const PercussionSet *;

[[nodiscard]] auto get_all_percussion_sets() -> const QList<PercussionSet> &;

[[nodiscard]] auto get_percussion_set_schema() -> nlohmann::json;

Q_DECLARE_METATYPE(const PercussionSet *);
