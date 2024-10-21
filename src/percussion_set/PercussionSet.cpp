#include "percussion_set/PercussionSet.hpp"

#include <QList>
#include <QMap>
#include <QtGlobal>
#include <algorithm>
#include <fluidsynth.h>
#include <nlohmann/json.hpp>
#include <set>
#include <string>

#include "instrument/Instrument.hpp"
#include "other/other.hpp"

auto get_all_percussion_sets() -> const QList<PercussionSet> & {
  static const QList<PercussionSet> all_percussion_sets =
      []() -> QList<PercussionSet> {
    QList<PercussionSet> temp_percussion_sets({PercussionSet()});

    auto *settings_pointer = new_fluid_settings();
    auto *synth_pointer = new_fluid_synth(settings_pointer);

    fluid_sfont_t *soundfont_pointer = fluid_synth_get_sfont_by_id(
        synth_pointer, static_cast<int>(get_soundfont_id(synth_pointer)));
    Q_ASSERT(soundfont_pointer != nullptr);

    fluid_sfont_iteration_start(soundfont_pointer);
    auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
    const auto &skip_names = get_skip_names();
    const auto &percussion_set_names = get_percussion_set_names();

    while (preset_pointer != nullptr) {
      auto name = QString(fluid_preset_get_name(preset_pointer));
      auto bank_number =
          static_cast<short>(fluid_preset_get_banknum(preset_pointer));
      auto preset_number =
          static_cast<short>(fluid_preset_get_num(preset_pointer));
      if (skip_names.count(name) == 0 &&
          percussion_set_names.count(name) == 1) {
        temp_percussion_sets.push_back(
            PercussionSet({name, bank_number, preset_number}));
      }
      preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
    }

    delete_fluid_synth(synth_pointer);
    delete_fluid_settings(settings_pointer);

    std::sort(temp_percussion_sets.begin(), temp_percussion_sets.end(),
              [](const PercussionSet &percussion_set_1,
                 const PercussionSet &percussion_set_2) -> bool {
                return percussion_set_1.name <= percussion_set_2.name;
              });

    return temp_percussion_sets;
  }();

  return all_percussion_sets;
}

auto get_percussion_set_pointer(const QString &name) -> const PercussionSet * {
  static const auto percussion_set_map =
      []() -> QMap<QString, const PercussionSet *> {
    const QList<PercussionSet> &percussion_sets = get_all_percussion_sets();
    QMap<QString, const PercussionSet *> temp_map;
    for (const auto &percussion_set : percussion_sets) {
      temp_map[percussion_set.name] = &percussion_set;
    }
    return temp_map;
  }();
  Q_ASSERT(percussion_set_map.count(name) == 1);
  return percussion_set_map.value(name);
}

auto percussion_set_pointer_is_default(const PercussionSet* percussion_set_pointer) -> bool {
  Q_ASSERT(percussion_set_pointer != nullptr);
  return percussion_set_pointer->name.isEmpty();
};

auto get_percussion_set_schema() -> nlohmann::json {
  return nlohmann::json({{"type", "string"},
                         {"description", "the percussion set"},
                         {"enum", get_names(get_all_percussion_sets())}});
};

auto percussion_set_pointer_to_json(const PercussionSet* percussion_set_pointer) -> nlohmann::json {
  Q_ASSERT(percussion_set_pointer != nullptr);
  return percussion_set_pointer->name.toStdString();
}

auto json_to_percussion_set_pointer(const nlohmann::json &json_percussion_set)
    -> const PercussionSet * {
  Q_ASSERT(json_percussion_set.is_string());
  return get_percussion_set_pointer(
      QString::fromStdString(json_percussion_set.get<std::string>()));
  ;
}