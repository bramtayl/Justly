#pragma once

#include <QtCore/QString>

struct PlayState {
  double current_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;
};
