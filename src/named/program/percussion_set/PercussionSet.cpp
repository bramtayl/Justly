#include "named/program/percussion_set/PercussionSet.hpp"

PercussionSet::PercussionSet(const char* name, short bank_number,
                             short preset_number)
    : Program(name, bank_number, preset_number) {}

auto PercussionSet::get_all_nameds() -> const QList<PercussionSet> & {
  static const auto all_percussion_sets = get_programs<PercussionSet>(true);
  return all_percussion_sets;
}
