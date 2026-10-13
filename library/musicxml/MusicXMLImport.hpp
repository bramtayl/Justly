#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <optional>

#include "rows/Chord.hpp"

class QWidget;

// a midi-instrument's sound, 0-based like a soundfont's
struct MidiProgram {
  // if the file doesn't say
  std::optional<short> bank_number;
  short preset_number = 0;
};

struct MusicXMLVoice {
  QString name;
  std::optional<MidiProgram> midi_program;
};

struct MusicXMLVoices {
  QList<MusicXMLVoice> pitched;
  QList<MusicXMLVoice> unpitched;
};

// a musicxml score converted to Justly's terms, ready to replace the song
struct ImportedScore {
  // uniquely and non-emptily named, with at least one unpitched voice
  MusicXMLVoices voices;
  int starting_midi_key = 0;
  int starting_tempo = 0;
  // notes name their voices from voices
  QList<Chord> chords;
};

// reads a .musicxml file, or a compressed .mxl file, by running
// import_score.py, which reads it with music21; warns and returns nullopt if
// the score can't be imported
[[nodiscard]] auto import_score(QWidget& parent, const QString& filename)
    -> std::optional<ImportedScore>;
