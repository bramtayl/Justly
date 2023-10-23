#pragma once

enum NoteChordField {
  symbol_column,
  instrument_column,
  interval_column,
  beats_column,
  volume_percent_column,
  tempo_percent_column,
  words_column
};

struct SongIndex {
  int chord_number;
  int note_number;
  NoteChordField note_chord_field;
};
