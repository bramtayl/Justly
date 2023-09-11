#include "Note.h"

#include <qstring.h>        // for QString

#include "JsonErrorHandler.h"
#include "NoteChord.h"  // for error_level, note_level, TreeLevel
#include "utilities.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json_fwd.hpp>     // for json

auto Note::get_validator() -> nlohmann::json_schema::json_validator& {
  static nlohmann::json_schema::json_validator validator(R"(
  {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "array",
    "title": "Notes",
    "description": "the notes",
      "items": {
      "type": "object",
      "description": "a note",
      "properties": {
        "interval": {
          "type": "object",
          "description": "an interval",
          "properties": {
            "numerator": {
              "type": "integer",
              "description": "the numerator",
              "minimum": 1,
              "maximum": 199
            },
            "denominator": {
              "type": "integer",
              "description": "the denominator",
              "minimum": 1,
              "maximum": 199
            },
            "octave": {
              "type": "integer",
              "description": "the octave",
              "minimum": -9,
              "maximum": 9
            }
          }
        },
        "beats": {
          "type": "integer",
          "description": "the number of beats",
          "minimum": 1,
          "maximum": 100
        },
        "tempo_percent": {
          "type": "number",
          "description": "the tempo percent",
          "minimum": 1.0,
          "maximum": 400.0
        },
        "volume_percent": {
          "type": "number",
          "description": "the volume percent",
          "minimum": 1.0,
          "maximum": 400.0
        },
        "words": {
          "description": "the words",
          "type": "string"
        },
        "instrument": {
          "type": "string",
          "description": "the instrument",
          "enum": [
            "12-String Guitar",
            "5th Saw Wave Expressive",
            "5th Saw Wave",
            "808 Tom",
            "Accordion Expressive",
            "Accordion",
            "Acoustic Bass",
            "Agogo",
            "Alto Sax Expressive",
            "Alto Sax",
            "Applause",
            "Atmosphere Expressive",
            "Atmosphere",
            "Bagpipe",
            "Bandoneon Expressive",
            "Bandoneon",
            "Banjo",
            "Baritone Sax Expressive",
            "Baritone Sax",
            "Bass & Lead Expressive",
            "Bass & Lead",
            "Basses Fast Expressive",
            "Basses Fast",
            "Basses Pizzicato",
            "Basses Slow Expressive",
            "Basses Slow",
            "Basses Trem Expressive",
            "Basses Tremolo",
            "Bassoon Expressive",
            "Bassoon",
            "Bird Tweet",
            "Bottle Chiff Expressive",
            "Bottle Chiff",
            "Bowed Glass Expressive",
            "Bowed Glass",
            "Brass 2 Expressive",
            "Brass 2",
            "Brass Section Expressive",
            "Brass Section",
            "Breath Noise",
            "Bright Grand Piano",
            "Brightness",
            "Brush 1",
            "Brush 2",
            "Brush",
            "Calliope Expressive",
            "Calliope Lead",
            "Castanets",
            "Celesta",
            "Celli Fast Expressive",
            "Celli Fast",
            "Celli Pizzicato",
            "Celli Slow Expressive",
            "Celli Slow",
            "Celli Trem Expressive",
            "Celli Tremolo",
            "Cello Expressive",
            "Cello",
            "Charang Expressive",
            "Charang",
            "Chiffer Lead Expressive",
            "Chiffer Lead",
            "Choir Aahs Expressive",
            "Choir Aahs",
            "Church Bell",
            "Church Organ 2 Expressive",
            "Church Organ 2",
            "Church Organ Expressive",
            "Church Organ",
            "Clarinet Expressive",
            "Clarinet",
            "Clavinet",
            "Clean Guitar",
            "Concert Bass Drum",
            "Contrabass Expressive",
            "Contrabass",
            "Coupled Harpsichord",
            "Crystal",
            "Detuned FM EP",
            "Detuned Org. 1 Expressive",
            "Detuned Org. 2 Expressive",
            "Detuned Organ 1",
            "Detuned Organ 2",
            "Detuned Saw Expressive",
            "Detuned Saw",
            "Detuned Tine EP",
            "Distortion Guitar",
            "Drawback Organ Expressive",
            "Drawbar Organ",
            "Dulcimer",
            "Echo Drops Expressive",
            "Echo Drops",
            "Electric Grand Piano",
            "Electronic",
            "English Horn Expressive",
            "English Horn",
            "FM Electric Piano",
            "Fantasia",
            "Feedback Guitar",
            "Fiddle Expressive",
            "Fiddle",
            "Fingered Bass",
            "Flute Expressive",
            "Flute",
            "French Horns Expressive",
            "French Horns",
            "Fret Noise",
            "Fretless Bass",
            "Funk Guitar",
            "Glockenspiel",
            "Goblin Expressive",
            "Goblin",
            "Grand Piano",
            "Guitar Feedback",
            "Guitar Harmonics",
            "Halo Pad Expressive",
            "Halo Pad",
            "Harmon Mute Trumpet",
            "Harmonica Expressive",
            "Harmonica",
            "Harp",
            "Harpsichord",
            "Hawaiian Guitar",
            "Helicopter",
            "Hmn. Mute Tpt. Expressive",
            "Honky-Tonk Piano",
            "Ice Rain",
            "It. Accordion Expressive",
            "Italian Accordion",
            "Jazz 1",
            "Jazz 2",
            "Jazz 3",
            "Jazz 4",
            "Jazz Guitar",
            "Jazz",
            "Kalimba",
            "Koto",
            "Mandolin",
            "Marching Bass",
            "Marching Cymbals",
            "Marching Snare",
            "Marching Tenor",
            "Marimba",
            "Melo Tom 2",
            "Melodic Tom",
            "Metal Pad Expressive",
            "Metal Pad",
            "Music Box",
            "Nylon String Guitar",
            "Oboe Expressive",
            "Oboe",
            "Ocarina Expressive",
            "Ocarina",
            "Old Marching Tenor",
            "Old Marching Bass",
            "Orchestra Hit",
            "Orchestra Kit",
            "Orchestra Pad",
            "Overdrive Guitar",
            "Palm Muted Guitar",
            "Pan Flute Expressive",
            "Pan Flute",
            "Perc. Organ Expressive",
            "Percussive Organ",
            "Piccolo Expressive",
            "Piccolo",
            "Picked Bass",
            "Polysynth Expressive",
            "Polysynth",
            "Pop Bass",
            "Power 1",
            "Power 2",
            "Power 3",
            "Power",
            "Recorder Expressive",
            "Recorder",
            "Reed Organ Expressive",
            "Reed Organ",
            "Reverse Cymbal",
            "Rock Organ Expressive",
            "Rock Organ",
            "Room 1",
            "Room 2",
            "Room 3",
            "Room 4",
            "Room 5",
            "Room 6",
            "Room 7",
            "Room",
            "Saw Lead Expressive",
            "Saw Lead",
            "Sea Shore",
            "Shakuhachi Expressive",
            "Shakuhachi",
            "Shamisen",
            "Shenai",
            "Shennai Expressive",
            "Sine Wave Expressive",
            "Sine Wave",
            "Sitar",
            "Slap Bass",
            "Slow Violin Expressive",
            "Slow Violin",
            "Solo Vox Expressive",
            "Solo Vox",
            "Soprano Sax Expressive",
            "Soprano Sax",
            "Soundtrack Expressive",
            "Soundtrack",
            "Space Voice Expressive",
            "Space Voice",
            "Square Lead Expressive",
            "Square Lead",
            "Standard 1",
            "Standard 2",
            "Standard 3",
            "Standard 4",
            "Standard 5",
            "Standard 6",
            "Standard 7",
            "Standard",
            "Star Theme Expressive",
            "Star Theme",
            "Steel Drums",
            "Steel String Guitar",
            "Strings Fast Expressive",
            "Strings Fast",
            "Strings Pizzicato",
            "Strings Slow Expressive",
            "Strings Slow",
            "Strings Trem Expressive",
            "Strings Tremelo",
            "Sweep Pad Expressive",
            "Sweep Pad",
            "Syn. Strings 1 Expressive",
            "Syn. Strings 2 Expressive",
            "Synth Bass 1 Expressive",
            "Synth Bass 1",
            "Synth Bass 2 Expressive",
            "Synth Bass 2",
            "Synth Bass 3",
            "Synth Bass 4",
            "Synth Brass 1",
            "Synth Brass 2",
            "Synth Brass 3 Expressive",
            "Synth Brass 3",
            "Synth Brass 4 Expressive",
            "Synth Brass 4",
            "Synth Drum",
            "Synth Strings 1",
            "Synth Strings 2",
            "Synth Strings 3 Expr",
            "Synth Strings 3",
            "Synth Voice Expressive",
            "Synth Voice",
            "TR-808",
            "Taiko Drum",
            "Taisho Koto",
            "Telephone",
            "Temple Blocks",
            "Tenor Sax Expressive",
            "Tenor Sax",
            "Timpani",
            "Tine Electric Piano",
            "Tinker Bell",
            "Trombone Expressive",
            "Trombone",
            "Trumpet Expressive",
            "Trumpet",
            "Tuba Expressive",
            "Tuba",
            "Tubular Bells",
            "Ukelele",
            "Vibraphone",
            "Viola Expressive",
            "Viola",
            "Violas Fast Expressive",
            "Violas Fast",
            "Violas Pizzicato",
            "Violas Slow Expressive",
            "Violas Slow",
            "Violas Trem Expressive",
            "Violas Tremolo",
            "Violin Expressive",
            "Violin",
            "Violins Fast Expressive",
            "Violins Fast",
            "Violins Pizzicato",
            "Violins Slow Expressive",
            "Violins Slow",
            "Violins Trem Expressive",
            "Violins Tremolo",
            "Violins2 Fast Expressive",
            "Violins2 Fast",
            "Violins2 Pizzicato",
            "Violins2 Slow Expressive",
            "Violins2 Slow",
            "Violins2 Trem Expressive",
            "Violins2 Tremolo",
            "Voice Oohs Expressive",
            "Voice Oohs",
            "Warm Pad Expressive",
            "Warm Pad",
            "Whistle Expressive",
            "Whistle",
            "Woodblock",
            "Xylophone"
          ]
        }
      }
    }
  }
  )"_json);
  return validator;
}

Note::Note() : NoteChord() {}

auto Note::symbol_for() const -> QString { return "♪"; }

auto Note::get_level() const -> TreeLevel { return note_level; };

auto Note::new_child_pointer() -> std::unique_ptr<NoteChord> {
  error_level(note_level);
  return nullptr;
}

auto Note::verify_json_items(const QString &note_text) -> bool {
  nlohmann::json parsed_json;
  if (!(parse_json(parsed_json, note_text))) {
    return false;
  }

  JsonErrorHandler error_handler;  
  get_validator().validate(parsed_json, error_handler);
  return !error_handler;
}
