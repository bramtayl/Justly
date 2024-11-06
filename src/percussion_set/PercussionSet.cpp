#include "percussion_set/PercussionSet.hpp"

#include <utility>

PercussionSet::PercussionSet(QString name, short bank_number,
                             short preset_number)
    : AbstractInstrument(std::move(name), bank_number, preset_number) {}

auto PercussionSet::get_all_nameds() -> const QList<PercussionSet> & {
  static const auto all_percussion_sets = fill_instruments<PercussionSet>(true);
  return all_percussion_sets;
}
