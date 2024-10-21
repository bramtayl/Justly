#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <nlohmann/json.hpp>

template <typename T> class QList;

struct PercussionSet {
  QString name = "";
  short bank_number = -1;
  short preset_number = -1;
};

[[nodiscard]] auto
get_all_percussion_sets() -> const QList<PercussionSet> &;

[[nodiscard]] auto get_percussion_set_pointer(const QString &name = "")
    -> const PercussionSet *;

[[nodiscard]] auto percussion_set_pointer_is_default(const PercussionSet* percussion_set_pointer) -> bool;
[[nodiscard]] auto get_percussion_set_schema() -> nlohmann::json;
[[nodiscard]] auto percussion_set_pointer_to_json(const PercussionSet* percussion_set_pointer) -> nlohmann::json;
[[nodiscard]] auto json_to_percussion_set_pointer(const nlohmann::json &json_percussion_set) -> const PercussionSet*;

Q_DECLARE_METATYPE(const PercussionSet *);
