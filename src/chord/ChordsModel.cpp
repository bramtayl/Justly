#include "chord/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <vector>

#include "chord/Chord.hpp"
#include "chord/SetChordBeats.hpp"
#include "chord/SetChordInterval.hpp"
#include "chord/SetChordTempoRatio.hpp"
#include "chord/SetChordVelocityRatio.hpp"
#include "chord/SetChordWords.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "other/conversions.hpp"
#include "other/templates.hpp"
#include "rational/Rational.hpp"

// IWYU pragma: no_include <algorithm>

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

static const auto C_SCALE = 0;
static const auto C_SHARP_SCALE = 1;
static const auto D_SCALE = 2;
static const auto E_FLAT_SCALE = 3;
static const auto E_SCALE = 4;
static const auto F_SCALE = 5;
static const auto F_SHARP_SCALE = 6;
static const auto G_SCALE = 7;
static const auto A_FLAT_SCALE = 8;
static const auto A_SCALE = 9;
static const auto B_FLAT_SCALE = 10;
static const auto B_SCALE = 11;

[[nodiscard]] static auto
get_chord_column(const QModelIndex &index) -> ChordColumn {
  return to_chord_column(index.column());
}

// header functions

auto get_child_number(const QModelIndex &index) -> size_t {
  return to_size_t(index.row());
}

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
                         QWidget *parent_pointer_input)
    : QAbstractTableModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input), gain(DEFAULT_GAIN),
      starting_key(DEFAULT_STARTING_KEY),
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

auto ChordsModel::headerData(int column, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (to_chord_column(column)) {
    case chord_interval_column:
      return ChordsModel::tr("Interval or PercussionInstrument");
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
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  Q_ASSERT(index.isValid());
  auto selectable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  auto chord_column = get_chord_column(index);
  return ((chord_column == chord_notes_column) ||
          (chord_column == chord_percussions_column))
             ? selectable
             : selectable | Qt::ItemIsEditable;
}

static auto get_key_text(double starting_key, const std::vector<Chord> &chords,
                         size_t last_chord_number) -> QString {
  auto key = starting_key;
  for (size_t chord_number = 0; chord_number <= last_chord_number;
       chord_number++) {
    key =
        key * interval_to_double(get_const_item(chords, chord_number).interval);
  }
  auto midi_float = get_midi(key);
  auto closest_midi = round(midi_float);
  auto difference_from_c = closest_midi - C_0_MIDI;
  auto octave =
      static_cast<int>(floor(difference_from_c / HALFSTEPS_PER_OCTAVE));
  auto scale =
      static_cast<int>(difference_from_c - octave * HALFSTEPS_PER_OCTAVE);
  auto cents =
      static_cast<int>(round((midi_float - closest_midi) * CENTS_PER_HALFSTEP));
  QString scale_text;
  switch (scale) {
  case C_SCALE:
    scale_text = "C";
    break;
  case C_SHARP_SCALE:
    scale_text = "C♯";
    break;
  case D_SCALE:
    scale_text = "D";
    break;
  case E_FLAT_SCALE:
    scale_text = "E♭";
    break;
  case E_SCALE:
    scale_text = "E";
    break;
  case F_SCALE:
    scale_text = "F";
    break;
  case F_SHARP_SCALE:
    scale_text = "F♯";
    break;
  case G_SCALE:
    scale_text = "G";
    break;
  case A_FLAT_SCALE:
    scale_text = "A♭";
    break;
  case A_SCALE:
    scale_text = "A";
    break;
  case B_FLAT_SCALE:
    scale_text = "B♭";
    break;
  case B_SCALE:
    scale_text = "B";
    break;
  default:
    Q_ASSERT(false);
  }
  QString result;
  QTextStream stream(&result);
  stream << key << " Hz; " << scale_text << octave << " "
         << (cents >= 0 ? "+" : "−") << " " << abs(cents) << " cents";
  return result;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  Q_ASSERT(index.isValid());
  auto child_number = get_child_number(index);
  if (role == Qt::StatusTipRole) {
    return get_key_text(starting_key, chords, child_number);
  }
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    const auto &chord = get_const_item(chords, child_number);
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
    default:
      Q_ASSERT(false);
      return {};
    }
  }
  return {};
}

auto ChordsModel::setData(const QModelIndex &index, const QVariant &new_value,
                          int role) -> bool {
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  auto chord_number = get_child_number(index);
  const auto &chord = get_const_item(chords, chord_number);
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
  parent_pointer->setFocus();
  return true;
}

void ChordsModel::edited_chords_cells(size_t first_chord_number,
                                      size_t number_of_chords,
                                      ChordColumn left_column,
                                      ChordColumn right_column) {
  emit dataChanged(
      index(first_chord_number, left_column),
      index(first_chord_number + number_of_chords - 1, right_column),
      {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::begin_insert_rows(size_t first_chord_number,
                                    size_t number_of_chords) {
  beginInsertRows(QModelIndex(), static_cast<int>(first_chord_number),
                  static_cast<int>(first_chord_number + number_of_chords) - 1);
}

void ChordsModel::end_insert_rows() { endInsertRows(); }

void ChordsModel::begin_remove_rows(size_t first_chord_number,
                                    size_t number_of_chords) {
  beginRemoveRows(QModelIndex(), static_cast<int>(first_chord_number),
                  static_cast<int>(first_chord_number + number_of_chords) - 1);
}

void ChordsModel::end_remove_rows() { endRemoveRows(); }

void insert_chord(ChordsModel *chords_model_pointer, size_t chord_number,
                  const Chord &new_chord) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &chords = chords_model_pointer->chords;

  check_end_number(chords, chord_number);

  chords_model_pointer->begin_insert_rows(chord_number, 1);
  chords.insert(chords.begin() + static_cast<int>(chord_number), new_chord);
  chords_model_pointer->end_insert_rows();
}

void insert_chords(ChordsModel *chords_model_pointer, size_t first_chord_number,
                   const std::vector<Chord> &new_chords) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &chords = chords_model_pointer->chords;

  check_end_number(chords, first_chord_number);

  chords_model_pointer->begin_insert_rows(first_chord_number,
                                          new_chords.size());
  chords.insert(chords.begin() + static_cast<int>(first_chord_number),
                new_chords.begin(), new_chords.end());
  chords_model_pointer->end_insert_rows();
}

void remove_chords(ChordsModel *chords_model_pointer, size_t first_chord_number,
                   size_t number_of_chords) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &chords = chords_model_pointer->chords;
  check_range(chords, first_chord_number, number_of_chords);

  chords_model_pointer->begin_remove_rows(first_chord_number, number_of_chords);
  chords.erase(chords.begin() + static_cast<int>(first_chord_number),
               chords.begin() +
                   static_cast<int>(first_chord_number + number_of_chords));
  chords_model_pointer->end_remove_rows();
}
