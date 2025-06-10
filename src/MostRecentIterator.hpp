#pragma once

#include <QtCore/QMap>

struct MostRecentIterator {
  QMap<int, int>::iterator state;
  const QMap<int, int>::iterator end;
  int value;

  MostRecentIterator(QMap<int, int> &dict, const int value_input)
      : state(dict.begin()), end(dict.end()), value(value_input) {}
};
