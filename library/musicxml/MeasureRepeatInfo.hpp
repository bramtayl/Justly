#pragma once

#include <QtCore/QList>
#include <QtCore/QtSwap>
#include <utility>

struct MeasureRepeatInfo {
  int start_time = 0;
  int end_time = 0;
  bool has_forward_repeat = false;
  bool has_backward_repeat = false;
  int repeat_times = 2;
  QList<int> ending_numbers;
};
