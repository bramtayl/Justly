#pragma once

#include <QtCore/QMap>

#include "MusicXMLChord.hpp"

struct PartInfo {
  QString part_name;
  QMap<std::string, QString> instrument_map;
  QMap<int, MusicXMLChord> part_chords_dict;
  QMap<int, int> part_divisions_dict;
  QMap<int, int> part_midi_keys_dict;
  QMap<int, int> part_measure_number_dict;
};
