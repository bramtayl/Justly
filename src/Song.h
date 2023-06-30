#pragma once

#include <qjsondocument.h>  // for QJsonDocument
#include <qjsonobject.h>    // for QJsonObject
#include <qstring.h>        // for QString

#include <vector>  // for vector

#include "Instrument.h"   // for Instrument
#include "TreeNode.h" // for TreeNode

class QByteArray;

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

const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

class Song {
 public:
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_volume = DEFAULT_STARTING_VOLUME;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QString starting_instrument;
  const std::vector<Instrument> instruments = {
      Instrument("Grand Piano", "grand_piano", 0, 0),
      Instrument("Bright Grand Piano", "bright_grand_piano", 0, 1),
      Instrument("Electric Grand Piano", "electric_grand_piano", 0, 2),
      Instrument("Honky-Tonk Piano", "honky_tonk_piano", 0, 3),
      Instrument("Tine Electric Piano", "tine_electric_piano", 0, 4),
      Instrument("FM Electric Piano", "fm_electric_piano", 0, 5),
      Instrument("Harpsichord", "harpsichord", 0, 6),
      Instrument("Clavinet", "clavinet", 0, 7),
      Instrument("Celesta", "celesta", 0, 8),
      Instrument("Glockenspiel", "glockenspiel", 0, 9),
      Instrument("Music Box", "music_box", 0, 10),
      Instrument("Vibraphone", "vibraphone", 0, 11),
      Instrument("Marimba", "marimba", 0, 12),
      Instrument("Xylophone", "xylophone", 0, 13),
      Instrument("Tubular Bells", "tubular_bells", 0, 14),
      Instrument("Dulcimer", "dulcimer", 0, 15),
      Instrument("Drawbar Organ", "drawbar_organ", 0, 16),
      Instrument("Percussive Organ", "percussive_organ", 0, 17),
      Instrument("Rock Organ", "rock_organ", 0, 18),
      Instrument("Church Organ", "church_organ", 0, 19),
      Instrument("Reed Organ", "reed_organ", 0, 20),
      Instrument("Accordion", "accordion", 0, 21),
      Instrument("Harmonica", "harmonica", 0, 22),
      Instrument("Bandoneon", "bandoneon", 0, 23),
      Instrument("Nylon String Guitar", "nylon_string_guitar", 0, 24),
      Instrument("Steel String Guitar", "steel_string_guitar", 0, 25),
      Instrument("Jazz Guitar", "jazz_guitar", 0, 26),
      Instrument("Clean Guitar", "clean_guitar", 0, 27),
      Instrument("Palm Muted Guitar", "palm_muted_guitar", 0, 28),
      Instrument("Overdrive Guitar", "overdrive_guitar", 0, 29),
      Instrument("Distortion Guitar", "distortion_guitar", 0, 30),
      Instrument("Guitar Harmonics", "guitar_harmonics", 0, 31),
      Instrument("Acoustic Bass", "acoustic_bass", 0, 32),
      Instrument("Fingered Bass", "fingered_bass", 0, 33),
      Instrument("Picked Bass", "picked_bass", 0, 34),
      Instrument("Fretless Bass", "fretless_bass", 0, 35),
      Instrument("Slap Bass", "slap_bass", 0, 36),
      Instrument("Pop Bass", "pop_bass", 0, 37),
      Instrument("Synth Bass 1", "synth_bass_1", 0, 38),
      Instrument("Synth Bass 2", "synth_bass_2", 0, 39),
      Instrument("Violin", "violin", 0, 40),
      Instrument("Viola", "viola", 0, 41),
      Instrument("Cello", "cello", 0, 42),
      Instrument("Contrabass", "contrabass", 0, 43),
      Instrument("Strings Tremelo", "strings_tremelo", 0, 44),
      Instrument("Strings Pizzicato", "strings_pizzicato", 0, 45),
      Instrument("Harp", "harp", 0, 46),
      Instrument("Timpani", "timpani", 0, 47),
      Instrument("Strings Fast", "strings_fast", 0, 48),
      Instrument("Strings Slow", "strings_slow", 0, 49),
      Instrument("Synth Strings 1", "synth_strings_1", 0, 50),
      Instrument("Synth Strings 2", "synth_strings_2", 0, 51),
      Instrument("Choir Aahs", "choir_aahs", 0, 52),
      Instrument("Voice Oohs", "voice_oohs", 0, 53),
      Instrument("Synth Voice", "synth_voice", 0, 54),
      Instrument("Orchestra Hit", "orchestra_hit", 0, 55),
      Instrument("Trumpet", "trumpet", 0, 56),
      Instrument("Trombone", "trombone", 0, 57),
      Instrument("Tuba", "tuba", 0, 58),
      Instrument("Harmon Mute Trumpet", "harmon_mute_trumpet", 0, 59),
      Instrument("French Horns", "french_horns", 0, 60),
      Instrument("Brass Section", "brass_section", 0, 61),
      Instrument("Synth Brass 1", "synth_brass_1", 0, 62),
      Instrument("Synth Brass 2", "synth_brass_2", 0, 63),
      Instrument("Soprano Sax", "soprano_sax", 0, 64),
      Instrument("Alto Sax", "alto_sax", 0, 65),
      Instrument("Tenor Sax", "tenor_sax", 0, 66),
      Instrument("Baritone Sax", "baritone_sax", 0, 67),
      Instrument("Oboe", "oboe", 0, 68),
      Instrument("English Horn", "english_horn", 0, 69),
      Instrument("Bassoon", "bassoon", 0, 70),
      Instrument("Clarinet", "clarinet", 0, 71),
      Instrument("Piccolo", "piccolo", 0, 72),
      Instrument("Flute", "flute", 0, 73),
      Instrument("Recorder", "recorder", 0, 74),
      Instrument("Pan Flute", "pan_flute", 0, 75),
      Instrument("Bottle Chiff", "bottle_chiff", 0, 76),
      Instrument("Shakuhachi", "shakuhachi", 0, 77),
      Instrument("Whistle", "whistle", 0, 78),
      Instrument("Ocarina", "ocarina", 0, 79),
      Instrument("Square Lead", "square_lead", 0, 80),
      Instrument("Saw Lead", "saw_lead", 0, 81),
      Instrument("Calliope Lead", "calliope_lead", 0, 82),
      Instrument("Chiffer Lead", "chiffer_lead", 0, 83),
      Instrument("Charang", "charang", 0, 84),
      Instrument("Solo Vox", "solo_vox", 0, 85),
      Instrument("5th Saw Wave", "fifth_saw_wave", 0, 86),
      Instrument("Bass & Lead", "bass_and_lead", 0, 87),
      Instrument("Fantasia", "fantasia", 0, 88),
      Instrument("Warm Pad", "warm_pad", 0, 89),
      Instrument("Polysynth", "polysynth", 0, 90),
      Instrument("Space Voice", "space_voice", 0, 91),
      Instrument("Bowed Glass", "bowed_glass", 0, 92),
      Instrument("Metal Pad", "metal_pad", 0, 93),
      Instrument("Halo Pad", "halo_pad", 0, 94),
      Instrument("Sweep Pad", "sweep_pad", 0, 95),
      Instrument("Ice Rain", "ice_rain", 0, 96),
      Instrument("Soundtrack", "soundtrack", 0, 97),
      Instrument("Crystal", "crystal", 0, 98),
      Instrument("Atmosphere", "atmosphere", 0, 99),
      Instrument("Brightness", "brightness", 0, 100),
      Instrument("Goblin", "goblin", 0, 101),
      Instrument("Echo Drops", "echo_drops", 0, 102),
      Instrument("Star Theme", "star_theme", 0, 103),
      Instrument("Sitar", "sitar", 0, 104),
      Instrument("Banjo", "banjo", 0, 105),
      Instrument("Shamisen", "shamisen", 0, 106),
      Instrument("Koto", "koto", 0, 107),
      Instrument("Kalimba", "kalimba", 0, 108),
      Instrument("Bagpipe", "bagpipe", 0, 109),
      Instrument("Fiddle", "fiddle", 0, 110),
      Instrument("Shenai", "shenai", 0, 111),
      Instrument("Tinker_bell", "tinker_bell", 0, 112),
      Instrument("Agogo", "agogo", 0, 113),
      Instrument("Steel Drums", "steel_drums", 0, 114),
      Instrument("Woodblock", "woodblock", 0, 115),
      Instrument("Taiko Drum", "taiko_drum", 0, 116),
      Instrument("Melodic Tom", "melodic_tom", 0, 117),
      Instrument("Synth Drum", "synth_drum", 0, 118),
      Instrument("Reverse Cymbal", "reverse_cymbal", 0, 119),
      Instrument("Fret Noise", "fret_noise", 0, 120),
      Instrument("Breath Noise", "breath_noise", 0, 121),
      Instrument("Sea Shore", "sea_shore", 0, 122),
      Instrument("Bird Tweet", "bird_tweet", 0, 123),
      Instrument("Telephone", "telephone", 0, 124),
      Instrument("Helicopter", "helicopter", 0, 125),
      Instrument("Applause", "applause", 0, 126),
      Instrument("Gun Shot", "gun_shot", 0, 127)};
  TreeNode root;

  explicit Song(const QString &starting_instrument_input = DEFAULT_STARTING_INSTRUMENT);

  [[nodiscard]] auto to_json() const -> QJsonDocument;

  [[nodiscard]] auto load_from(const QByteArray &song_text) -> bool;

  [[nodiscard]] auto verify_json(const QJsonObject &json_song) -> bool;

  [[nodiscard]] auto get_instrument_code(const QString &name) -> QString;
};
