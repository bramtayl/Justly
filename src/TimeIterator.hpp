#pragma once

#include <QtCore/QMap>

// given a sequence of divisions changes at certain division times
// find a real time and the beats per division
struct TimeIterator {
  const QMap<int, int> dict;
  QMap<int, int>::const_iterator state;
  const QMap<int, int>::const_iterator end;
  const int song_divisions;
  int last_change_time = 0;
  int next_change_divisions_time = 0;
  int time_per_division = 1;

  TimeIterator(QMap<int, int> dict_input, const int song_divisions_input)
      : dict(std::move(dict_input)), state(dict.begin()), end(dict.end()),
        song_divisions(song_divisions_input) {}
};
