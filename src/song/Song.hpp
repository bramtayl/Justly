#pragma once

#include <QList>
#include <QString>

#include "song/ControlId.hpp"

struct Chord;

static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

struct Song {
  // data
  double gain = DEFAULT_GAIN;
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_velocity = DEFAULT_STARTING_TEMPO;
  double starting_tempo = DEFAULT_STARTING_VELOCITY;
  QList<Chord> chords;
};

[[nodiscard]] auto get_double(const Song &song,
                       ControlId command_id) -> double;

[[nodiscard]] auto get_midi(double key) -> double;

[[nodiscard]] auto get_key_text(const Song &song,
                                int chord_number, double ratio = 1) -> QString;