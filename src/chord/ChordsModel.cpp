#include "chord/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QTextStream>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <cmath>
#include <cstdlib>
#include <memory>

#include "chord/Chord.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "items_model/SetBeats.hpp"
#include "items_model/SetInstrument.hpp"
#include "items_model/SetInterval.hpp"
#include "items_model/SetPercussionInstrument.hpp"
#include "items_model/SetPercussionSet.hpp"
#include "items_model/SetTempoRatio.hpp"
#include "items_model/SetVelocityRatio.hpp"
#include "items_model/SetWords.hpp"
#include "justly/ChordColumn.hpp"
#include "note/Note.hpp"
#include "percussion/Percussion.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

static const auto NUMBER_OF_CHORD_COLUMNS = 10;

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
                         QList<Chord> *chords_pointer_input,
                         QObject *parent_pointer)
    : ItemsModel(chords_pointer_input, parent_pointer),
      undo_stack_pointer(undo_stack_pointer_input), gain(DEFAULT_GAIN),
      starting_key(DEFAULT_STARTING_KEY),
      starting_velocity(DEFAULT_STARTING_VELOCITY),
      starting_tempo(DEFAULT_STARTING_TEMPO) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto ChordsModel::get_instrument_column() const -> int {
  return chord_instrument_column;
};

auto ChordsModel::get_percussion_set_column() const -> int {
  return chord_percussion_set_column;
};

auto ChordsModel::get_percussion_instrument_column() const -> int {
  return chord_percussion_instrument_column;
};

auto ChordsModel::get_interval_column() const -> int {
  return chord_interval_column;
};

auto ChordsModel::get_beats_column() const -> int {
  return chord_beats_column;
};

auto ChordsModel::get_tempo_ratio_column() const -> int {
  return chord_tempo_ratio_column;
};

auto ChordsModel::get_velocity_ratio_column() const -> int {
  return chord_velocity_ratio_column;
};

auto ChordsModel::get_words_column() const -> int {
  return chord_words_column;
};

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

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  Q_ASSERT(items_pointer != nullptr);
  Q_ASSERT(index.isValid());
  auto row_number = index.row();
  if (role == Qt::StatusTipRole) {
    return get_key_text(*this, row_number);
  }
  if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return {};
  }
  const auto &chord = items_pointer->at(row_number);
  switch (get_chord_column(index)) {
  case chord_instrument_column:
    return QVariant::fromValue(chord.instrument_pointer);
  case chord_percussion_set_column:
    return QVariant::fromValue(chord.percussion_set_pointer);
  case chord_percussion_instrument_column:
    return QVariant::fromValue(chord.percussion_instrument_pointer);
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
  Q_ASSERT(items_pointer != nullptr);
  if (role != Qt::EditRole) {
    return false;
  }
  auto chord_number = index.row();
  const auto &chord = items_pointer->at(chord_number);
  switch (get_chord_column(index)) {
  case chord_instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    undo_stack_pointer->push(std::make_unique<SetInstrument<Chord>>(
                                 this, chord_number, chord.instrument_pointer,
                                 new_value.value<const Instrument *>())
                                 .release());
    break;
  case chord_percussion_set_column:
    Q_ASSERT(new_value.canConvert<const PercussionSet *>());
    undo_stack_pointer->push(std::make_unique<SetPercussionSet<Chord>>(
                                 this, chord_number,
                                 chord.percussion_set_pointer,
                                 new_value.value<const PercussionSet *>())
                                 .release());
    break;
  case chord_percussion_instrument_column:
    Q_ASSERT(new_value.canConvert<const PercussionInstrument *>());
    undo_stack_pointer->push(
        std::make_unique<SetPercussionInstrument<Chord>>(
            this, chord_number, chord.percussion_instrument_pointer,
            new_value.value<const PercussionInstrument *>())
            .release());
    break;
  case chord_interval_column:
    Q_ASSERT(new_value.canConvert<const Interval>());
    undo_stack_pointer->push(
        std::make_unique<SetInterval<Chord>>(this, chord_number, chord.interval,
                                             new_value.value<Interval>())
            .release());
    break;
  case chord_beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetBeats<Chord>>(this, chord_number, chord.beats,
                                          new_value.value<Rational>())
            .release());
    break;
  case chord_velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(std::make_unique<SetVelocityRatio<Chord>>(
                                 this, chord_number, chord.velocity_ratio,
                                 new_value.value<Rational>())
                                 .release());
    break;
  case chord_tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetTempoRatio<Chord>>(
            this, chord_number, chord.tempo_ratio, new_value.value<Rational>())
            .release());
    break;
  case chord_words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    undo_stack_pointer->push(
        std::make_unique<SetWords<Chord>>(this, chord_number, chord.words,
                                          new_value.value<QString>())
            .release());
    break;
  default:
    Q_ASSERT(false);
  }
  return true;
}

void ChordsModel::set_cells(int first_item_number, int left_column,
                            int right_column, const QList<Chord> &new_items) {
  Q_ASSERT(items_pointer != nullptr);
  auto number_of_chords = items_pointer->size();
  for (auto replace_number = 0; replace_number < number_of_chords;
       replace_number++) {
    auto &chord = (*items_pointer)[first_item_number + replace_number];
    const auto &new_chord = new_items.at(replace_number);
    for (auto chord_column = left_column; chord_column <= right_column;
         chord_column++) {
      switch (to_chord_column(chord_column)) {
      case chord_instrument_column:
        chord.instrument_pointer = new_chord.instrument_pointer;
        break;
      case chord_percussion_set_column:
        chord.percussion_set_pointer = new_chord.percussion_set_pointer;
        break;
      case chord_percussion_instrument_column:
        chord.percussion_instrument_pointer =
            new_chord.percussion_instrument_pointer;
        break;
      case chord_interval_column:
        chord.interval = new_chord.interval;
        break;
      case chord_beats_column:
        chord.beats = new_chord.beats;
        break;
      case chord_velocity_ratio_column:
        chord.velocity_ratio = new_chord.velocity_ratio;
        break;
      case chord_tempo_ratio_column:
        chord.tempo_ratio = new_chord.tempo_ratio;
        break;
      case chord_words_column:
        chord.words = new_chord.words;
        break;
      case chord_notes_column:
        chord.notes = new_chord.notes;
        break;
      case chord_percussions_column:
        chord.percussions = new_chord.percussions;
        break;
      }
    }
  }
  edited_cells(first_item_number, static_cast<int>(number_of_chords),
               left_column, right_column);
}
