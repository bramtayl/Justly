#include "note/NotesModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QObject>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/SetNoteBeats.hpp"
#include "note/SetNoteInstrument.hpp"
#include "note/SetNoteInterval.hpp"
#include "note/SetNoteTempoRatio.hpp"
#include "note/SetNoteVelocityRatio.hpp"
#include "note/SetNoteWords.hpp"
#include "rational/Rational.hpp"

// IWYU pragma: no_include <algorithm>

static const auto NUMBER_OF_NOTE_COLUMNS = 6;

// static functions
[[nodiscard]] static auto
get_note_column(const QModelIndex &index) -> NoteColumn {
  return to_note_column(index.column());
}

// header functions

auto to_note_column(int column) -> NoteColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_NOTE_COLUMNS);
  return static_cast<NoteColumn>(column);
}

NotesModel::NotesModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input)
    : QAbstractTableModel(parent_pointer_input),
      parent_pointer(parent_pointer_input),
      undo_stack_pointer(undo_stack_pointer_input) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto NotesModel::rowCount(const QModelIndex & /*parent_index*/) const -> int {
  return static_cast<int>(notes.size());
}

auto NotesModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_NOTE_COLUMNS;
}

auto NotesModel::headerData(int column, Qt::Orientation orientation,
                            int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (to_note_column(column)) {
    case note_instrument_column:
      return NotesModel::tr("Instrument");
    case note_interval_column:
      return NotesModel::tr("Interval");
    case note_beats_column:
      return NotesModel::tr("Beats");
    case note_velocity_ratio_column:
      return NotesModel::tr("Velocity ratio");
    case note_tempo_ratio_column:
      return NotesModel::tr("Tempo ratio");
    case note_words_column:
      return NotesModel::tr("Words");
    }
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto NotesModel::flags(const QModelIndex & /*index*/) const -> Qt::ItemFlags {
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

auto NotesModel::data(const QModelIndex &index, int role) const -> QVariant {
  const auto &note = notes.at(get_child_number(index));
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (get_note_column(index)) {
    case note_instrument_column:
      return QVariant::fromValue(note.instrument_pointer);
    case note_interval_column:
      return QVariant::fromValue(note.interval);
    case note_beats_column:
      return QVariant::fromValue(note.beats);
    case note_velocity_ratio_column:
      return QVariant::fromValue(note.velocity_ratio);
    case note_tempo_ratio_column:
      return QVariant::fromValue(note.tempo_ratio);
    case note_words_column:
      return note.words;
    }
  }
  // no data for other roles
  return {};
}

auto NotesModel::setData(const QModelIndex &index, const QVariant &new_value,
                         int role) -> bool {
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  auto note_number = get_child_number(index);
  const auto &note = notes.at(note_number);
  switch (get_note_column(index)) {
  case note_instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    undo_stack_pointer->push(std::make_unique<SetNoteInstrument>(
                                 this, note_number, note.instrument_pointer,
                                 new_value.value<const Instrument *>())
                                 .release());
    break;
  case note_interval_column:
    Q_ASSERT(new_value.canConvert<Interval>());
    undo_stack_pointer->push(
        std::make_unique<SetNoteInterval>(this, note_number, note.interval,
                                          new_value.value<Interval>())
            .release());

    break;
  case note_beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetNoteBeats>(this, note_number, note.beats,
                                       new_value.value<Rational>())
            .release());
    break;
  case note_velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetNoteVelocityRatio>(
            this, note_number, note.velocity_ratio, new_value.value<Rational>())
            .release());
    break;
  case note_tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetNoteTempoRatio>(this, note_number, note.tempo_ratio,
                                            new_value.value<Rational>())
            .release());
    break;
  case note_words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    undo_stack_pointer->push(
        std::make_unique<SetNoteWords>(this, note_number, note.words,
                                       new_value.value<QString>())
            .release());
    break;
  default:
    Q_ASSERT(false);
  }
  parent_pointer->setFocus();
  return true;
}

void NotesModel::edited_notes_cells(qsizetype first_note_number,
                                    qsizetype number_of_notes,
                                    NoteColumn left_column,
                                    NoteColumn right_column) {
  emit dataChanged(index(first_note_number, left_column),
                   index(first_note_number + number_of_notes - 1, right_column),
                   {Qt::DisplayRole, Qt::EditRole});
}

void NotesModel::begin_insert_rows(qsizetype first_note_number,
                                   qsizetype number_of_notes) {
  beginInsertRows(QModelIndex(), static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + number_of_notes) - 1);
}

void NotesModel::end_insert_rows() { endInsertRows(); }

void NotesModel::begin_remove_rows(qsizetype first_note_number,
                                   qsizetype number_of_notes) {
  beginRemoveRows(QModelIndex(), static_cast<int>(first_note_number),
                  static_cast<int>(first_note_number + number_of_notes) - 1);
}

void NotesModel::end_remove_rows() { endRemoveRows(); }

void insert_notes(NotesModel *notes_model_pointer, qsizetype first_note_number,
                  const QList<Note> &new_notes) {
  Q_ASSERT(notes_model_pointer != nullptr);
  auto &notes = notes_model_pointer->notes;

  notes_model_pointer->begin_insert_rows(first_note_number, new_notes.size());
  std::copy(new_notes.cbegin(),
        new_notes.cend(),
        std::inserter(notes, notes.begin() + first_note_number));
  notes_model_pointer->end_insert_rows();
};

void remove_notes(NotesModel *notes_model_pointer, qsizetype first_note_number,
                  qsizetype number_of_notes) {
  Q_ASSERT(notes_model_pointer != nullptr);
  auto &notes = notes_model_pointer->notes;

  notes_model_pointer->begin_remove_rows(first_note_number, number_of_notes);
  notes.erase(notes.begin() + static_cast<int>(first_note_number),
              notes.begin() +
                  static_cast<int>(first_note_number + number_of_notes));
  notes_model_pointer->end_remove_rows();
}
