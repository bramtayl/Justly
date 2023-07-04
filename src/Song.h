#pragma once

#include <qjsondocument.h> // for QJsonDocument
#include <qjsonobject.h>   // for QJsonObject
#include <qstring.h>       // for QString

#include <vector> // for vector

#include "Instrument.h" // for Instrument
#include "TreeNode.h"   // for TreeNode

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
      // THESE DON'T WORK
      // Instrument("Grand Piano", "grand_piano", 0, 0),
      // Instrument("Bright Grand Piano", "bright_grand_piano", 0, 1),
      // Instrument("Honky-Tonk Piano", "honky_tonk_piano", 0, 3),
      // Instrument("Tine Electric Piano", "tine_electric_piano", 0, 4),
      // Instrument("FM Electric Piano", "fm_electric_piano", 0, 5),
      // Instrument("Harpsichord", "harpsichord", 0, 6),
      // Instrument("Clavinet", "clavinet", 0, 7),
      // Instrument("Celesta", "celesta", 0, 8),
      // Instrument("Glockenspiel", "glockenspiel", 0, 9),
      // Instrument("Music Box", "music_box", 0, 10),
      // Instrument("Vibraphone", "vibraphone", 0, 11),
      // Instrument("Xylophone", "xylophone", 0, 13),
      // Instrument("Tubular Bells", "tubular_bells", 0, 14),
      // Instrument("Dulcimer", "dulcimer", 0, 15),
      // Instrument("Drawbar Organ", "drawbar_organ", 0, 16),
      // Instrument("Percussive Organ", "percussive_organ", 0, 17),
      // Instrument("Rock Organ", "rock_organ", 0, 18),
      // Instrument("Church Organ", "church_organ", 0, 19),
      // Instrument("Nylon String Guitar", "nylon_string_guitar", 0, 24),
      // Instrument("Steel String Guitar", "steel_string_guitar", 0, 25),
      // Instrument("Acoustic Bass", "acoustic_bass", 0, 32),
      // Instrument("Fingered Bass", "fingered_bass", 0, 33),
      // Instrument("Picked Bass", "picked_bass", 0, 34),
      // Instrument("Fretless Bass", "fretless_bass", 0, 35),
      // Instrument("Slap Bass", "slap_bass", 0, 36),
      // Instrument("Violin", "violin", 0, 40),
      // Instrument("Viola", "viola", 0, 41),
      // Instrument("Cello", "cello", 0, 42),
      // Instrument("Strings Fast", "strings_fast", 0, 48),
      // Instrument("Trumpet", "trumpet", 0, 56),
      // Instrument("Trombone", "trombone", 0, 57),
      // Instrument("Tuba", "tuba", 0, 58),
      // Instrument("Harmon Mute Trumpet", "harmon_mute_trumpet", 0, 59),
      // Instrument("Sweep Pad", "sweep_pad", 0, 95),
      // Instrument("Ice Rain", "ice_rain", 0, 96),
      // Instrument("Temple Blocks", "temple_blocks", 8, 0),
      // Instrument("Detuned Tine EP", "detuned_tine_ep", 8, 4),
      // Instrument("Detuned FM EP", "detuned_fm_ep", 8, 5),
      // Instrument("Coupled Harpsichord", "coupled_harpischord", 8, 6),
      // Instrument("Church Bell", "church_bell", 8, 14),
      // Instrument("Detuned Organ 1", "detuned_organ_1", 8, 16),
      // Instrument("Detuned Organ 2", "detuned_organ_2", 8, 17),
      // Instrument("Church Organ 2", "church_organ_2", 8, 19),
      // Instrument("Ukelele", "ukelele", 8, 24),
      // Instrument("12-String Guitar", "twelve_string_guitar", 8, 25),
      // Instrument("Slow Violin", "slow_violin", 8, 40),
      // Instrument("Orchestra Pad", "orchestra_pad", 8, 48),
      // Instrument("Mandolin", "mandolin", 16, 25),
      // Instrument("Drawback Organ Expr.", "drawback_organ_expr", 17, 16),
      // Instrument("Perc. Organ Expr.", "perc_organ_expr", 17, 17),
      // Instrument("Rock Organ Expr.", "rock_organ_expr", 17, 18),
      // Instrument("Church Organ Expr.", "church_organ_expr", 17, 19),
      // Instrument("Violin Expr.", "violin_expr", 17, 40),
      // Instrument("Viola Expr.", "viola_expr", 17, 41),
      // Instrument("Cello Expr.", "cello_expr", 17, 42),
      // Instrument("Strings Trem Expr.", "strings_trem_expr", 17, 44),
      // Instrument("Trumpet Expr.", "trumpet_expr", 17, 56),
      // Instrument("Trombone Expr.", "trombone_expr", 17, 57),
      // Instrument("Tuba Expr.", "tuba_expr", 17, 58),
      // Instrument("Hmn. Mute Tpt. Expr.", "hmn_mute_tpt_expr", 17, 59),
      // Instrument("Sweep Pad Expr.", "sweep_pad_expr", 17, 95),
      // Instrument("Detuned Org. 1 Expr.", "detuned_org_1_expr", 18, 16),
      // Instrument("Detuned Org. 2 Expr.", "detuned_org_2_expr", 18, 17),
      // Instrument("Church Organ 2 Expr.", "church_org_2_expr", 18, 19),
      // Instrument("Slow Violin Expr.", "slow_violin_expr", 18, 40),
      // Instrument("Violins Fast", "violins_fast", 20, 48),
      // Instrument("Violins2 Fast", "violins2_fast", 25, 48),
      // Instrument("Violins Fast Expr.", "violins_fast_expr", 21, 48),
      // Instrument("Violins2 Fast Expr.", "violins2_fast_expr", 26, 48),
      // Instrument("Violas Fast", "violas_fast", 30, 48),
      // Instrument("Violas Fast Expr.", "violas_fast_expr", 31, 48),
      // Instrument("Celli Fast", "celli_fast", 40, 48),
      // Instrument("Basses Fast", "basses_fast", 50, 48),
      // Instrument("Standard", "standard", 128, 0),
      // Instrument("Standard 1", "standard_1", 128, 1),
      // Instrument("Standard 2", "standard_2", 128, 2),
      // Instrument("Standard 3", "standard_3", 128, 3),
      // Instrument("Standard 4", "standard_4", 128, 4),
      // Instrument("Standard 5", "standard_5", 128, 5),
      // Instrument("Standard 6", "standard_6", 128, 6),
      // Instrument("Standard 7", "standard_7", 128, 7),
      // Instrument("Room", "room", 128, 8),
      // Instrument("Room 1", "room_1", 128, 9),
      // Instrument("Room 2", "room_2", 128, 10),
      // Instrument("Room 3", "room_3", 128, 11),
      // Instrument("Room 4", "room_4", 128, 12),
      // Instrument("Room 5", "room_5", 128, 13),
      // Instrument("Room 6", "room_6", 128, 14),
      // Instrument("Room 7", "room_7", 128, 15),
      // Instrument("Power", "power", 128, 16),
      // Instrument("Power 1", "power_1", 128, 17),
      // Instrument("Power 2", "power_2", 128, 18),
      // Instrument("Power 3", "power_3", 128, 19),
      // Instrument("Electronic", "electronic", 128, 24),
      // Instrument("TR-808", "tr_808", 128, 25),
      // Instrument("Jazz", "jazz", 128, 32),
      // Instrument("Jazz 1", "jazz_1", 128, 33),
      // Instrument("Jazz 2", "jazz_2", 128, 34),
      // Instrument("Jazz 3", "jazz_3", 128, 35),
      // Instrument("Jazz 4", "jazz_4", 128, 36),
      // Instrument("Brush", "brush", 128, 40),
      // Instrument("Brush 1", "brush_1", 128, 41),
      // Instrument("Brush 2", "brush_2", 128, 42),
      // Instrument("Orchestra Kit", "orhcestra_kit", 128, 48),
      // Instrument("Marching Snare", "marching_snare", 128, 56),
      // Instrument("OldMarchingBass", "old_marching_bass", 128, 57),
      // Instrument("Marching Cymbals", "marching_cymbls", 128, 58),
      // Instrument("Marching Bass", "marching_bass", 128, 59),
      // Instrument("OldMarching Tenor", "old_marching_tenor", 128, 95),
      // Instrument("Marching Tenor", "marching_tenor", 128, 96),

      // expressive versions
      // Instrument("Reed Organ Expr.", "reed_organ_expr", 17, 20),
      // Instrument("Accordion Expr.", "accordion_expr", 17, 21),
      // Instrument("Harmonic Expr.", "harmonic_expr", 17, 22),
      // Instrument("Bandoneon Expr.", "bandoneon_expr", 17, 23),
      // Instrument("Contrabass Expr.", "contrabass_expr", 17, 43),
      // Instrument("Strings Fast Expr.", "strings_fast_expr", 17, 48),
      // Instrument("Strings Slow Expr.", "strings_slow_expr", 17, 49),
      // Instrument("Syn. Strings 1 Expr.", "syn_strings_1_expr", 17, 50),
      // Instrument("Syn. Strings 2 Expr.", "syn_stirngs_2_expr", 17, 51),
      // Instrument("Choir Aahs Expr.", "choir_aahs_expr", 17, 52),
      // Instrument("Voice Oohs Expr.", "voice_oohs_expr", 17, 53),
      // Instrument("Synth Voice Expr.", "synth_voice_expr", 17, 54),
      // Instrument("French Horns Expr.", "french_horns_expr", 17, 60),
      // Instrument("Brass Section Expr.", "brass_section_expr", 17, 61),
      // Instrument("Synth Bass 1 Expr.", "synth_bass_1_expr", 17, 62),
      // Instrument("Synth Bass 2 Expr.", "synth_bass_2_expr", 17, 63),
      // Instrument("Soprano Sax Expr.", "soprano_sax_expr", 17, 64),
      // Instrument("Alto Sax Expr.", "alto_sax_expr", 17, 65),
      // Instrument("Tenor Sax Expr.", "tenor_sax_expr", 17, 66),
      // Instrument("Baritone Sax Expr.", "bariton_sax_expr", 17, 67),
      // Instrument("Oboe Expr.", "oboe_expr", 17, 68),
      // Instrument("English Horn Expr.", "english_horn_expr", 17, 69),
      // Instrument("Bassoon Expr.", "bassoon_expr", 17, 70),
      // Instrument("Clarinet Expr.", "clarinet_expr", 17, 71),
      // Instrument("Piccolo Expr.", "piccolo_expr", 17, 72),
      // Instrument("Flute Expr.", "flute_expr", 17, 73),
      // Instrument("Recorder Expr.", "recorder_expr", 17, 74),
      // Instrument("Pan Flute Expr.", "pan_flute_expr", 17, 75),
      // Instrument("Bottle Chiff Expr.", "bottle_chiff_expr", 17, 76),
      // Instrument("Shakuhachi Expr.", "shakuhachi_expr", 17, 77),
      // Instrument("Whistle Expr.", "whistle_expr", 17, 78),
      // Instrument("Ocarina Expr.", "ocarina_expr", 17, 79),
      // Instrument("Square Lead Expr.", "square_lead_expr", 17, 80),
      // Instrument("Saw Lead Expr.", "saw_lead_expr", 17, 81),
      // Instrument("Calliope Expr.", "calliope_expr", 17, 82),
      // Instrument("Chiffer Lead Expr.", "chiffer_expr", 17, 83),
      // Instrument("Charang Expr.", "charang_expr", 17, 84),
      // Instrument("Solo Vox Expr.", "solo_vox_expr", 17, 85),
      // Instrument("5th Saw Wave Expr.", "fifth_saw_wave_expr", 17, 86),
      // Instrument("Bass & Lead Expr.", "bass_lead_expr", 17, 87),
      // Instrument("Warm Pad Expr.", "warm_pad_expr", 17, 89),
      // Instrument("Polysynth Expr.", "polysynth_expr", 17, 90),
      // Instrument("Space Voice Expr.", "space_voice_expr", 17, 91),
      // Instrument("Bowed Glass Expr.", "bowed_glass_expr", 17, 92),
      // Instrument("Metal Pad Expr.", "metal_pad_expr", 17, 93),
      // Instrument("Halo Pad Expr.", "halo_pad_expr", 17, 94),
      // Instrument("Soundtrack Expr.", "soundtrack_expr", 17, 97),
      // Instrument("Atmosphere Expr.", "atmosphere_expr", 17, 99),
      // Instrument("Goblin Expr.", "goblin_expr", 17, 101),
      // Instrument("Echo Drops Expr.", "echo_drops_expr", 17, 102),
      // Instrument("Star Theme Expr.", "star_theme_expr", 17, 103),
      // Instrument("Fiddle Expr.", "fiddle_expr", 17, 110),
      // Instrument("Shennai Expr.", "shennai_expr", 17, 111),
      // Instrument("It. Accordion Expr.", "it_accordion_expr", 18, 21),
      // Instrument("Synth Strings 3 Expr", "synth_strings_3_Expr", 18, 50),
      // Instrument("Brass 2 Expr.", "brass_2_expr", 18, 61),
      // Instrument("Synth Brass 3 Expr.", "synth_brass_3_expr", 18, 62),
      // Instrument("Synth Brass 4 Expr.", "synth_brass_4_expr", 18, 63),
      // Instrument("Sine Wave Expr.", "sine_wave_expr", 18, 80),
      // Instrument("Basses Trem Expr.", "basses_trem_expr", 51, 44),
      // Instrument("Basses Fast Expr.", "basses_fast_expr", 51, 49),
      // Instrument("Basses Slow Expr.", "basses_slow_expr", 51, 49),

      // Instrument("Violins Trem Expr.", "violins_trem_expr", 21, 44),
      // Instrument("Violins Slow Expr.", "violins_slow_expr", 21, 49),
      // Instrument("Detuned Saw Expr.", "detuned_saw_expr", 21, 81),
      // Instrument("Violins2 Trem Expr.", "violins2_trem_expr", 26, 44),
      // Instrument("Violins2 Slow Expr.", "violins2_slow_expr", 26, 49),
      // Instrument("Violas Trem Expr.", "violas_trem_expr", 31, 44),
      // Instrument("Violas Slow Expr.", "violas_slow_expr", 31, 49),
      // Instrument("Celli Trem Expr.", "celli_trem_expr", 41, 44),
      // Instrument("Celli Fast Expr.", "celli_fast_expr", 41, 48),
      // Instrument("Celli Slow Expr.", "celli_slow_expr", 41, 49),

      //Instrument("5th Saw Wave", "fifth_saw_wave", 0, 86),
      // Instrument("808 Tom", "eight_hundred_eight_tom", 8, 118),
      Instrument("Accordion", "accordion", 0, 21),
      Instrument("Agogo", "agogo", 0, 113),
      Instrument("Alto Sax", "alto_sax", 0, 65),
      //Instrument("Applause", "applause", 0, 126),
      //Instrument("Atmosphere", "atmosphere", 0, 99),
      Instrument("Bagpipe", "bagpipe", 0, 109),
      Instrument("Bandoneon", "bandoneon", 0, 23),
      Instrument("Banjo", "banjo", 0, 105),
      Instrument("Baritone Sax", "baritone_sax", 0, 67),
      //Instrument("Bass & Lead", "bass_and_lead", 0, 87),
      //Instrument("Basses Pizzicato", "basses_pizzicato", 50, 45),
      //Instrument("Basses Slow", "basses_slow", 50, 49),
      //Instrument("Basses Tremolo", "basses_tremolo", 50, 44),
      Instrument("Bassoon", "bassoon", 0, 70),
      //Instrument("Bird Tweet", "bird_tweet", 0, 123),
      Instrument("Bottle Chiff", "bottle_chiff", 0, 76),
      Instrument("Bowed Glass", "bowed_glass", 0, 92),
      //Instrument("Brass 2", "brass_2", 8, 61),
      Instrument("Brass Section", "brass_section", 0, 61),
      //Instrument("Breath Noise", "breath_noise", 0, 121),
      //Instrument("Brightness", "brightness", 0, 100),
      //Instrument("Calliope Lead", "calliope_lead", 0, 82),
      Instrument("Castanets", "castanets", 8, 115),
      //Instrument("Celli Pizzicato", "celli_pizzicato", 40, 45),
      //Instrument("Celli Slow", "celli_slow", 40, 49),
      //Instrument("Celli Tremolo", "celli_tremolo", 40, 44),
      Instrument("Charang", "charang", 0, 84),
      //Instrument("Chiffer Lead", "chiffer_lead", 0, 83),
      Instrument("Choir Aahs", "choir_aahs", 0, 52),
      Instrument("Clarinet", "clarinet", 0, 71),
      Instrument("Clean Guitar", "clean_guitar", 0, 27),
      Instrument("Concert Bass Drum", "concert_bass_drum", 8, 116),
      Instrument("Contrabass", "contrabass", 0, 43),
      //Instrument("Crystal", "crystal", 0, 98),
      //Instrument("Detuned Saw", "detuned_saw", 20, 81),
      //Instrument("Distortion Guitar", "distortion_guitar", 0, 30),
      //Instrument("Echo Drops", "echo_drops", 0, 102),
      //Instrument("Electric Grand Piano", "electric_grand_piano", 0, 2),
      Instrument("English Horn", "english_horn", 0, 69),
      //Instrument("Fantasia", "fantasia", 0, 88),
      //Instrument("Feedback Guitar", "feedback_guitar", 8, 30),
      Instrument("Fiddle", "fiddle", 0, 110),
      Instrument("Flute", "flute", 0, 73),
      Instrument("French Horns", "french_horns", 0, 60),
      //Instrument("Fret Noise", "fret_noise", 0, 120),
      Instrument("Funk Guitar", "funk_guitar", 8, 28),
      //Instrument("Goblin", "goblin", 0, 101),
      //Instrument("Guitar Feedback", "guitar_feedback", 8, 31),
      //Instrument("Guitar Harmonics", "guitar_harmonics", 0, 31),
      //Instrument("Halo Pad", "halo_pad", 0, 94),
      Instrument("Harmonica", "harmonica", 0, 22),
      Instrument("Harp", "harp", 0, 46),
      Instrument("Hawaiian Guitar", "hawaiian_guitar", 8, 26),
      //Instrument("Helicopter", "helicopter", 0, 125),
      Instrument("Italian Accordion", "italian_accordion", 8, 21),
      Instrument("Jazz Guitar", "jazz_guitar", 0, 26),
      Instrument("Kalimba", "kalimba", 0, 108),
      Instrument("Koto", "koto", 0, 107),
      Instrument("Marimba", "marimba", 0, 12),
      // Instrument("Melo Tom 2", "melo_tom_2", 8, 117),
      Instrument("Melodic Tom", "melodic_tom", 0, 117),
      //Instrument("Metal Pad", "metal_pad", 0, 93),
      Instrument("Oboe", "oboe", 0, 68),
      Instrument("Ocarina", "ocarina", 0, 79),
      //Instrument("Orchestra Hit", "orchestra_hit", 0, 55),
      //Instrument("Overdrive Guitar", "overdrive_guitar", 0, 29),
      Instrument("Palm Muted Guitar", "palm_muted_guitar", 0, 28),
      Instrument("Pan Flute", "pan_flute", 0, 75),
      Instrument("Piccolo", "piccolo", 0, 72),
      //Instrument("Polysynth", "polysynth", 0, 90),
      Instrument("Pop Bass", "pop_bass", 0, 37),
      Instrument("Recorder", "recorder", 0, 74),
      Instrument("Reed Organ", "reed_organ", 0, 20),
      //Instrument("Reverse Cymbal", "reverse_cymbal", 0, 119),
      //Instrument("Saw Lead", "saw_lead", 0, 81),
      //Instrument("Sea Shore", "sea_shore", 0, 122),
      Instrument("Shakuhachi", "shakuhachi", 0, 77),
      Instrument("Shamisen", "shamisen", 0, 106),
      Instrument("Shenai", "shenai", 0, 111),
      //Instrument("Sine Wave", "sine_wave", 8, 80),
      Instrument("Sitar", "sitar", 0, 104),
      //Instrument("Solo Vox", "solo_vox", 0, 85),
      Instrument("Soprano Sax", "soprano_sax", 0, 64),
      //Instrument("Soundtrack", "soundtrack", 0, 97),
      //Instrument("Space Voice", "space_voice", 0, 91),
      //Instrument("Square Lead", "square_lead", 0, 80),
      //Instrument("Star Theme", "star_theme", 0, 103),
      Instrument("Steel Drums", "steel_drums", 0, 114),
      Instrument("Strings Pizzicato", "strings_pizzicato", 0, 45),
      Instrument("Strings Slow", "strings_slow", 0, 49),
      Instrument("Strings Tremelo", "strings_tremelo", 0, 44),
      //Instrument("Synth Bass 1", "synth_bass_1", 0, 38),
      //Instrument("Synth Bass 2", "synth_bass_2", 0, 39),
      //Instrument("Synth Bass 3", "synth_bass_3", 8, 38),
      //Instrument("Synth Bass 4", "synth_bass_4", 8, 39),
      //Instrument("Synth Brass 1", "synth_brass_1", 0, 62),
      //Instrument("Synth Brass 2", "synth_brass_2", 0, 63),
      //Instrument("Synth Brass 3", "synth_brass_3", 8, 62),
      //Instrument("Synth Brass 4", "synth_brass_4", 8, 63),
      //Instrument("Synth Drum", "synth_drum", 0, 118),
      //Instrument("Synth Strings 1", "synth_strings_1", 0, 50),
      //Instrument("Synth Strings 2", "synth_strings_2", 0, 51),
      //Instrument("Synth Strings 3", "synth_strings_3", 8, 50),
      //Instrument("Synth Voice", "synth_voice", 0, 54),
      Instrument("Taiko Drum", "taiko_drum", 0, 116),
      Instrument("Taisho Koto", "taisho_koto", 8, 107),
      //Instrument("Telephone", "telephone", 0, 124),
      Instrument("Tenor Sax", "tenor_sax", 0, 66),
      Instrument("Timpani", "timpani", 0, 47),
      Instrument("Tinker Bell", "tinker_bell", 0, 112),
      //Instrument("Violas Pizzicato", "violas_pizzicato", 30, 45),
      //Instrument("Violas Slow", "violas_slow", 30, 49),
      //Instrument("Violas Tremolo", "violas_tremolo", 30, 44),
      //Instrument("Violins Pizzicato", "violins_pizzicato", 20, 45),
      //Instrument("Violins Slow", "violins_slow", 20, 49),
      //Instrument("Violins Tremolo", "violins_tremolo", 20, 44),
      //Instrument("Violins2 Pizzicato", "violin2_pizzicato", 25, 45),
      //Instrument("Violins2 Slow", "violins2_slow", 25, 49),
      //Instrument("Violins2 Tremolo", "violins2_tremolo", 25, 44),
      Instrument("Voice Oohs", "voice_oohs", 0, 53),
      //Instrument("Warm Pad", "warm_pad", 0, 89),
      Instrument("Whistle", "whistle", 0, 78),
      Instrument("Woodblock", "woodblock", 0, 115),
  };
  TreeNode root;

  explicit Song(
      const QString &starting_instrument_input = DEFAULT_STARTING_INSTRUMENT);

  [[nodiscard]] auto to_json() const -> QJsonDocument;

  [[nodiscard]] auto load_text(const QByteArray &song_text) -> bool;

  [[nodiscard]] auto verify_json(const QJsonObject &json_song) -> bool;

  [[nodiscard]] auto get_instrument_code(const QString &name) -> QString;
};
