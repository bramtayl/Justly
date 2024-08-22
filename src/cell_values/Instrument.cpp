#include "cell_values/Instrument.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QtGlobal>
#include <algorithm>
#include <filesystem>
#include <map>
#include <vector>

#include <fluidsynth.h>

[[nodiscard]] auto
get_soundfont_id(fluid_synth_t *synth_pointer) -> unsigned int {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath("../share/MuseScore_General.sf2")
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

auto get_instrument_pointer(const std::string &name) -> const Instrument * {
  static const auto instrument_map = []() {
    const std::vector<Instrument> &instruments = get_all_instruments();
    std::map<std::string, const Instrument *> temp_map;
    for (const auto &instrument : instruments) {
      temp_map[instrument.name] = &instrument;
    }
    return temp_map;
  }();
  Q_ASSERT(instrument_map.count(name) == 1);
  return instrument_map.at(name);
}

auto instrument_is_default(const Instrument &instrument) -> bool {
  return instrument.name.empty();
}

auto get_all_instruments() -> const std::vector<Instrument> & {
  static const std::vector<Instrument> all_instruments = []() {
    std::vector<Instrument> temp_instruments = {Instrument({"", -1, -1})};

    auto *settings_pointer = new_fluid_settings();
    auto *synth_pointer = new_fluid_synth(settings_pointer);
    auto soundfont_id = get_soundfont_id(synth_pointer);

    fluid_sfont_t *soundfont_pointer = fluid_synth_get_sfont_by_id(
        synth_pointer, static_cast<int>(soundfont_id));
    Q_ASSERT(soundfont_pointer != nullptr);

    fluid_sfont_iteration_start(soundfont_pointer);
    auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);

    while (preset_pointer != nullptr) {
      // TODO: add filter to remove broken instruments
      temp_instruments.push_back(Instrument(
          {std::string(fluid_preset_get_name(preset_pointer)),
           static_cast<int16_t>(fluid_preset_get_banknum(preset_pointer)),
           static_cast<int16_t>(fluid_preset_get_num(preset_pointer))}));
      preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
    }

    delete_fluid_synth(synth_pointer);
    delete_fluid_settings(settings_pointer);

    std::sort(
        temp_instruments.begin(), temp_instruments.end(),
        [](const Instrument &instrument_1, const Instrument &instrument_2) {
          return instrument_1.name <= instrument_2.name;
        });

    return temp_instruments;
  }();

  return all_instruments;
}
