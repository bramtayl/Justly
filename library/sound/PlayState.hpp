#pragma once

#include <QtCore/QString>

struct PlayState {
  double current_time = 0;

  QString current_pitched_voice;
  QString current_unpitched_voice;
  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;
};
