#include "other/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QBrush>
#include <QList>
#include <QObject>
#include <QPalette>
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
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>
#include <vector>

#include "commands/ChangeToInterval.hpp"
#include "commands/ChangeToPercussion.hpp"
#include "commands/SetChordBeats.hpp"
#include "commands/SetChordInterval.hpp"
#include "commands/SetChordTempoRatio.hpp"
#include "commands/SetChordVelocityRatio.hpp"
#include "commands/SetChordWords.hpp"
#include "commands/SetNoteBeats.hpp"
#include "commands/SetNoteInstrument.hpp"
#include "commands/SetNoteInterval.hpp"
#include "commands/SetNotePercussion.hpp"
#include "commands/SetNoteTempoRatio.hpp"
#include "commands/SetNoteVelocityRatio.hpp"
#include "commands/SetNoteWords.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "note_chord/NoteChord.hpp"
#include "other/RowRange.hpp"
#include "other/conversions.hpp"
#include "other/templates.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

// IWYU pragma: no_include <algorithm>

static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

static const auto NUMBER_OF_NOTE_CHORD_COLUMNS = 7;

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

template <typename Item>
[[nodiscard]] static auto get_item(std::vector<Item> &items,
                                   size_t item_number) -> Item & {
  check_number(items, item_number);
  return items[item_number];
}

// static functions
[[nodiscard]] static auto get_parent_chord_number(const QModelIndex &index) {
  return get_child_number(index.parent());
}

[[nodiscard]] static auto get_note_chord_column(const QModelIndex &index) {
  return to_note_chord_column(index.column());
}

// header functions

auto get_child_number(const QModelIndex &index) -> size_t {
  return to_size_t(index.row());
}

auto to_note_chord_column(int column) -> NoteChordColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_NOTE_CHORD_COLUMNS);
  return static_cast<NoteChordColumn>(column);
}

auto is_root_index(const QModelIndex &index) -> bool {
  // root index is invalid
  return !index.isValid();
}

auto valid_is_chord_index(const QModelIndex &index) -> bool {
  Q_ASSERT(!is_root_index(index));
  // chords have null parent pointers
  return index.internalPointer() == nullptr;
}

auto get_midi(double key) -> double {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QWidget *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input), gain(DEFAULT_GAIN),
      starting_key(DEFAULT_STARTING_KEY),
      starting_velocity(DEFAULT_STARTING_VELOCITY),
      starting_tempo(DEFAULT_STARTING_TEMPO) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto ChordsModel::get_chord_index(size_t chord_number,
                                  NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return createIndex(static_cast<int>(chord_number), note_chord_column,
                     nullptr);
}

auto ChordsModel::get_note_index(size_t chord_number, size_t note_number,
                                 NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return createIndex(static_cast<int>(note_number), note_chord_column,
                     &get_const_item(chords, chord_number));
}

auto ChordsModel::rowCount(const QModelIndex &parent_index) const -> int {
  if (is_root_index(parent_index)) {
    return static_cast<int>(chords.size());
  }
  // only nest into the symbol cell
  if (valid_is_chord_index(parent_index) &&
      get_note_chord_column(parent_index) == type_column) {
    return static_cast<int>(
        get_const_item(chords, get_child_number(parent_index)).notes.size());
  }
  // notes and non-symbol chord cells have no children
  return 0;
}

auto ChordsModel::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NUMBER_OF_NOTE_CHORD_COLUMNS;
}

// get the parent index
auto ChordsModel::parent(const QModelIndex &index) const -> QModelIndex {
  // chords have null parents
  if (valid_is_chord_index(index)) {
    return {};
  }
  // notes are nested into the type column
  // the internal pointer is the pointer to the parent chord
  // use std::distance to get the chord number from the pointer
  return createIndex(
      static_cast<int>(std::distance(
          chords.data(), static_cast<const Chord *>(index.internalPointer()))),
      type_column, nullptr);
}

auto ChordsModel::index(int signed_child_number, int column,
                        const QModelIndex &parent_index) const -> QModelIndex {
  auto child_number = to_size_t(signed_child_number);
  auto note_chord_column = to_note_chord_column(column);
  if (is_root_index(parent_index)) {
    return get_chord_index(child_number, note_chord_column);
  }
  return get_note_index(get_child_number(parent_index), child_number,
                        note_chord_column);
}

auto ChordsModel::headerData(int column, Qt::Orientation orientation,
                             int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (to_note_chord_column(column)) {
    case type_column:
      return ChordsModel::tr("Type");
    case instrument_column:
      return ChordsModel::tr("Instrument");
    case interval_or_percussion_column:
      return ChordsModel::tr("Interval or Percussion");
    case beats_column:
      return ChordsModel::tr("Beats");
    case velocity_ratio_column:
      return ChordsModel::tr("Velocity ratio");
    case tempo_ratio_column:
      return ChordsModel::tr("Tempo ratio");
    case words_column:
      return ChordsModel::tr("Words");
    default:
      Q_ASSERT(false);
      return {};
    }
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto ChordsModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  auto selectable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  auto note_chord_column = get_note_chord_column(index);
  return ((note_chord_column == type_column) ||
          (valid_is_chord_index(index) &&
           (note_chord_column == instrument_column)))
             ? selectable
             : selectable | Qt::ItemIsEditable;
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  const NoteChord *note_chord_pointer = nullptr;
  auto child_number = get_child_number(index);
  auto is_chord = valid_is_chord_index(index);
  if (is_chord) {
    note_chord_pointer = &get_const_item(chords, child_number);
  } else {
    note_chord_pointer = &(get_const_item(
        get_const_item(chords, get_parent_chord_number(index)).notes,
        child_number));
  }
  if (role == Qt::StatusTipRole) {
    auto key = starting_key;
    if (is_chord) {
      for (size_t chord_number = 0; chord_number <= child_number;
           chord_number++) {
        const auto &interval_or_percussion_pointer =
            get_const_item(chords, chord_number).interval_or_percussion_pointer;
        Q_ASSERT(
            std::holds_alternative<Interval>(interval_or_percussion_pointer));
        key = key * interval_to_double(
                        std::get<Interval>(interval_or_percussion_pointer));
      }
    } else {
      auto parent_chord_number = get_parent_chord_number(index);
      for (size_t chord_number = 0; chord_number <= parent_chord_number;
           chord_number++) {
        const auto &interval_or_percussion_pointer =
            get_const_item(chords, chord_number).interval_or_percussion_pointer;
        Q_ASSERT(
            std::holds_alternative<Interval>(interval_or_percussion_pointer));
        key = key * interval_to_double(
                        std::get<Interval>(interval_or_percussion_pointer));
      }
      const auto &note_interval_or_percussion_pointer =
          get_const_item(get_const_item(chords, parent_chord_number).notes,
                         child_number)
              .interval_or_percussion_pointer;
      if (std::holds_alternative<const Percussion *>(
              note_interval_or_percussion_pointer)) {
        return "";
      }
      key = key * interval_to_double(
                      std::get<Interval>(note_interval_or_percussion_pointer));
    }
    auto midi_float = get_midi(key);
    auto closest_midi = round(midi_float);
    auto difference_from_c = closest_midi - C_0_MIDI;
    auto octave =
        static_cast<int>(floor(difference_from_c / HALFSTEPS_PER_OCTAVE));
    auto scale =
        static_cast<int>(difference_from_c - octave * HALFSTEPS_PER_OCTAVE);
    auto cents = static_cast<int>(
        round((midi_float - closest_midi) * CENTS_PER_HALFSTEP));
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
  if (role == Qt::BackgroundRole) {
    const auto &palette = parent_pointer->palette();
    return note_chord_pointer->is_chord() ? palette.base()
                                          : palette.alternateBase();
  }
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    const auto &interval_or_percussion_pointer =
        note_chord_pointer->interval_or_percussion_pointer;
    switch (get_note_chord_column(index)) {
    case type_column:
      return note_chord_pointer->is_chord() ? "Chord" : "Note";
    case instrument_column:
      return QVariant::fromValue(note_chord_pointer->instrument_pointer);
    case interval_or_percussion_column:
      if (std::holds_alternative<const Percussion *>(
              interval_or_percussion_pointer)) {
        return QVariant::fromValue(
            std::get<const Percussion *>(interval_or_percussion_pointer));
      } else {
        return QVariant::fromValue(
            std::get<Interval>(interval_or_percussion_pointer));
      }
    case (beats_column):
      return QVariant::fromValue(note_chord_pointer->beats);
    case velocity_ratio_column:
      return QVariant::fromValue(note_chord_pointer->velocity_ratio);
    case tempo_ratio_column:
      return QVariant::fromValue(note_chord_pointer->tempo_ratio);
    case words_column:
      return note_chord_pointer->words;
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
  auto note_chord_column = get_note_chord_column(index);
  if (valid_is_chord_index(index)) {
    auto chord_number = get_child_number(index);
    const auto &chord = get_const_item(chords, chord_number);
    const auto &old_interval_or_percussion_pointer =
        chord.interval_or_percussion_pointer;
    switch (note_chord_column) {
    case type_column:
      Q_ASSERT(false);
    case interval_or_percussion_column:
      Q_ASSERT(
          std::holds_alternative<Interval>(old_interval_or_percussion_pointer));
      Q_ASSERT(new_value.canConvert<const Interval>());
      undo_stack_pointer->push(
          std::make_unique<SetChordInterval>(
              this, chord_number,
              std::get<Interval>(old_interval_or_percussion_pointer),
              new_value.value<Interval>())
              .release());
      break;
    case beats_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      undo_stack_pointer->push(
          std::make_unique<SetChordBeats>(this, chord_number, chord.beats,
                                          new_value.value<Rational>())
              .release());
      break;
    case velocity_ratio_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      undo_stack_pointer->push(std::make_unique<SetChordVelocityRatio>(
                                   this, chord_number, chord.velocity_ratio,
                                   new_value.value<Rational>())
                                   .release());
      break;
    case tempo_ratio_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      undo_stack_pointer->push(std::make_unique<SetChordTempoRatio>(
                                   this, chord_number, chord.tempo_ratio,
                                   new_value.value<Rational>())
                                   .release());
      break;
    case words_column:
      Q_ASSERT(new_value.canConvert<QString>());
      undo_stack_pointer->push(
          std::make_unique<SetChordWords>(this, chord_number, chord.words,
                                          new_value.value<QString>())
              .release());
      break;
    default:
      Q_ASSERT(false);
    }
  } else {
    auto chord_number = get_parent_chord_number(index);
    auto note_number = get_child_number(index);

    const auto &note =
        get_const_item(get_const_item(chords, chord_number).notes, note_number);
    const auto &old_interval_or_percussion_pointer =
        note.interval_or_percussion_pointer;

    if (note_chord_column == instrument_column) {
      const auto *old_instrument_pointer = note.instrument_pointer;
      Q_ASSERT(old_instrument_pointer != nullptr);

      Q_ASSERT(new_value.canConvert<const Instrument *>());
      const auto *new_instrument_pointer =
          new_value.value<const Instrument *>();
      Q_ASSERT(new_instrument_pointer != nullptr);
      auto new_is_percussion = new_instrument_pointer->is_percussion;

      if (old_instrument_pointer->is_percussion == new_is_percussion) {
        undo_stack_pointer->push(
            std::make_unique<SetNoteInstrument>(this, chord_number, note_number,
                                                old_instrument_pointer,
                                                new_instrument_pointer)
                .release());
      } else {
        const auto &old_interval_or_percussion_pointer =
            note.interval_or_percussion_pointer;
        if (new_is_percussion) {
          Q_ASSERT(std::holds_alternative<Interval>(
              old_interval_or_percussion_pointer));
          undo_stack_pointer->push(
              std::make_unique<ChangeToPercussion>(
                  this, chord_number, note_number, old_instrument_pointer,
                  new_instrument_pointer,
                  std::get<Interval>(old_interval_or_percussion_pointer),
                  get_percussion_pointer("Tambourine"))
                  .release());
        } else {
          Q_ASSERT(std::holds_alternative<const Percussion *>(
              old_interval_or_percussion_pointer));
          undo_stack_pointer->push(std::make_unique<ChangeToInterval>(
                                       this, chord_number, note_number,
                                       old_instrument_pointer,
                                       new_instrument_pointer,
                                       std::get<const Percussion *>(
                                           note.interval_or_percussion_pointer),
                                       Interval())
                                       .release());
        }
      }
    }
    switch (note_chord_column) {
    case type_column:
      Q_ASSERT(false);
    case interval_or_percussion_column:
      if (std::holds_alternative<const Percussion *>(
              old_interval_or_percussion_pointer)) {
        Q_ASSERT(new_value.canConvert<const Percussion *>());
        undo_stack_pointer->push(std::make_unique<SetNotePercussion>(
                                     this, chord_number, note_number,
                                     std::get<const Percussion *>(
                                         old_interval_or_percussion_pointer),
                                     new_value.value<const Percussion *>())
                                     .release());
      }
      Q_ASSERT(new_value.canConvert<const Interval>());
      undo_stack_pointer->push(
          std::make_unique<SetNoteInterval>(
              this, chord_number, note_number,
              std::get<Interval>(old_interval_or_percussion_pointer),
              new_value.value<Interval>())
              .release());
      break;
    case beats_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      undo_stack_pointer->push(std::make_unique<SetNoteBeats>(
                                   this, chord_number, note_number, note.beats,
                                   new_value.value<Rational>())
                                   .release());
      break;
    case velocity_ratio_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      undo_stack_pointer->push(std::make_unique<SetNoteVelocityRatio>(
                                   this, chord_number, note_number,
                                   note.velocity_ratio,
                                   new_value.value<Rational>())
                                   .release());
      break;
    case tempo_ratio_column:
      Q_ASSERT(new_value.canConvert<Rational>());
      undo_stack_pointer->push(
          std::make_unique<SetNoteTempoRatio>(this, chord_number, note_number,
                                              note.tempo_ratio,
                                              new_value.value<Rational>())
              .release());
      break;
    case words_column:
      Q_ASSERT(new_value.canConvert<QString>());
      undo_stack_pointer->push(
          std::make_unique<SetNoteWords>(this, chord_number, note_number,
                                         note.words, new_value.value<QString>())
              .release());
      break;
    default:
      Q_ASSERT(false);
    }
  }
  parent_pointer->setFocus();
  return true;
}

void ChordsModel::set_chord_interval(size_t chord_number,
                                     const Interval &new_interval) {
  auto &chord = get_item(chords, chord_number);
  chord.interval_or_percussion_pointer = new_interval;
  auto index = get_chord_index(chord_number, interval_or_percussion_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_chord_beats(size_t chord_number,
                                  const Rational &new_beats) {
  auto &chord = get_item(chords, chord_number);
  chord.beats = new_beats;
  auto index = get_chord_index(chord_number, beats_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_chord_velocity_ratio(size_t chord_number,
                                           const Rational &new_velocity_ratio) {
  auto &chord = get_item(chords, chord_number);
  chord.velocity_ratio = new_velocity_ratio;
  auto index = get_chord_index(chord_number, velocity_ratio_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_chord_tempo_ratio(size_t chord_number,
                                        const Rational &new_tempo_ratio) {
  auto &chord = get_item(chords, chord_number);
  chord.tempo_ratio = new_tempo_ratio;
  auto index = get_chord_index(chord_number, tempo_ratio_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_chord_words(size_t chord_number,
                                  const QString &new_words) {
  auto &chord = get_item(chords, chord_number);
  chord.words = new_words;
  auto index = get_chord_index(chord_number, words_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}
void ChordsModel::set_note_instrument(
    size_t chord_number, size_t note_number,
    const Instrument *new_instrument_pointer) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.instrument_pointer = new_instrument_pointer;
  auto index = get_note_index(chord_number, note_number, instrument_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_note_interval(size_t chord_number, size_t note_number,
                                    const Interval &new_interval) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.interval_or_percussion_pointer = new_interval;
  auto index =
      get_note_index(chord_number, note_number, interval_or_percussion_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_note_percussion(
    size_t chord_number, size_t note_number,
    const Percussion *new_percussion_pointer) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.interval_or_percussion_pointer = new_percussion_pointer;
  auto index =
      get_note_index(chord_number, note_number, interval_or_percussion_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_note_beats(size_t chord_number, size_t note_number,
                                 const Rational &new_beats) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.beats = new_beats;
  auto index = get_note_index(chord_number, note_number, beats_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_note_velocity_ratio(size_t chord_number,
                                          size_t note_number,
                                          const Rational &new_velocity_ratio) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.velocity_ratio = new_velocity_ratio;
  auto index = get_note_index(chord_number, note_number, velocity_ratio_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_note_tempo_ratio(size_t chord_number, size_t note_number,
                                       const Rational &new_tempo_ratio) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.tempo_ratio = new_tempo_ratio;
  auto index = get_note_index(chord_number, note_number, tempo_ratio_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_note_words(size_t chord_number, size_t note_number,
                                 const QString &new_words) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.words = new_words;
  auto index = get_note_index(chord_number, note_number, words_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::change_to_interval(size_t chord_number, size_t note_number,
                                     const Instrument *instrument_pointer,
                                     const Interval &new_interval) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.instrument_pointer = instrument_pointer;
  note.interval_or_percussion_pointer = new_interval;
  emit dataChanged(
      get_note_index(chord_number, note_number, instrument_column),
      get_note_index(chord_number, note_number, interval_or_percussion_column),
      {Qt::DisplayRole, Qt::EditRole});
};

void ChordsModel::change_to_percussion(size_t chord_number, size_t note_number,
                                       const Instrument *instrument_pointer,
                                       const Percussion *percussion_pointer) {
  auto &note = get_item(get_item(chords, chord_number).notes, note_number);
  note.instrument_pointer = instrument_pointer;
  note.interval_or_percussion_pointer = percussion_pointer;
  emit dataChanged(
      get_note_index(chord_number, note_number, instrument_column),
      get_note_index(chord_number, note_number, interval_or_percussion_column),
      {Qt::DisplayRole, Qt::EditRole});
};

void ChordsModel::replace_cell_ranges(const std::vector<RowRange> &row_ranges,
                                      const std::vector<NoteChord> &note_chords,
                                      NoteChordColumn left_column,
                                      NoteChordColumn right_column) {
  size_t note_chord_number = 0;
  for (const auto &row_range : row_ranges) {
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
    auto last_child_number = get_end_child_number(row_range) - 1;
    if (is_chords(row_range)) {
      for (size_t write_number = 0; write_number < number_of_children;
           write_number++) {
        auto &chord = get_item(chords, first_child_number + write_number);
        const auto &new_note_chord =
            get_const_item(note_chords, note_chord_number + write_number);
        for (auto note_chord_column = left_column;
             note_chord_column <= right_column;
             note_chord_column =
                 static_cast<NoteChordColumn>(note_chord_column + 1)) {
          switch (note_chord_column) {
          case instrument_column:
            break;
          case interval_or_percussion_column:
            chord.interval_or_percussion_pointer =
                new_note_chord.interval_or_percussion_pointer;
            break;
          case beats_column:
            chord.beats = new_note_chord.beats;
            break;
          case velocity_ratio_column:
            chord.velocity_ratio = new_note_chord.velocity_ratio;
            break;
          case tempo_ratio_column:
            chord.tempo_ratio = new_note_chord.tempo_ratio;
            break;
          case words_column:
            chord.words = new_note_chord.words;
            break;
          default:
            Q_ASSERT(false);
            break;
          }
        }
      }
      emit dataChanged(get_chord_index(first_child_number, left_column),
                       get_chord_index(last_child_number, right_column),
                       {Qt::DisplayRole, Qt::EditRole});
      if (left_column <= instrument_column &&
          right_column >= instrument_column) {
        emit dataChanged(
            get_chord_index(first_child_number, interval_or_percussion_column),
            get_chord_index(last_child_number, interval_or_percussion_column),
            {Qt::DisplayRole, Qt::EditRole});
      }
    } else {
      auto chord_number = get_parent_chord_number(row_range);

      auto &notes = get_item(chords, chord_number).notes;
      for (size_t replace_number = 0; replace_number < number_of_children;
           replace_number = replace_number + 1) {
        auto new_note_chord_number = note_chord_number + replace_number;
        auto &note = get_item(notes, first_child_number + replace_number);
        const auto &new_note_chord =
            get_const_item(note_chords, new_note_chord_number);
        for (auto note_chord_column = left_column;
             note_chord_column <= right_column;
             note_chord_column =
                 static_cast<NoteChordColumn>(note_chord_column + 1)) {
          switch (note_chord_column) {
          case instrument_column:
            note.instrument_pointer = new_note_chord.instrument_pointer;
            break;
          case interval_or_percussion_column:
            note.interval_or_percussion_pointer =
                new_note_chord.interval_or_percussion_pointer;
            break;
          case beats_column:
            note.beats = new_note_chord.beats;
            break;
          case velocity_ratio_column:
            note.velocity_ratio = new_note_chord.velocity_ratio;
            break;
          case tempo_ratio_column:
            note.tempo_ratio = new_note_chord.tempo_ratio;
            break;
          case words_column:
            note.words = new_note_chord.words;
            break;
          default:
            Q_ASSERT(false);
            break;
          }
        }
      }
      emit dataChanged(
          get_note_index(chord_number, first_child_number, left_column),
          get_note_index(chord_number, last_child_number, right_column),
          {Qt::DisplayRole, Qt::EditRole});
      if (left_column <= instrument_column &&
          right_column >= instrument_column) {
        emit dataChanged(get_note_index(chord_number, first_child_number,
                                        interval_or_percussion_column),
                         get_note_index(chord_number, last_child_number,
                                        interval_or_percussion_column),
                         {Qt::DisplayRole, Qt::EditRole});
      }
    }

    note_chord_number = note_chord_number + number_of_children;
  }
}

void ChordsModel::insert_chord(size_t chord_number, const Chord &new_chord) {
  auto int_chord_number = static_cast<int>(chord_number);
  check_end_number(chords, int_chord_number);

  beginInsertRows(QModelIndex(), int_chord_number, int_chord_number);
  chords.insert(chords.begin() + int_chord_number, new_chord);
  endInsertRows();
}

void ChordsModel::insert_chords(size_t first_chord_number,
                                const std::vector<Chord> &new_chords) {
  auto int_first_chord_number = static_cast<int>(first_chord_number);

  check_end_number(chords, first_chord_number);

  beginInsertRows(QModelIndex(), int_first_chord_number,
                  static_cast<int>(first_chord_number + new_chords.size()) - 1);
  chords.insert(chords.begin() + int_first_chord_number, new_chords.begin(),
                new_chords.end());
  endInsertRows();
}

void ChordsModel::append_json_chords(const nlohmann::json &json_chords) {
  auto chords_size = chords.size();

  beginInsertRows(QModelIndex(), static_cast<int>(chords_size),
                  static_cast<int>(chords_size + json_chords.size()) - 1);
  json_to_chords(chords, json_chords);
  endInsertRows();
}

void ChordsModel::remove_chords(size_t first_chord_number,
                                size_t number_of_chords) {
  auto int_first_chord_number = static_cast<int>(first_chord_number);
  auto int_end_child_number =
      static_cast<int>(first_chord_number + number_of_chords);

  check_range(chords, first_chord_number, number_of_chords);

  beginRemoveRows(QModelIndex(), int_first_chord_number,
                  int_end_child_number - 1);
  chords.erase(chords.begin() + int_first_chord_number,
               chords.begin() + int_end_child_number);
  endRemoveRows();
}

void ChordsModel::insert_note(size_t chord_number, size_t note_number,
                              const Note &new_note) {
  auto int_note_number = static_cast<int>(note_number);

  auto &notes = get_item(chords, chord_number).notes;
  check_end_number(notes, int_note_number);

  beginInsertRows(get_chord_index(chord_number), int_note_number,
                  int_note_number);
  notes.insert(notes.begin() + int_note_number, new_note);
  endInsertRows();
}

void ChordsModel::insert_notes(size_t chord_number, size_t first_note_number,
                               const std::vector<Note> &new_notes) {
  auto int_first_note_number = static_cast<int>(first_note_number);

  auto &notes = get_item(chords, chord_number).notes;
  check_end_number(notes, first_note_number);

  beginInsertRows(get_chord_index(chord_number), int_first_note_number,
                  static_cast<int>(first_note_number + new_notes.size()) - 1);
  notes.insert(notes.begin() + int_first_note_number, new_notes.begin(),
               new_notes.end());
  endInsertRows();
};

void ChordsModel::remove_notes(size_t chord_number, size_t first_note_number,
                               size_t number_of_notes) {
  auto int_first_note_number = static_cast<int>(first_note_number);
  auto int_end_child_number =
      static_cast<int>(first_note_number + number_of_notes);

  auto &notes = get_item(chords, chord_number).notes;
  check_range(notes, first_note_number, number_of_notes);

  beginRemoveRows(get_chord_index(chord_number), int_first_note_number,
                  int_end_child_number - 1);
  notes.erase(notes.begin() + int_first_note_number,
              notes.begin() + int_end_child_number);
  endRemoveRows();
}
