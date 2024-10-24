#include "chord/ChordsModel.hpp"

#include <QList>
#include <QString>
#include <QTextStream>
#include <QtGlobal>
#include <cmath>
#include <cstdlib>

#include "chord/Chord.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "note/Note.hpp" // IWYU pragma: keep
#include "percussion/Percussion.hpp" // IWYU pragma: keep

static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

static const auto CENTS_PER_HALFSTEP = 100;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;

static const auto C_0_MIDI = 12;

enum Degree {
  c_degree = 0,
  c_sharp_degree = 1,
  d_degree = 2,
  e_flat_degree = 3,
  e_degree = 4,
  f_degree = 5,
  f_sharp_degree = 6,
  g_degree = 7,
  a_flat_degree = 8,
  a_degree = 9,
  b_flat_degree = 10,
  b_degree = 11
};

// header functions

auto get_midi(double key) -> double {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QList<Chord> *chords_pointer_input,
                         QObject *parent_pointer)
    : ItemsModel(undo_stack_pointer_input, chords_pointer_input,
                 parent_pointer),
      gain(DEFAULT_GAIN), starting_key(DEFAULT_STARTING_KEY),
      starting_velocity(DEFAULT_STARTING_VELOCITY),
      starting_tempo(DEFAULT_STARTING_TEMPO) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto ChordsModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_CHORD_COLUMNS;
}

auto ChordsModel::get_column_name(int column_number) const -> QString {
  switch (to_chord_column(column_number)) {
  case chord_instrument_column:
    return ChordsModel::tr("Instrument");
  case chord_percussion_set_column:
    return ChordsModel::tr("Percussion set");
  case chord_percussion_instrument_column:
    return ChordsModel::tr("Percussion instrument");
  case chord_interval_column:
    return ChordsModel::tr("Interval");
  case chord_beats_column:
    return ChordsModel::tr("Beats");
  case chord_velocity_ratio_column:
    return ChordsModel::tr("Velocity ratio");
  case chord_tempo_ratio_column:
    return ChordsModel::tr("Tempo ratio");
  case chord_words_column:
    return ChordsModel::tr("Words");
  case chord_notes_column:
    return ChordsModel::tr("Notes");
  case chord_percussions_column:
    return ChordsModel::tr("Percussions");
  }
}

auto ChordsModel::is_column_editable(int column_number) const -> bool {
  return column_number != chord_notes_column &&
         column_number != chord_percussions_column;
}

auto ChordsModel::get_status(int row_number) const -> QString {
  return get_key_text(*this, row_number);
};

auto get_key_text(const ChordsModel &chords_model, int last_chord_number,
                  double ratio) -> QString {
  auto *items_pointer = chords_model.items_pointer;
  Q_ASSERT(items_pointer != nullptr);
  auto key = chords_model.starting_key;
  for (auto chord_number = 0; chord_number <= last_chord_number;
       chord_number++) {
    key = key * interval_to_double(items_pointer->at(chord_number).interval);
  }
  key = key * ratio;
  auto midi_float = get_midi(key);
  auto closest_midi = round(midi_float);
  auto difference_from_c = static_cast<int>(closest_midi) - C_0_MIDI;
  auto octave =
      difference_from_c / HALFSTEPS_PER_OCTAVE; // floor integer division
  auto degree = difference_from_c - octave * HALFSTEPS_PER_OCTAVE;
  auto cents =
      static_cast<int>(round((midi_float - closest_midi) * CENTS_PER_HALFSTEP));

  QString result;
  QTextStream stream(&result);
  stream << key << " Hz; ";

  Q_ASSERT(degree >= 0);
  Q_ASSERT(degree <= 11);
  switch (static_cast<Degree>(degree)) {
  case c_degree:
    stream << "C";
    break;
  case c_sharp_degree:
    stream << "C♯";
    break;
  case d_degree:
    stream << "D";
    break;
  case e_flat_degree:
    stream << "E♭";
    break;
  case e_degree:
    stream << "E";
    break;
  case f_degree:
    stream << "F";
    break;
  case f_sharp_degree:
    stream << "F♯";
    break;
  case g_degree:
    stream << "G";
    break;
  case a_flat_degree:
    stream << "A♭";
    break;
  case a_degree:
    stream << "A";
    break;
  case b_flat_degree:
    stream << "B♭";
    break;
  case b_degree:
    stream << "B";
    break;
  }
  stream << octave;
  if (cents != 0) {
    stream << " " << (cents >= 0 ? "+" : "−") << " " << abs(cents) << " cents";
  }
  return result;
}
