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
  check_number(chords, chord_number);
  return createIndex(static_cast<int>(chord_number), note_chord_column,
                     nullptr);
}

auto ChordsModel::get_note_index(size_t chord_number, size_t note_number,
                                 NoteChordColumn note_chord_column) const
    -> QModelIndex {
  check_number(get_const_item(chords, note_number).notes, note_number);
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
    } else {
      switch (note_chord_column) {
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
        } else {
          Q_ASSERT(new_value.canConvert<Interval>());
          undo_stack_pointer->push(
              std::make_unique<SetNoteInterval>(
                  this, chord_number, note_number,
                  std::get<Interval>(old_interval_or_percussion_pointer),
                  new_value.value<Interval>())
                  .release());
        }
        break;
      case beats_column:
        Q_ASSERT(new_value.canConvert<Rational>());
        undo_stack_pointer->push(std::make_unique<SetNoteBeats>(
                                     this, chord_number, note_number,
                                     note.beats, new_value.value<Rational>())
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
        undo_stack_pointer->push(std::make_unique<SetNoteWords>(
                                     this, chord_number, note_number,
                                     note.words, new_value.value<QString>())
                                     .release());
        break;
      default:
        Q_ASSERT(false);
      }
    }
  }
  parent_pointer->setFocus();
  return true;
}

void ChordsModel::edited_chords_cells(size_t first_chord_number,
                                      size_t number_of_chords,
                                      NoteChordColumn left_column,
                                      NoteChordColumn right_column) {
  emit dataChanged(
      get_chord_index(first_chord_number, left_column),
      get_chord_index(first_chord_number + number_of_chords - 1, right_column),
      {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::edited_notes_cells(size_t chord_number,
                                     size_t first_note_number,
                                     size_t number_of_notes,
                                     NoteChordColumn left_column,
                                     NoteChordColumn right_column) {
  emit dataChanged(get_note_index(chord_number, first_note_number, left_column),
                   get_note_index(chord_number,
                                  first_note_number + number_of_notes - 1,
                                  right_column),
                   {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::begin_insert_chords(size_t first_chord_number,
                                      size_t number_of_chords) {
  beginInsertRows(QModelIndex(), static_cast<int>(first_chord_number),
                  static_cast<int>(first_chord_number + number_of_chords) - 1);
}

void ChordsModel::begin_insert_notes(size_t chord_number,
                                     size_t first_note_number,
                                     size_t number_of_notes) {
  beginInsertRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + number_of_notes) - 1);
}

void ChordsModel::end_insert_rows() { endInsertRows(); }

void ChordsModel::begin_remove_chords(size_t first_chord_number,
                                      size_t number_of_chords) {
  beginRemoveRows(QModelIndex(), static_cast<int>(first_chord_number),
                  static_cast<int>(first_chord_number + number_of_chords) - 1);
}

void ChordsModel::begin_remove_notes(size_t chord_number,
                                     size_t first_note_number,
                                     size_t number_of_notes) {
  beginRemoveRows(get_chord_index(chord_number),
                  static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + number_of_notes) - 1);
}

void ChordsModel::end_remove_rows() { endRemoveRows(); }

void insert_chord(ChordsModel *chords_model_pointer, size_t chord_number,
                  const Chord &new_chord) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &chords = chords_model_pointer->chords;

  check_end_number(chords, chord_number);

  chords_model_pointer->begin_insert_chords(chord_number, 1);
  chords.insert(chords.begin() + static_cast<int>(chord_number), new_chord);
  chords_model_pointer->end_insert_rows();
}

void insert_chords(ChordsModel *chords_model_pointer, size_t first_chord_number,
                   const std::vector<Chord> &new_chords) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &chords = chords_model_pointer->chords;

  check_end_number(chords, first_chord_number);

  chords_model_pointer->begin_insert_chords(first_chord_number,
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

  chords_model_pointer->begin_remove_chords(first_chord_number,
                                            number_of_chords);
  chords.erase(chords.begin() + static_cast<int>(first_chord_number),
               chords.begin() +
                   static_cast<int>(first_chord_number + number_of_chords));
  chords_model_pointer->end_remove_rows();
}

void insert_notes(ChordsModel *chords_model_pointer, size_t chord_number,
                  size_t first_note_number,
                  const std::vector<Note> &new_notes) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &notes = get_item(chords_model_pointer->chords, chord_number).notes;
  check_end_number(notes, first_note_number);

  chords_model_pointer->begin_insert_notes(chord_number, first_note_number,
                                           new_notes.size());
  notes.insert(notes.begin() + static_cast<int>(first_note_number),
               new_notes.begin(), new_notes.end());
  chords_model_pointer->end_insert_rows();
};

void remove_notes(ChordsModel *chords_model_pointer, size_t chord_number,
                  size_t first_note_number, size_t number_of_notes) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &notes = get_item(chords_model_pointer->chords, chord_number).notes;
  check_range(notes, first_note_number, number_of_notes);

  chords_model_pointer->begin_remove_notes(chord_number, first_note_number,
                                           number_of_notes);
  notes.erase(notes.begin() + static_cast<int>(first_note_number),
              notes.begin() +
                  static_cast<int>(first_note_number + number_of_notes));
  chords_model_pointer->end_remove_rows();
}

void change_to_interval(ChordsModel *chords_model_pointer, size_t chord_number,
                        size_t note_number,
                        const Instrument *instrument_pointer,
                        const Interval &new_interval) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &note = get_item(
      get_item(chords_model_pointer->chords, chord_number).notes, note_number);
  note.instrument_pointer = instrument_pointer;
  note.interval_or_percussion_pointer = new_interval;
  chords_model_pointer->edited_notes_cells(chord_number, note_number, 1,
                                           instrument_column,
                                           interval_or_percussion_column);
};

void change_to_percussion(ChordsModel *chords_model_pointer,
                          size_t chord_number, size_t note_number,
                          const Instrument *instrument_pointer,
                          const Percussion *percussion_pointer) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &note = get_item(
      get_item(chords_model_pointer->chords, chord_number).notes, note_number);
  note.instrument_pointer = instrument_pointer;
  note.interval_or_percussion_pointer = percussion_pointer;
  chords_model_pointer->edited_notes_cells(chord_number, note_number, 1,
                                           instrument_column,
                                           interval_or_percussion_column);
};
