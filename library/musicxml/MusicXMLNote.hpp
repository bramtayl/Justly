#pragma once

#include <QtCore/QString>
#include <optional>
#include <string>

#include "musicxml/Accidental.hpp"

struct MusicXMLNote {
  // as written in the file
  int start_time = 0;
  int duration = 0;
  bool is_pitched = true;
  // the written step and octave, for pitched notes
  QString step;
  int octave = 0;
  std::optional<Accidental> accidental;
  QString staff = "1";
  std::string instrument_id;
  bool tie_start = false;
  bool tie_stop = false;

  // filled in after parsing
  int midi_number = 0;
  // Johnston septimal quartertones (36/35): -1 for a 7, +1 for an el
  int septimal_quartertones = 0;
  int voice_number = 0;
  QString words;
};
