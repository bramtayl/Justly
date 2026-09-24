#pragma once

#include <QtCore/QString>

struct MusicXMLNote {
  int start_time = 0;
  int duration = 0;
  int midi_number = 0;
  // Johnston septimal quartertones (36/35): -1 for a 7, +1 for an el
  int septimal_quartertones = 0;
  int voice_number = 0;
  QString words;
};