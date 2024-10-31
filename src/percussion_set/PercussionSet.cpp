#include "percussion_set/PercussionSet.hpp"

#include <QList>
#include <QtGlobal>
#include <utility>

PercussionSet::PercussionSet(QString name, short bank_number,
                             short preset_number)
    : AbstractInstrument(std::move(name), bank_number,
      preset_number) {}

auto variant_to_percussion_set(const QVariant &variant)
    -> const PercussionSet * {
  Q_ASSERT(variant.canConvert<const PercussionSet *>());
  return variant.value<const PercussionSet *>();
}

auto PercussionSet::get_all_nameds() -> const QList<PercussionSet> & {
  static const auto all_percussion_sets = []() {
    QList<PercussionSet> temp_percussion_sets;
    fill_instruments(temp_percussion_sets, true);
    return temp_percussion_sets;
  }();

  return all_percussion_sets;
}
