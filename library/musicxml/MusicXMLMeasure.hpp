#pragma once

#include <QtCore/QList>

#include "musicxml/MusicXMLNote.hpp"

struct MusicXMLMeasure {
  int number = 1;
  int start_time = 0;
  int end_time = 0;
  bool has_forward_repeat = false;
  bool has_backward_repeat = false;
  int repeat_times = 2;
  // the passes through a repeat this measure plays on, if it's in an ending
  QList<int> ending_numbers;
  // rests are left out, but the rest of the notes are in written order
  QList<MusicXMLNote> notes;
};
