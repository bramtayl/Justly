#include "instrument/Instrument.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QList>
#include <QMap>
#include <QString>
#include <QtGlobal>
#include <algorithm>
#include <filesystem>
#include <set>
#include <string>

#include <fluidsynth.h>

[[nodiscard]] auto get_skip_names() -> const std::set<QString> & {
  static const std::set<QString> skip_names(
      {"Marching Snare", "OldMarchingBass", "Marching Cymbals", "Marching Bass",
       "OldMarchingTenor", "Marching Tenor",
       // dummy instruments
       "Basses Fast", "Basses Pizzicato", "Basses Slow", "Basses Tremolo",
       "Celli Fast", "Celli Pizzicato", "Celli Slow", "Celli Tremolo",
       "Violas Fast", "Violas Pizzicato", "Violas Slow", "Violas Tremolo",
       "Violins Fast", "Violins Pizzicato", "Violins Slow", "Violins Tremolo",
       "Violins2 Fast", "Violins2 Pizzicato", "Violins2 Slow",
       "Violins2 Tremolo",
       // expressive instruments
       "5th Saw Wave Expr.", "Accordion Expr.", "Alto Sax Expr.",
       "Atmosphere Expr.", "Bandoneon Expr.", "Baritone Sax Expr.",
       "Bass & Lead Expr.", "Basses Fast Expr.", "Basses Slow Expr.",
       "Basses Trem Expr.", "Bassoon Expr.", "Bottle Chiff Expr.",
       "Bowed Glass Expr.", "Brass 2 Expr.", "Brass Section Expr.",
       "Calliope Lead Expr.", "Celli Fast Expr.", "Celli Slow Expr.",
       "Celli Trem Expr.", "Cello Expr.", "Charang Expr.", "Chiffer Lead Expr.",
       "Choir Aahs Expr.", "Church Organ 2 Expr.", "Church Organ Expr.",
       "Clarinet Expr.", "Contrabass Expr.", "Detuned Org. 1 Expr.",
       "Detuned Org. 2 Expr.", "Detuned Saw Expr.", "Drawbar Organ Expr.",
       "Echo Drops Expr.", "English Horn Expr.", "Fiddle Expr.", "Flute Expr.",
       "French Horns Expr.", "Goblin Expr.", "Halo Pad Expr.",
       "Harmonica Expr.", "Hmn. Mute Tpt. Expr.", "It. Accordion Expr.",
       "Metal Pad Expr.", "Oboe Expr.", "Ocarina Expr.", "Pan Flute Expr.",
       "Perc. Organ Expr.", "Piccolo Expr.", "Polysynth Expr.",
       "Recorder Expr.", "Reed Organ Expr.", "Rock Organ Expr.",
       "Saw Lead Expr.", "Shakuhachi Expr.", "Shenai Expr.", "Sine Wave Expr.",
       "Slow Violin Expr.", "Solo Vox Expr.", "Soprano Sax Expr.",
       "Soundtrack Expr.", "Space Voice Expr.", "Square Lead Expr.",
       "Star Theme Expr.", "Strings Fast Expr.", "Strings Slow Expr.",
       "Strings Trem Expr.", "Sweep Pad Expr.", "Syn. Strings 1 Expr.",
       "Syn. Strings 2 Expr.", "Synth Brass 1 Expr.", "Synth Brass 2 Expr.",
       "Synth Brass 3 Expr.", "Synth Brass 4 Expr.", "Synth Strings3 Expr.",
       "Synth Voice Expr.", "Tenor Sax Expr.", "Trombone Expr.",
       "Trumpet Expr.", "Tuba Expr.", "Viola Expr.", "Violas Fast Expr.",
       "Violas Slow Expr.", "Violas Trem Expr.", "Violin Expr.",
       "Violins Fast Expr.", "Violins Slow Expr.", "Violins Trem Expr.",
       "Violins2 Fast Expr.", "Violins2 Slow Expr.", "Violins2 Trem Expr.",
       "Voice Oohs Expr.", "Warm Pad Expr.", "Whistle Expr.",
       // not working?
       "Temple Blocks"});
  return skip_names;
}

[[nodiscard]] auto get_percussion_set_names() -> const std::set<QString> & {
  static const std::set<QString> percussion_set_names(
      {"Brush 1",    "Brush 2",    "Brush",      "Electronic", "Jazz 1",
       "Jazz 2",     "Jazz 3",     "Jazz 4",     "Jazz",       "Orchestra Kit",
       "Power 1",    "Power 2",    "Power 3",    "Power",      "Room 1",
       "Room 2",     "Room 3",     "Room 4",     "Room 5",     "Room 6",
       "Room 7",     "Room",       "Standard 1", "Standard 2", "Standard 3",
       "Standard 4", "Standard 5", "Standard 6", "Standard 7", "Standard",
       "TR-808"});
  return percussion_set_names;
}

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

auto get_all_instruments() -> const QList<Instrument> & {
  static const QList<Instrument> all_instruments = []() -> QList<Instrument> {
    QList<Instrument> temp_instruments;

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
      if (skip_names.count(name) == 0 &&
          percussion_set_names.count(name) == 0) {
        temp_instruments.push_back(Instrument(
            {name, static_cast<short>(fluid_preset_get_banknum(preset_pointer)),
             static_cast<short>(fluid_preset_get_num(preset_pointer))}));
      }
      preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
    }

    delete_fluid_synth(synth_pointer);
    delete_fluid_settings(settings_pointer);

    std::sort(temp_instruments.begin(), temp_instruments.end(),
              [](const Instrument &instrument_1, const Instrument &instrument_2)
                  -> bool { return instrument_1.name <= instrument_2.name; });

    return temp_instruments;
  }();

  return all_instruments;
}

auto get_instrument_pointer(const QString &name) -> const Instrument * {
  static const auto instrument_map = []() -> QMap<QString, const Instrument *> {
    const QList<Instrument> &instruments = get_all_instruments();
    QMap<QString, const Instrument *> temp_map;
    for (const auto &instrument : instruments) {
      temp_map[instrument.name] = &instrument;
    }
    return temp_map;
  }();
  Q_ASSERT(instrument_map.count(name) == 1);
  return instrument_map.value(name);
}
