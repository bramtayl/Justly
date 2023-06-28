#pragma once

#include <memory>          // for unique_ptr
#include <qjsondocument.h> // for QJsonDocument
#include <qjsonobject.h>   // for QJsonObject
#include <qpointer.h>      // for QPointer
#include <qstring.h>       // for QString
#include <vector>          // for vector

#include "ChordsModel.h" // for ChordsModel
#include "Instrument.h"
#include "Utilities.h"

class Csound;
class QByteArray;
class QUndoStack;

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 50;
const auto DEFAULT_STARTING_TEMPO = 200;
const auto MINIMUM_STARTING_KEY = 60;
const auto MAXIMUM_STARTING_KEY = 440;
const auto MINIMUM_STARTING_VOLUME = 1;
const auto MAXIMUM_STARTING_VOLUME = 100;
const auto MINIMUM_STARTING_TEMPO = 100;
const auto MAXIMUM_STARTING_TEMPO = 800;

const auto SECONDS_PER_MINUTE = 60;
const auto FULL_NOTE_VOLUME = 0.2;

const auto DEFAULT_STARTING_INSTRUMENT = "grand_piano";

class Song {

public:
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_volume = DEFAULT_STARTING_VOLUME;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QString starting_instrument;
  const std::vector<Instrument> instruments = {
    Instrument("grand_piano", "grand_piano", 0, 0),
    Instrument("bright_grand_piano", "bright_grand_piano", 0, 1),
    Instrument("electric_grand_piano", "electric_grand_piano", 0, 2),
    Instrument("honky_tonk_piano", "honky_tonk_piano", 0, 3),
    Instrument("tine_electric_piano", "tine_electric_piano", 0, 4),
    Instrument("fm_electric_piano", "fm_electric_piano", 0, 5),
    Instrument("harpsichord", "harpsichord", 0, 6),
    Instrument("clavinet", "clavinet", 0, 7),
    Instrument("celesta", "celesta", 0, 8),
    Instrument("glockenspiel", "glockenspiel", 0, 9),
    Instrument("music_box", "music_box", 0, 10),
    Instrument("vibraphone", "vibraphone", 0, 11),
    Instrument("marimba", "marimba", 0, 12),
    Instrument("xylophone", "xylophone", 0, 13),
    Instrument("tubular_bells", "tubular_bells", 0, 14),
    Instrument("dulcimer", "dulcimer", 0, 15),
    Instrument("drawbar_organ", "drawbar_organ", 0, 16),
    Instrument("percussive_organ", "percussive_organ", 0, 17),
    Instrument("rock_organ", "rock_organ", 0, 18),
    Instrument("church_organ", "church_organ", 0, 19),
    Instrument("reed_organ", "reed_organ", 0, 20),
    Instrument("accordion", "accordion", 0, 21),
    Instrument("harmonica", "harmonica", 0, 22),
    Instrument("bandoneon", "bandoneon", 0, 23),
    Instrument("nylon_string_guitar", "nylon_string_guitar", 0, 24),
    Instrument("steel_string_guitar", "steel_string_guitar", 0, 25),
    Instrument("jazz_guitar", "jazz_guitar", 0, 26),
    Instrument("clean_guitar", "clean_guitar", 0, 27),
    Instrument("palm_muted_guitar", "palm_muted_guitar", 0, 28),
    Instrument("overdrive_guitar", "overdrive_guitar", 0, 29),
    Instrument("distortion_guitar", "distortion_guitar", 0, 30),
    Instrument("guitar_harmonics", "guitar_harmonics", 0, 31),
    Instrument("acoustic_bass", "acoustic_bass", 0, 32),
    Instrument("fingered_bass", "fingered_bass", 0, 33),
    Instrument("picked_bass", "picked_bass", 0, 34),
    Instrument("fretless_bass", "fretless_bass", 0, 35),
    Instrument("slap_bass", "slap_bass", 0, 36),
    Instrument("pop_bass", "pop_bass", 0, 37),
    Instrument("synth_bass_1", "synth_bass_1", 0, 38),
    Instrument("synth_bass_2", "synth_bass_2", 0, 39),
    Instrument("violin", "violin", 0, 40),
    Instrument("viola", "viola", 0, 41),
    Instrument("cello", "cello", 0, 42),
    Instrument("contrabass", "contrabass", 0, 43),
    Instrument("strings_tremelo", "strings_tremelo", 0, 44),
    Instrument("strings_pizzicato", "strings_pizzicato", 0, 45),
    Instrument("harp", "harp", 0, 46),
    Instrument("timpani", "timpani", 0, 47),
    Instrument("strings_fast", "strings_fast", 0, 48),
    Instrument("strings_slow", "strings_slow", 0, 49),
    Instrument("synth_strings_1", "synth_strings_1", 0, 50),
    Instrument("synth_strings_2", "synth_strings_2", 0, 51),
    Instrument("choir_aahs", "choir_aahs", 0, 52),
    Instrument("voice_oos", "voice_oos", 0, 53),
    Instrument("synth_voice", "synth_voice", 0, 54),
    Instrument("orchestra_hit", "orchestra_hit", 0, 55),
    Instrument("trumpet", "trumpet", 0, 56),
    Instrument("trombone", "trombone", 0, 57),
    Instrument("tuba", "tuba", 0, 58),
    Instrument("harmon_mute_trumpet", "harmon_mute_trumpet", 0, 59),
    Instrument("french_horns", "french_horns", 0, 60),
    Instrument("brass_section", "brass_section", 0, 61),
    Instrument("synth_brass_1", "synth_brass_1", 0, 62),
    Instrument("synth_brass_2", "synth_brass_2", 0, 63),
    Instrument("soprano_sax", "soprano_sax", 0, 64),
    Instrument("alto_sax", "alto_sax", 0, 65),
    Instrument("tenor_sax", "tenor_sax", 0, 66),
    Instrument("baritone_sax", "baritone_sax", 0, 67),
    Instrument("oboe", "oboe", 0, 68),
    Instrument("english_horn", "english_horn", 0, 69),
    Instrument("bassoon", "bassoon", 0, 70),
    Instrument("clarinet", "clarinet", 0, 71),
    Instrument("piccolo", "piccolo", 0, 72),
    Instrument("flute", "flute", 0, 73),
    Instrument("recorder", "recorder", 0, 74),
    Instrument("pan_flute", "pan_flute", 0, 75),
    Instrument("bottle_chiff", "bottle_chiff", 0, 76),
    Instrument("shakuhachi", "shakuhachi", 0, 77),
    Instrument("whistle", "whistle", 0, 78),
    Instrument("ocarina", "ocarina", 0, 79),
    Instrument("square_lead", "square_lead", 0, 80),
    Instrument("saw_lead", "saw_lead", 0, 81),
    Instrument("calliope_lead", "calliope_lead", 0, 82),
    Instrument("chiffer_lead", "chiffer_lead", 0, 83),
    Instrument("charang", "charang", 0, 84),
    Instrument("solo_vox", "solo_vox", 0, 85),
    Instrument("fifth_saw_wave", "fifth_saw_wave", 0, 86),
    Instrument("bass_and_lead", "bass_and_lead", 0, 87),
    Instrument("fantasia", "fantasia", 0, 88),
    Instrument("warm_pad", "warm_pad", 0, 89),
    Instrument("polysynth", "polysynth", 0, 90),
    Instrument("space_voice", "space_voice", 0, 91),
    Instrument("bowed_glass", "bowed_glass", 0, 92),
    Instrument("metal_pad", "metal_pad", 0, 93),
    Instrument("halo_pad", "halo_pad", 0, 94),
    Instrument("sweep_pad", "sweep_pad", 0, 95),
    Instrument("ice_rain", "ice_rain", 0, 96),
    Instrument("soundtrack", "soundtrack", 0, 97),
    Instrument("crystal", "crystal", 0, 98),
    Instrument("atmosphere", "atmosphere", 0, 99),
    Instrument("brightness", "brightness", 0, 100),
    Instrument("goblin", "goblin", 0, 101),
    Instrument("echo_drops", "echo_drops", 0, 102),
    Instrument("star_theme", "star_theme", 0, 103),
    Instrument("sitar", "sitar", 0, 104),
    Instrument("banjo", "banjo", 0, 105),
    Instrument("shamisen", "shamisen", 0, 106),
    Instrument("koto", "koto", 0, 107),
    Instrument("kalimba", "kalimba", 0, 108),
    Instrument("bagpipe", "bagpipe", 0, 109),
    Instrument("fiddle", "fiddle", 0, 110),
    Instrument("shenai", "shenai", 0, 111),
    Instrument("tinker_bell", "tinker_bell", 0, 112),
    Instrument("agogo", "agogo", 0, 113),
    Instrument("steel_drums", "steel_drums", 0, 114),
    Instrument("woodblock", "woodblock", 0, 115),
    Instrument("taiko_drum", "taiko_drum", 0, 116),
    Instrument("melodic_tom", "melodic_tom", 0, 117),
    Instrument("synth_drum", "synth_drum", 0, 118),
    Instrument("reverse_cymbal", "reverse_cymbal", 0, 119),
    Instrument("fret_noise", "fret_noise", 0, 120),
    Instrument("breath_noise", "breath_noise", 0, 121),
    Instrument("sea_shore", "sea_shore", 0, 122),
    Instrument("bird_tweet", "bird_tweet", 0, 123),
    Instrument("telephone", "telephone", 0, 124),
    Instrument("helicopter", "helicopter", 0, 125),
    Instrument("applause", "applause", 0, 126),
    Instrument("gun_shot", "gun_shot", 0, 127)
    };
  Csound &csound_session;
  QUndoStack &undo_stack;

  const QPointer<ChordsModel> chords_model_pointer =
      new ChordsModel(instruments, undo_stack);

  explicit Song(
      Csound &csound_session_input, QUndoStack &undo_stack_input,
      const QString &starting_instrument_input = DEFAULT_STARTING_INSTRUMENT);

  [[nodiscard]] auto to_json() const -> QJsonDocument;

  [[nodiscard]] auto load_from(const QByteArray &song_text) -> bool;

  [[nodiscard]] auto verify_json(const QJsonObject &json_song) -> bool;

};
