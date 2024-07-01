#include "justly/Instrument.hpp"

#include <qassert.h>  // for Q_ASSERT

#include <algorithm>                     // for transform
#include <iterator>                      // for back_insert_iterator, back_i...
#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, basi...
#include <string>                        // for string, allocator, basic_string
#include <utility>                       // for move
#include <vector>                        // for vector

Instrument::Instrument(std::string name_input, int16_t bank_number_input,
                       int16_t preset_number_input)
    : instrument_name(std::move(name_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input) {}

auto Instrument::is_default() const -> bool { return !instrument_name.empty(); }

auto get_all_instruments() -> const std::vector<Instrument>& {
  static const std::vector<Instrument> all_instruments = {
      Instrument(""),
      Instrument("12-String Guitar", 8, 25),
      Instrument("5th Saw Wave", 0, 86),
      Instrument("808 Tom", 8, 118),
      Instrument("Accordion", 0, 21),
      Instrument("Acoustic Bass", 0, 32),
      Instrument("Agogo", 0, 113),
      Instrument("Alto Sax", 0, 65),
      Instrument("Applause", 0, 126),
      Instrument("Atmosphere", 0, 99),
      Instrument("Bagpipe", 0, 109),
      Instrument("Bandoneon", 0, 23),
      Instrument("Banjo", 0, 105),
      Instrument("Baritone Sax", 0, 67),
      Instrument("Bass & Lead", 0, 87),
      Instrument("Bassoon", 0, 70),
      Instrument("Bird Tweet", 0, 123),
      Instrument("Bottle Chiff", 0, 76),
      Instrument("Bowed Glass", 0, 92),
      Instrument("Brass 2", 8, 61),
      Instrument("Brass Section", 0, 61),
      Instrument("Breath Noise", 0, 121),
      Instrument("Bright Grand Piano", 0, 1),
      Instrument("Brightness", 0, 100),
      Instrument("Brush", 128, 40),
      Instrument("Brush 1", 128, 41),
      Instrument("Brush 2", 128, 42),
      Instrument("Calliope Lead", 0, 82),
      Instrument("Castanets", 8, 115),
      Instrument("Celesta", 0, 8),
      Instrument("Cello", 0, 42),
      Instrument("Charang", 0, 84),
      Instrument("Chiffer Lead", 0, 83),
      Instrument("Choir Aahs", 0, 52),
      Instrument("Church Bell", 8, 14),
      Instrument("Church Organ", 0, 19),
      Instrument("Church Organ 2", 8, 19),
      Instrument("Clarinet", 0, 71),
      Instrument("Clavinet", 0, 7),
      Instrument("Clean Guitar", 0, 27),
      Instrument("Concert Bass Drum", 8, 116),
      Instrument("Contrabass", 0, 43),
      Instrument("Coupled Harpsichord", 8, 6),
      Instrument("Crystal", 0, 98),
      Instrument("Detuned FM EP", 8, 5),
      Instrument("Detuned Organ 1", 8, 16),
      Instrument("Detuned Organ 2", 8, 17),
      Instrument("Detuned Saw", 20, 81),
      Instrument("Detuned Tine EP", 8, 4),
      Instrument("Distortion Guitar", 0, 30),
      Instrument("Drawbar Organ", 0, 16),
      Instrument("Dulcimer", 0, 15),
      Instrument("Echo Drops", 0, 102),
      Instrument("Electric Grand Piano", 0, 2),
      Instrument("Electronic", 128, 24),
      Instrument("English Horn", 0, 69),
      Instrument("Fantasia", 0, 88),
      Instrument("Feedback Guitar", 8, 30),
      Instrument("Fiddle", 0, 110),
      Instrument("Fingered Bass", 0, 33),
      Instrument("Flute", 0, 73),
      Instrument("FM Electric Piano", 0, 5),
      Instrument("French Horns", 0, 60),
      Instrument("Fret Noise", 0, 120),
      Instrument("Fretless Bass", 0, 35),
      Instrument("Funk Guitar", 8, 28),
      Instrument("Glockenspiel", 0, 9),
      Instrument("Goblin", 0, 101),
      Instrument("Grand Piano", 0, 0),
      Instrument("Guitar Feedback", 8, 31),
      Instrument("Guitar Harmonics", 0, 31),
      Instrument("Gun Shot", 0, 127),
      Instrument("Halo Pad", 0, 94),
      Instrument("Harmon Mute Trumpet", 0, 59),
      Instrument("Harmonica", 0, 22),
      Instrument("Harp", 0, 46),
      Instrument("Harpsichord", 0, 6),
      Instrument("Hawaiian Guitar", 8, 26),
      Instrument("Helicopter", 0, 125),
      Instrument("Honky-Tonk Piano", 0, 3),
      Instrument("Ice Rain", 0, 96),
      Instrument("Italian Accordion", 8, 21),
      Instrument("Jazz", 128, 32),
      Instrument("Jazz 1", 128, 33),
      Instrument("Jazz 2", 128, 34),
      Instrument("Jazz 3", 128, 35),
      Instrument("Jazz 4", 128, 36),
      Instrument("Jazz Guitar", 0, 26),
      Instrument("Kalimba", 0, 108),
      Instrument("Koto", 0, 107),
      Instrument("Mandolin", 16, 25),
      Instrument("Marching Snare", 128, 56),
      Instrument("Marimba", 0, 12),
      Instrument("Melo Tom 2", 8, 117),
      Instrument("Melodic Tom", 0, 117),
      Instrument("Metal Pad", 0, 93),
      Instrument("Music Box", 0, 10),
      Instrument("Nylon String Guitar", 0, 24),
      Instrument("Oboe", 0, 68),
      Instrument("Ocarina", 0, 79),
      Instrument("Orchestra Hit", 0, 55),
      Instrument("Orchestra Kit", 128, 48),
      Instrument("Orchestra Pad", 8, 48),
      Instrument("Overdrive Guitar", 0, 29),
      Instrument("Palm Muted Guitar", 0, 28),
      Instrument("Pan Flute", 0, 75),
      Instrument("Percussive Organ", 0, 17),
      Instrument("Piccolo", 0, 72),
      Instrument("Picked Bass", 0, 34),
      Instrument("Polysynth", 0, 90),
      Instrument("Pop Bass", 0, 37),
      Instrument("Power", 128, 16),
      Instrument("Power 1", 128, 17),
      Instrument("Power 2", 128, 18),
      Instrument("Power 3", 128, 19),
      Instrument("Recorder", 0, 74),
      Instrument("Reed Organ", 0, 20),
      Instrument("Reverse Cymbal", 0, 119),
      Instrument("Rock Organ", 0, 18),
      Instrument("Room", 128, 8),
      Instrument("Room 1", 128, 9),
      Instrument("Room 2", 128, 10),
      Instrument("Room 3", 128, 11),
      Instrument("Room 4", 128, 12),
      Instrument("Room 5", 128, 13),
      Instrument("Room 6", 128, 14),
      Instrument("Room 7", 128, 15),
      Instrument("Saw Lead", 0, 81),
      Instrument("Sea Shore", 0, 122),
      Instrument("Shakuhachi", 0, 77),
      Instrument("Shamisen", 0, 106),
      Instrument("Shenai", 0, 111),
      Instrument("Sine Wave", 8, 80),
      Instrument("Sitar", 0, 104),
      Instrument("Slap Bass", 0, 36),
      Instrument("Slow Violin", 8, 40),
      Instrument("Solo Vox", 0, 85),
      Instrument("Soprano Sax", 0, 64),
      Instrument("Soundtrack", 0, 97),
      Instrument("Space Voice", 0, 91),
      Instrument("Square Lead", 0, 80),
      Instrument("Standard", 128, 0),
      Instrument("Standard 1", 128, 1),
      Instrument("Standard 2", 128, 2),
      Instrument("Standard 3", 128, 3),
      Instrument("Standard 4", 128, 4),
      Instrument("Standard 5", 128, 5),
      Instrument("Standard 6", 128, 6),
      Instrument("Standard 7", 128, 7),
      Instrument("Star Theme", 0, 103),
      Instrument("Steel Drums", 0, 114),
      Instrument("Steel String Guitar", 0, 25),
      Instrument("Strings Fast", 0, 48),
      Instrument("Strings Pizzicato", 0, 45),
      Instrument("Strings Slow", 0, 49),
      Instrument("Strings Tremelo", 0, 44),
      Instrument("Sweep Pad", 0, 95),
      Instrument("Synth Bass 1", 0, 38),
      Instrument("Synth Bass 2", 0, 39),
      Instrument("Synth Bass 3", 8, 38),
      Instrument("Synth Bass 4", 8, 39),
      Instrument("Synth Brass 1", 0, 62),
      Instrument("Synth Brass 2", 0, 63),
      Instrument("Synth Brass 3", 8, 62),
      Instrument("Synth Brass 4", 8, 63),
      Instrument("Synth Drum", 0, 118),
      Instrument("Synth Strings 1", 0, 50),
      Instrument("Synth Strings 2", 0, 51),
      Instrument("Synth Strings 3", 8, 50),
      Instrument("Synth Voice", 0, 54),
      Instrument("Taisho Koto", 8, 107),
      Instrument("Telephone", 0, 124),
      Instrument("Temple Blocks", 8, 0),
      Instrument("Tenor Sax", 0, 66),
      Instrument("Timpani", 0, 47),
      Instrument("Tine Electric Piano", 0, 4),
      Instrument("Tinker Bell", 0, 112),
      Instrument("TR-808", 128, 25),
      Instrument("Trombone", 0, 57),
      Instrument("Trumpet", 0, 56),
      Instrument("Tuba", 0, 58),
      Instrument("Tubular Bells", 0, 14),
      Instrument("Ukulele", 8, 24),
      Instrument("Vibraphone", 0, 11),
      Instrument("Viola", 0, 41),
      Instrument("Violin", 0, 40),
      Instrument("Voice Oohs", 0, 53),
      Instrument("Warm Pad", 0, 89),
      Instrument("Whistle", 0, 78),
      Instrument("Woodblock", 0, 115),
      Instrument("Xylophone", 0, 13)};
  return all_instruments;
}

auto get_instrument_pointer(const std::string& instrument_name)
    -> const Instrument* {
  const auto& instruments = get_all_instruments();
  for (const auto& instrument : instruments) {
    if (instrument.instrument_name == instrument_name) {
      return &instrument;
    }
  }
  Q_ASSERT(false);
  return nullptr;
}

auto get_instrument_names() -> const std::vector<std::string>& {
  static const std::vector<std::string> instrument_names = []() {
    std::vector<std::string> temp_names;
    const auto& all_instruments = get_all_instruments();
    std::transform(all_instruments.cbegin(), all_instruments.cend(),
                   std::back_inserter(temp_names),
                   [](const Instrument& instrument) {
                     return instrument.instrument_name;
                   });
    return temp_names;
  }();
  return instrument_names;
}

auto get_instrument_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema({{"type", "string"},
                                           {"description", "the instrument"},
                                           {"enum", get_instrument_names()}});
  return instrument_schema;
}
