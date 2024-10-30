#pragma once

#include <QList>
#include <QString>
#include <QStringListModel>
#include <QtGlobal>
#include <concepts>
#include <fluidsynth.h>
#include <iterator>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <utility>
#include <vector>

struct Named {
  QString name;
};

[[nodiscard]] auto get_name_or_empty(const Named *named_pointer) -> QString;

[[nodiscard]] auto get_soundfont_id(fluid_synth_t *synth_pointer) -> int;

[[nodiscard]] auto get_skip_names() -> const std::set<QString> &;

[[nodiscard]] auto get_percussion_set_names() -> const std::set<QString> &;

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto get_by_name(const QList<SubNamed> &nameds,
                               const QString &name) -> const SubNamed & {
  const auto named_pointer =
      std::find_if(nameds.cbegin(), nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto
get_names(const QList<SubNamed> &nameds) -> std::vector<std::string> {
  std::vector<std::string> names;
  std::transform(nameds.cbegin(), nameds.cend(), std::back_inserter(names),
                 [](const SubNamed &item) { return item.name.toStdString(); });
  return names;
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto
get_list_model(const QList<SubNamed> &nameds) -> QStringListModel {
  QList<QString> names({""});
  std::transform(nameds.cbegin(), nameds.cend(), std::back_inserter(names),
                 [](const SubNamed &item) { return item.name; });
  return QStringListModel(names);
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto
json_to_named(const QList<SubNamed> &nameds,
              const nlohmann::json &json_instrument) -> const SubNamed & {
  Q_ASSERT(json_instrument.is_string());
  return get_by_name(
      nameds, QString::fromStdString(json_instrument.get<std::string>()));
}

template <std::derived_from<Named> SubNamed>
void add_named_to_json(nlohmann::json &json_row, const SubNamed *named_pointer,
                       const char *column_name) {
  if (named_pointer != nullptr) {
    std::string named = named_pointer->name.toStdString();
    json_row[column_name] = std::move(named);
  }
}

template <std::derived_from<Named> SubNamed>
auto fill_instruments(QList<SubNamed>& nameds, bool is_percussion) {
  auto *settings_pointer = new_fluid_settings();
  auto *synth_pointer = new_fluid_synth(settings_pointer);

  fluid_sfont_t *soundfont_pointer = fluid_synth_get_sfont_by_id(
      synth_pointer, get_soundfont_id(synth_pointer));
  Q_ASSERT(soundfont_pointer != nullptr);

  fluid_sfont_iteration_start(soundfont_pointer);
  auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  const auto &skip_names = get_skip_names();
  const auto &percussion_set_names = get_percussion_set_names();

  while (preset_pointer != nullptr) {
    auto name = QString(fluid_preset_get_name(preset_pointer));
    auto named_is_percussion = percussion_set_names.contains(name);
    if (!skip_names.contains(name) && is_percussion ? named_is_percussion
                                                    : !named_is_percussion) {
      nameds.push_back(SubNamed(
          name, static_cast<short>(fluid_preset_get_banknum(preset_pointer)),
          static_cast<short>(fluid_preset_get_num(preset_pointer))));
    }
    preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  }

  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);

  std::sort(
        nameds.begin(), nameds.end(),
        [](const SubNamed &instrument_1, const SubNamed &instrument_2) {
          return instrument_1.name <= instrument_2.name;
        });
}
