#pragma once

#include <QtCore/QString>

struct MusicXMLNote {
  int start_time = 0;
  int duration = 0;
  int midi_number = 0;
  QString words;
};