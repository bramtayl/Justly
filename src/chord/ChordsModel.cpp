#include "chord/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QTextStream>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <memory>

#include "chord/Chord.hpp"
#include "chord/SetChordBeats.hpp"
#include "chord/SetChordInterval.hpp"
#include "chord/SetChordTempoRatio.hpp"
#include "chord/SetChordVelocityRatio.hpp"
#include "chord/SetChordWords.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "rational/Rational.hpp"

static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

static const auto NUMBER_OF_CHORD_COLUMNS = 7;

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

[[nodiscard]] static auto
get_chord_column(const QModelIndex &index) -> ChordColumn {
  return to_chord_column(index.column());
}

// header functions

auto to_chord_column(int column) -> ChordColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_CHORD_COLUMNS);
  return static_cast<ChordColumn>(column);
}

auto get_midi(double key) -> double {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QObject *parent_pointer)
    : ItemModel(parent_pointer), undo_stack_pointer(undo_stack_pointer_input),
      gain(DEFAULT_GAIN), starting_key(DEFAULT_STARTING_KEY),
      starting_velocity(DEFAULT_STARTING_VELOCITY),
      starting_tempo(DEFAULT_STARTING_TEMPO) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto ChordsModel::rowCount(const QModelIndex & /*parent_index*/) const -> int {
  return static_cast<int>(chords.size());
}

auto ChordsModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_CHORD_COLUMNS;
}

auto ChordsModel::get_column_name(int column_number) const -> QString {
  switch (to_chord_column(column_number)) {
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
  return column_number != chord_notes_column && column_number != chord_percussions_column;
}

auto get_key_text(const ChordsModel &chords_model, qsizetype last_chord_number,
                  double ratio) -> QString {
  const auto &chords = chords_model.chords;
  auto key = chords_model.starting_key;
  for (qsizetype chord_number = 0; chord_number <= last_chord_number;
       chord_number++) {
    key = key * interval_to_double(chords.at(chord_number).interval);
  }
  key = key * ratio;
  auto midi_float = get_midi(key);
  auto closest_midi = round(midi_float);
  auto difference_from_c = closest_midi - C_0_MIDI;
  auto octave = difference_from_c / HALFSTEPS_PER_OCTAVE; // floor integer division
  auto degree =
      difference_from_c - octave * HALFSTEPS_PER_OCTAVE;
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

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  Q_ASSERT(index.isValid());
  auto row_number = get_row_number(index);
  if (role == Qt::StatusTipRole) {
    return get_key_text(*this, row_number);
  }
  if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return {};
  }
  const auto &chord = chords.at(row_number);
  switch (get_chord_column(index)) {
  case chord_interval_column:
    return QVariant::fromValue(chord.interval);
  case chord_beats_column:
    return QVariant::fromValue(chord.beats);
  case chord_velocity_ratio_column:
    return QVariant::fromValue(chord.velocity_ratio);
  case chord_tempo_ratio_column:
    return QVariant::fromValue(chord.tempo_ratio);
  case chord_words_column:
    return chord.words;
  case chord_notes_column:
    return chord.notes.size();
  case chord_percussions_column:
    return chord.percussions.size();
  }
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  auto chord_number = get_row_number(index);
  const auto &chord = chords.at(chord_number);
  switch (get_chord_column(index)) {
  case chord_interval_column:
    Q_ASSERT(new_value.canConvert<const Interval>());
    undo_stack_pointer->push(
        std::make_unique<SetChordInterval>(this, chord_number, chord.interval,
                                           new_value.value<Interval>())
            .release());
    break;
  case chord_beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetChordBeats>(this, chord_number, chord.beats,
                                        new_value.value<Rational>())
            .release());
    break;
  case chord_velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(std::make_unique<SetChordVelocityRatio>(
                                 this, chord_number, chord.velocity_ratio,
                                 new_value.value<Rational>())
                                 .release());
    break;
  case chord_tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetChordTempoRatio>(
            this, chord_number, chord.tempo_ratio, new_value.value<Rational>())
            .release());
    break;
  case chord_words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    undo_stack_pointer->push(
        std::make_unique<SetChordWords>(this, chord_number, chord.words,
                                        new_value.value<QString>())
            .release());
    break;
  default:
    Q_ASSERT(false);
  }
  return true;
}

void insert_chords(ChordsModel &chords_model, qsizetype first_chord_number,
                   const QList<Chord> &new_chords) {
  auto &chords = chords_model.chords;

  chords_model.begin_insert_rows(first_chord_number, new_chords.size());
  std::copy(new_chords.cbegin(), new_chords.cend(),
            std::inserter(chords, chords.begin() + first_chord_number));
  chords_model.end_insert_rows();
}

void remove_chords(ChordsModel &chords_model, qsizetype first_chord_number,
                   qsizetype number_of_chords) {
  auto &chords = chords_model.chords;

  chords_model.begin_remove_rows(first_chord_number, number_of_chords);
  chords.erase(chords.begin() + static_cast<int>(first_chord_number),
               chords.begin() +
                   static_cast<int>(first_chord_number + number_of_chords));
  chords_model.end_remove_rows();
}
