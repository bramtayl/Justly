#include "percussion_set/PercussionSet.hpp"

#include <QList>
#include <QtGlobal>
#include <nlohmann/json.hpp>

class QStringListModel;

PercussionSet::PercussionSet(const QString &name, short bank_number_input,
                             short preset_number_input)
    : Named({name}), bank_number(bank_number_input),
      preset_number(preset_number_input) {}

auto variant_to_percussion_set(const QVariant &variant)
    -> const PercussionSet * {
  Q_ASSERT(variant.canConvert<const PercussionSet *>());
  return variant.value<const PercussionSet *>();
}

auto get_all_percussion_sets() -> const QList<PercussionSet> & {
  static const auto all_percussion_sets = []() {
    QList<PercussionSet> temp_percussion_sets;
    fill_instruments(temp_percussion_sets, true);
    return temp_percussion_sets;
  }();

  return all_percussion_sets;
}

auto get_percussion_set_schema() -> nlohmann::json {
  return nlohmann::json({{"type", "string"},
                         {"description", "the unpitched_note set"},
                         {"enum", get_names(get_all_percussion_sets())}});
};

[[nodiscard]] auto get_percussion_set_names_model() -> QStringListModel & {
  static auto percussion_sets_model = get_list_model(get_all_percussion_sets());
  return percussion_sets_model;
}