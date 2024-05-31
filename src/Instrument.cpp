#include "src/Instrument.h"

#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qstring.h>           // for QString

#include <algorithm>  // for any_of
#include <iterator>   // for back_insert_iterator, back_inse...
#include <string>
#include <utility>  // for move

Instrument::Instrument(std::string name_input, int bank_number_input,
                       int preset_number_input, int instrument_id_input)
    : instrument_name(std::move(name_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input),
      instrument_id(instrument_id_input) {}

auto Instrument::get_all_instruments() -> const std::vector<Instrument> & {
  static const std::vector<Instrument> all_instruments = {
      Instrument("12-String Guitar", 8, 25, 0),
      Instrument("5th Saw Wave", 0, 86, 1),
      Instrument("808 Tom", 8, 118, 2),
      Instrument("Accordion", 0, 21, 3),
      Instrument("Acoustic Bass", 0, 32, 4),
      Instrument("Agogo", 0, 113, 5),
      Instrument("Alto Sax", 0, 65, 6),
      Instrument("Applause", 0, 126, 7),
      Instrument("Atmosphere", 0, 99, 8),
      Instrument("Bagpipe", 0, 109, 9),
      Instrument("Bandoneon", 0, 23, 10),
      Instrument("Banjo", 0, 105, 11),
      Instrument("Baritone Sax", 0, 67, 12),
      Instrument("Bass & Lead", 0, 87, 13),
      Instrument("Bassoon", 0, 70, 14),
      Instrument("Bird Tweet", 0, 123, 15),
      Instrument("Bottle Chiff", 0, 76, 16),
      Instrument("Bowed Glass", 0, 92, 17),
      Instrument("Brass 2", 8, 61, 18),
      Instrument("Brass Section", 0, 61, 19),
      Instrument("Breath Noise", 0, 121, 20),
      Instrument("Bright Grand Piano", 0, 1, 21),
      Instrument("Brightness", 0, 100, 22),
      Instrument("Brush", 128, 40, 23),
      Instrument("Brush 1", 128, 41, 24),
      Instrument("Brush 2", 128, 42, 25),
      Instrument("Calliope Lead", 0, 82, 26),
      Instrument("Castanets", 8, 115, 27),
      Instrument("Celesta", 0, 8, 28),
      Instrument("Cello", 0, 42, 29),
      Instrument("Charang", 0, 84, 30),
      Instrument("Chiffer Lead", 0, 83, 31),
      Instrument("Choir Aahs", 0, 52, 32),
      Instrument("Church Bell", 8, 14, 33),
      Instrument("Church Organ", 0, 19, 34),
      Instrument("Church Organ 2", 8, 19, 35),
      Instrument("Clarinet", 0, 71, 36),
      Instrument("Clavinet", 0, 7, 37),
      Instrument("Clean Guitar", 0, 27, 38),
      Instrument("Concert Bass Drum", 8, 116, 39),
      Instrument("Contrabass", 0, 43, 40),
      Instrument("Coupled Harpsichord", 8, 6, 41),
      Instrument("Crystal", 0, 98, 42),
      Instrument("Detuned FM EP", 8, 5, 43),
      Instrument("Detuned Organ 1", 8, 16, 44),
      Instrument("Detuned Organ 2", 8, 17, 45),
      Instrument("Detuned Saw", 20, 81, 46),
      Instrument("Detuned Tine EP", 8, 4, 47),
      Instrument("Distortion Guitar", 0, 30, 48),
      Instrument("Drawbar Organ", 0, 16, 49),
      Instrument("Dulcimer", 0, 15, 50),
      Instrument("Echo Drops", 0, 102, 51),
      Instrument("Electric Grand Piano", 0, 2, 52),
      Instrument("Electronic", 128, 24, 53),
      Instrument("English Horn", 0, 69, 54),
      Instrument("Fantasia", 0, 88, 55),
      Instrument("Feedback Guitar", 8, 30, 56),
      Instrument("Fiddle", 0, 110, 57),
      Instrument("Fingered Bass", 0, 33, 58),
      Instrument("Flute", 0, 73, 59),
      Instrument("FM Electric Piano", 0, 5, 60),
      Instrument("French Horns", 0, 60, 61),
      Instrument("Fret Noise", 0, 120, 62),
      Instrument("Fretless Bass", 0, 35, 63),
      Instrument("Funk Guitar", 8, 28, 64),
      Instrument("Glockenspiel", 0, 9, 65),
      Instrument("Goblin", 0, 101, 66),
      Instrument("Grand Piano", 0, 0, 67),
      Instrument("Guitar Feedback", 8, 31, 68),
      Instrument("Guitar Harmonics", 0, 31, 69),
      Instrument("Gun Shot", 0, 127, 70),
      Instrument("Halo Pad", 0, 94, 71),
      Instrument("Harmon Mute Trumpet", 0, 59, 72),
      Instrument("Harmonica", 0, 22, 73),
      Instrument("Harp", 0, 46, 74),
      Instrument("Harpsichord", 0, 6, 75),
      Instrument("Hawaiian Guitar", 8, 26, 76),
      Instrument("Helicopter", 0, 125, 77),
      Instrument("Honky-Tonk Piano", 0, 3, 78),
      Instrument("Ice Rain", 0, 96, 79),
      Instrument("Italian Accordion", 8, 21, 80),
      Instrument("Jazz", 128, 32, 81),
      Instrument("Jazz 1", 128, 33, 82),
      Instrument("Jazz 2", 128, 34, 83),
      Instrument("Jazz 3", 128, 35, 84),
      Instrument("Jazz 4", 128, 36, 85),
      Instrument("Jazz Guitar", 0, 26, 86),
      Instrument("Kalimba", 0, 108, 87),
      Instrument("Koto", 0, 107, 88),
      Instrument("Mandolin", 16, 25, 89),
      Instrument("Marching Snare", 128, 56, 90),
      Instrument("Marimba", 0, 12, 91),
      Instrument("Melo Tom 2", 8, 117, 92),
      Instrument("Melodic Tom", 0, 117, 93),
      Instrument("Metal Pad", 0, 93, 94),
      Instrument("Music Box", 0, 10, 95),
      Instrument("Nylon String Guitar", 0, 24, 96),
      Instrument("Oboe", 0, 68, 97),
      Instrument("Ocarina", 0, 79, 98),
      Instrument("Orchestra Hit", 0, 55, 99),
      Instrument("Orchestra Kit", 128, 48, 100),
      Instrument("Orchestra Pad", 8, 48, 101),
      Instrument("Overdrive Guitar", 0, 29, 102),
      Instrument("Palm Muted Guitar", 0, 28, 103),
      Instrument("Pan Flute", 0, 75, 104),
      Instrument("Percussive Organ", 0, 17, 105),
      Instrument("Piccolo", 0, 72, 106),
      Instrument("Picked Bass", 0, 34, 107),
      Instrument("Polysynth", 0, 90, 108),
      Instrument("Pop Bass", 0, 37, 109),
      Instrument("Power", 128, 16, 110),
      Instrument("Power 1", 128, 17, 111),
      Instrument("Power 2", 128, 18, 112),
      Instrument("Power 3", 128, 19, 113),
      Instrument("Recorder", 0, 74, 114),
      Instrument("Reed Organ", 0, 20, 115),
      Instrument("Reverse Cymbal", 0, 119, 116),
      Instrument("Rock Organ", 0, 18, 117),
      Instrument("Room", 128, 8, 118),
      Instrument("Room 1", 128, 9, 119),
      Instrument("Room 2", 128, 10, 120),
      Instrument("Room 3", 128, 11, 121),
      Instrument("Room 4", 128, 12, 122),
      Instrument("Room 5", 128, 13, 123),
      Instrument("Room 6", 128, 14, 124),
      Instrument("Room 7", 128, 15, 125),
      Instrument("Saw Lead", 0, 81, 126),
      Instrument("Sea Shore", 0, 122, 127),
      Instrument("Shakuhachi", 0, 77, 128),
      Instrument("Shamisen", 0, 106, 129),
      Instrument("Shenai", 0, 111, 130),
      Instrument("Sine Wave", 8, 80, 131),
      Instrument("Sitar", 0, 104, 132),
      Instrument("Slap Bass", 0, 36, 133),
      Instrument("Slow Violin", 8, 40, 134),
      Instrument("Solo Vox", 0, 85, 135),
      Instrument("Soprano Sax", 0, 64, 136),
      Instrument("Soundtrack", 0, 97, 137),
      Instrument("Space Voice", 0, 91, 138),
      Instrument("Square Lead", 0, 80, 139),
      Instrument("Standard", 128, 0, 140),
      Instrument("Standard 1", 128, 1, 141),
      Instrument("Standard 2", 128, 2, 142),
      Instrument("Standard 3", 128, 3, 143),
      Instrument("Standard 4", 128, 4, 144),
      Instrument("Standard 5", 128, 5, 145),
      Instrument("Standard 6", 128, 6, 146),
      Instrument("Standard 7", 128, 7, 147),
      Instrument("Star Theme", 0, 103, 148),
      Instrument("Steel Drums", 0, 114, 149),
      Instrument("Steel String Guitar", 0, 25, 150),
      Instrument("Strings Fast", 0, 48, 151),
      Instrument("Strings Pizzicato", 0, 45, 152),
      Instrument("Strings Slow", 0, 49, 153),
      Instrument("Strings Tremelo", 0, 44, 154),
      Instrument("Sweep Pad", 0, 95, 155),
      Instrument("Synth Bass 1", 0, 38, 156),
      Instrument("Synth Bass 2", 0, 39, 157),
      Instrument("Synth Bass 3", 8, 38, 158),
      Instrument("Synth Bass 4", 8, 39, 159),
      Instrument("Synth Brass 1", 0, 62, 160),
      Instrument("Synth Brass 2", 0, 63, 161),
      Instrument("Synth Brass 3", 8, 62, 162),
      Instrument("Synth Brass 4", 8, 63, 163),
      Instrument("Synth Drum", 0, 118, 164),
      Instrument("Synth Strings 1", 0, 50, 165),
      Instrument("Synth Strings 2", 0, 51, 166),
      Instrument("Synth Strings 3", 8, 50, 167),
      Instrument("Synth Voice", 0, 54, 168),
      Instrument("Taisho Koto", 8, 107, 169),
      Instrument("Telephone", 0, 124, 170),
      Instrument("Temple Blocks", 8, 0, 171),
      Instrument("Tenor Sax", 0, 66, 172),
      Instrument("Timpani", 0, 47, 173),
      Instrument("Tine Electric Piano", 0, 4, 174),
      Instrument("Tinker Bell", 0, 112, 175),
      Instrument("TR-808", 128, 25, 176),
      Instrument("Trombone", 0, 57, 177),
      Instrument("Trumpet", 0, 56, 178),
      Instrument("Tuba", 0, 58, 179),
      Instrument("Tubular Bells", 0, 14, 180),
      Instrument("Ukulele", 8, 24, 181),
      Instrument("Vibraphone", 0, 11, 182),
      Instrument("Viola", 0, 41, 183),
      Instrument("Violin", 0, 40, 184),
      Instrument("Voice Oohs", 0, 53, 185),
      Instrument("Warm Pad", 0, 89, 186),
      Instrument("Whistle", 0, 78, 187),
      Instrument("Woodblock", 0, 115, 188),
      Instrument("Xylophone", 0, 13, 189)
  };
  return all_instruments;
}

auto Instrument::get_all_instrument_names()
    -> const std::vector<std::string> & {
  static const std::vector<std::string> all_instrument_names = []() {
    std::vector<std::string> temp_names;
    const auto &all_instrument_names = get_all_instruments();
    std::transform(all_instrument_names.cbegin(), all_instrument_names.cend(),
                   std::back_inserter(temp_names),
                   [](const Instrument &instrument) {
                     return instrument.instrument_name;
                   });
    return temp_names;
  }();
  return all_instrument_names;
}

auto Instrument::get_instrument_by_name(const std::string &instrument_name)
    -> const Instrument & {
  if (instrument_name.empty()) {
    return Instrument::get_empty_instrument();
  }
  const auto &instruments = get_all_instruments();
  return *std::find_if(instruments.cbegin(), instruments.cend(),
                       [instrument_name](const Instrument &instrument) {
                         return instrument.instrument_name == instrument_name;
                       });
}

auto Instrument::get_empty_instrument() -> const Instrument & {
  static const auto empty_instrument = Instrument();
  return empty_instrument;
}
