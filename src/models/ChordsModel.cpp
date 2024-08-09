#include "models/ChordsModel.hpp"

#include <QAbstractItemModel>
#include <QBrush>
#include <QList>
#include <QObject>
#include <QPalette>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <cstddef>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

#include "cell_values/Instrument.hpp"
#include "cell_values/Interval.hpp"
#include "cell_values/Rational.hpp"
#include "changes/InsertChords.hpp"
#include "changes/InsertNotes.hpp"
#include "changes/RemoveChords.hpp"
#include "changes/RemoveNotes.hpp"
#include "changes/SetChordCell.hpp"
#include "changes/SetNoteCell.hpp"
#include "indices/RowRange.hpp"
#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "note_chord/NoteChord.hpp"
#include "other/conversions.hpp"
#include "other/templates.hpp"

static const auto NUMBER_OF_NOTE_CHORD_COLUMNS = 7;

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

static auto set_note_chord_data(NoteChord *note_chord_pointer,
                                NoteChordColumn note_chord_column,
                                const QVariant &new_value) {
  Q_ASSERT(note_chord_pointer != nullptr);
  switch (note_chord_column) {
  case instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    note_chord_pointer->instrument_pointer =
        new_value.value<const Instrument *>();
    break;
  case interval_column:
    Q_ASSERT(new_value.canConvert<Interval>());
    note_chord_pointer->interval = new_value.value<Interval>();
    break;
  case beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    note_chord_pointer->beats = new_value.value<Rational>();
    break;
  case velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    note_chord_pointer->velocity_ratio = new_value.value<Rational>();
    break;
  case tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    note_chord_pointer->tempo_ratio = new_value.value<Rational>();
    break;
  case words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    note_chord_pointer->words = new_value.toString();
    break;
  default:
    Q_ASSERT(false);
  }
};

static auto replace_cells(NoteChord *note_chord_pointer,
                          const NoteChord &new_note_chord,
                          NoteChordColumn left_column,
                          NoteChordColumn right_column) {
  Q_ASSERT(note_chord_pointer != nullptr);
  for (auto note_chord_column = left_column; note_chord_column <= right_column;
       note_chord_column =
           static_cast<NoteChordColumn>(note_chord_column + 1)) {
    switch (note_chord_column) {
    case instrument_column:
      note_chord_pointer->instrument_pointer =
          new_note_chord.instrument_pointer;
      break;
    case interval_column:
      note_chord_pointer->interval = new_note_chord.interval;
      break;
    case beats_column:
      note_chord_pointer->beats = new_note_chord.beats;
      break;
    case velocity_ratio_column:
      note_chord_pointer->velocity_ratio = new_note_chord.velocity_ratio;
      break;
    case tempo_ratio_column:
      note_chord_pointer->tempo_ratio = new_note_chord.tempo_ratio;
      break;
    case words_column:
      note_chord_pointer->words = new_note_chord.words;
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }
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

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         QWidget *parent_pointer_input)
    : QAbstractItemModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {
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
    case interval_column:
      return ChordsModel::tr("Interval");
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
  // type column isn't editable
  return get_note_chord_column(index) == type_column
             ? selectable
             : (selectable | Qt::ItemIsEditable);
}

auto ChordsModel::data(const QModelIndex &index, int role) const -> QVariant {
  const NoteChord *note_chord_pointer = nullptr;
  auto child_number = get_child_number(index);
  if (valid_is_chord_index(index)) {
    note_chord_pointer = &get_const_item(chords, child_number);
  } else {
    note_chord_pointer = &(get_const_item(
        get_const_item(chords, get_parent_chord_number(index)).notes,
        child_number));
  }
  if (role == Qt::BackgroundRole) {
    const auto &palette = parent_pointer->palette();
    return note_chord_pointer->is_chord() ? palette.base()
                                          : palette.alternateBase();
  }
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (get_note_chord_column(index)) {
    case type_column:
      return note_chord_pointer->is_chord() ? "♪" : "♫";
    case instrument_column:
      return QVariant::fromValue(note_chord_pointer->instrument_pointer);
    case interval_column:
      return QVariant::fromValue(note_chord_pointer->interval);
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
  if (valid_is_chord_index(index)) {
    undo_stack_pointer->push(
        std::make_unique<SetChordCell>(this, get_child_number(index),
                                       get_note_chord_column(index),
                                       data(index, Qt::EditRole), new_value)
            .release());
  } else {
    undo_stack_pointer->push(
        std::make_unique<SetNoteCell>(
            this, get_parent_chord_number(index), get_child_number(index),
            get_note_chord_column(index), data(index, Qt::EditRole), new_value)
            .release());
  }
  return true;
}

auto ChordsModel::insertRows(int signed_first_child_number,
                             int signed_number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto first_child_number = to_size_t(signed_first_child_number);
  auto number_of_children = to_size_t(signed_number_of_children);

  if (is_root_index(parent_index)) {
    Chord template_chord;
    if (first_child_number > 0) {
      template_chord.beats =
          get_const_item(chords, first_child_number - 1).beats;
    }

    std::vector<Chord> new_chords;
    for (size_t index = 0; index < number_of_children; index = index + 1) {
      new_chords.push_back(template_chord);
    }
    undo_stack_pointer->push(
        std::make_unique<InsertChords>(this, first_child_number,
                                       std::move(new_chords))
            .release());
  } else {
    auto chord_number = get_child_number(parent_index);
    const auto &parent_chord = get_const_item(chords, chord_number);

    Note template_note;

    if (first_child_number == 0) {
      template_note.beats = parent_chord.beats;
      template_note.words = parent_chord.words;
    } else {
      const auto &previous_note =
          get_const_item(parent_chord.notes, first_child_number - 1);

      template_note.beats = previous_note.beats;
      template_note.velocity_ratio = previous_note.velocity_ratio;
      template_note.tempo_ratio = previous_note.tempo_ratio;
      template_note.words = previous_note.words;
    }

    std::vector<Note> new_notes;
    for (size_t index = 0; index < number_of_children; index = index + 1) {
      new_notes.push_back(template_note);
    }
    undo_stack_pointer->push(std::make_unique<InsertNotes>(this, chord_number,
                                                           first_child_number,
                                                           std::move(new_notes))
                                 .release());
  }
  return true;
}

auto ChordsModel::removeRows(int signed_first_child_number,
                             int signed_number_of_children,
                             const QModelIndex &parent_index) -> bool {
  auto first_child_number = to_size_t(signed_first_child_number);
  auto number_of_children = to_size_t(signed_number_of_children);

  if (is_root_index(parent_index)) {
    check_range(chords, first_child_number, number_of_children);
    undo_stack_pointer->push(
        std::make_unique<RemoveChords>(
            this, first_child_number,
            std::vector<Chord>(
                chords.cbegin() + signed_first_child_number,
                chords.cbegin() +
                    static_cast<int>(first_child_number + number_of_children)))
            .release());
  } else {
    auto chord_number = get_child_number(parent_index);
    const auto &chord = get_const_item(chords, chord_number);
    const auto &notes = chord.notes;
    check_range(notes, first_child_number, number_of_children);
    undo_stack_pointer->push(
        std::make_unique<RemoveNotes>(
            this, chord_number, first_child_number,
            std::vector<Note>(
                notes.cbegin() + static_cast<int>(first_child_number),
                notes.cbegin() +
                    static_cast<int>(first_child_number + number_of_children)))
            .release());
  }
  return true;
}

void ChordsModel::set_chord_cell(size_t chord_number,
                                 NoteChordColumn note_chord_column,
                                 const QVariant &new_value) {
  set_note_chord_data(&get_item(chords, chord_number), note_chord_column,
                      new_value);
  auto index = get_chord_index(chord_number, note_chord_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::set_note_cell(size_t chord_number, size_t note_number,
                                NoteChordColumn note_chord_column,
                                const QVariant &new_value) {
  set_note_chord_data(
      &get_item(get_item(chords, chord_number).notes, note_number),
      note_chord_column, new_value);
  auto index = get_note_index(chord_number, note_number, note_chord_column);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

void ChordsModel::replace_cell_ranges(const std::vector<RowRange> &row_ranges,
                                      const std::vector<NoteChord> &note_chords,
                                      NoteChordColumn left_column,
                                      NoteChordColumn right_column) {
  size_t note_chord_number = 0;
  for (const auto &row_range : row_ranges) {
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
    auto last_child_number = get_end_child_number(row_range) - 1;
    QModelIndex first_index;
    QModelIndex last_index;
    if (is_chords(row_range)) {
      for (size_t write_number = 0; write_number < number_of_children;
           write_number++) {
        replace_cells(&get_item(chords, first_child_number + write_number),
                      get_const_item(note_chords, note_chord_number + write_number), left_column,
                      right_column);
      }
      first_index = get_chord_index(first_child_number, left_column);
      last_index = get_chord_index(last_child_number, right_column);
    } else {
      auto chord_number = get_parent_chord_number(row_range);

      auto &notes = get_item(chords, chord_number).notes;
      for (size_t replace_number = 0; replace_number < number_of_children;
           replace_number = replace_number + 1) {
        auto new_note_chord_number = note_chord_number + replace_number;
        replace_cells(&get_item(notes, first_child_number + replace_number),
                      get_const_item(note_chords, new_note_chord_number), left_column,
                      right_column);
      }
      first_index =
          get_note_index(chord_number, first_child_number, left_column);
      last_index =
          get_note_index(chord_number, last_child_number, right_column);
    }
    emit dataChanged(first_index, last_index, {Qt::DisplayRole, Qt::EditRole});
    note_chord_number = note_chord_number + number_of_children;
  }
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
