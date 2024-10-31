#include "other/AbstractInstrument.hpp"

#include <QCoreApplication>
#include <QDir>
#include <filesystem>
#include <utility>

#include "named/Named.hpp"

[[nodiscard]] auto get_soundfont_id(fluid_synth_t *synth_pointer) -> int {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath("../share/MuseScore_General.sf2")
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

AbstractInstrument::AbstractInstrument(QString name, short bank_number_input,
                       short preset_number_input)
    : Named({std::move(name)}), bank_number(bank_number_input),
      preset_number(preset_number_input) {}
