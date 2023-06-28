#pragma once

#include <memory>          // for unique_ptr
#include <qjsondocument.h> // for QJsonDocument
#include <qjsonobject.h>   // for QJsonObject
#include <qpointer.h>      // for QPointer
#include <qstring.h>       // for QString
#include <vector>          // for vector

#include "ChordsModel.h" // for ChordsModel
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

const std::vector<QString> SOUND_FONT_INSTRUMENTS = {
    "grand_piano",
    "bright_grand_piano",
    "electric_grand_piano",
    "honky_tonk_piano",
    "tine_electric_piano",
    "fm_electric_piano",
    "harpsichord",
    "clavinet",
    "celesta",
    "glockenspiel",
    "music_box",
    "vibraphone",
    "marimba",
    "xylophone",
    "tubular_bells",
    "dulcimer",
    "drawbar_organ",
    "percussive_organ",
    "rock_organ",
    "church_organ",
    "reed_organ",
    "accordion",
    "harmonica",
    "bandoneon",
    "nylon_string_guitar",
    "steel_string_guitar",
    "jazz_guitar",
    "clean_guitar",
    "palm_muted_guitar",
    "overdrive_guitar",
    "distortion_guitar",
    "guitar_harmonics",
    "acoustic_bass",
    "fingered_bass",
    "picked_bass",
    "fretless_bass",
    "slap_bass",
    "pop_bass",
    "synth_bass_1",
    "synth_bass_2",
    "violin",
    "viola",
    "cello",
    "contrabass",
    "strings_tremelo",
    "strings_pizzicato",
    "harp",
    "timpani",
    "strings_fast",
    "strings_slow",
    "synth_strings_1",
    "synth_strings_2",
    "choir_aahs",
    "voice_oos",
    "synth_voice",
    "orchestra_hit",
    "trumpet",
    "trombone",
    "tuba",
    "harmon_mute_trumpet",
    "french_horns",
    "brass_section",
    "synth_brass_1",
    "synth_brass_2",
    "soprano_sax",
    "alto_sax",
    "tenor_sax",
    "baritone_sax",
    "oboe",
    "english_horn",
    "bassoon",
    "clarinet",
    "piccolo",
    "flute",
    "recorder",
    "pan_flute",
    "bottle_chiff",
    "shakuhachi",
    "whistle",
    "ocarina",
    "square_lead",
    "saw_lead",
    "calliope_lead",
    "chiffer_lead",
    "charang",
    "solo_vox",
    "fifth_saw_wave",
    "bass_and_lead",
    "fantasia",
    "warm_pad",
    "polysynth",
    "space_voice",
    "bowed_glass",
    "metal_pad",
    "halo_pad",
    "sweep_pad",
    "ice_rain",
    "soundtrack",
    "crystal",
    "atmosphere",
    "brightness",
    "goblin",
    "echo_drops",
    "star_theme",
    "sitar",
    "banjo",
    "shamisen",
    "koto",
    "kalimba",
    "bagpipe",
    "fiddle",
    "shenai",
    "tinker_bell",
    "agogo",
    "steel_drums",
    "woodblock",
    "taiko_drum",
    "melodic_tom",
    "synth_drum",
    "reverse_cymbal",
    "fret_noise",
    "breath_noise",
    "sea_shore",
    "bird_tweet",
    "telephone",
    "helicopter",
    "applause",
    "gun_shot"
};

const auto DEFAULT_ORCHESTRA_TEXT = 
    generate_orchestra_code(
        "/home/brandon/Downloads/MuseScore_General.sf2",
        SOUND_FONT_INSTRUMENTS
    );
const auto DEFAULT_STARTING_INSTRUMENT = "grand_piano";

class Song {

public:
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_volume = DEFAULT_STARTING_VOLUME;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QString starting_instrument;
  std::vector<std::unique_ptr<const QString>> instrument_pointers;
  QString orchestra_code;
  Csound &csound_session;
  QUndoStack &undo_stack;

  const QPointer<ChordsModel> chords_model_pointer =
      new ChordsModel(instrument_pointers, undo_stack);

  explicit Song(
      Csound &csound_session_input, QUndoStack &undo_stack_input,
      const QString &starting_instrument_input = DEFAULT_STARTING_INSTRUMENT,
      const QString &orchestra_code_input = DEFAULT_ORCHESTRA_TEXT);

  [[nodiscard]] auto to_json() const -> QJsonDocument;

  [[nodiscard]] auto load_from(const QByteArray &song_text) -> bool;

  void set_orchestra_code(const QString &new_orchestra_text);
  [[nodiscard]] auto verify_json(const QJsonObject &json_song) -> bool;
  [[nodiscard]] auto
  verify_orchestra_code_compiles(const QString &new_orchestra_text) -> bool;
};
