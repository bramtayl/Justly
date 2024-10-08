#include "note/NotesModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <iterator>
#include <memory>

#include "chord/ChordsModel.hpp"
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

NotesModel::NotesModel(ChordsModel *parent_chords_model_pointer_input,
                       QObject *parent_pointer)
    : ItemModel(parent_pointer),
      parent_chords_model_pointer(parent_chords_model_pointer_input) {
  Q_ASSERT(parent_chords_model_pointer != nullptr);
}

auto NotesModel::rowCount(const QModelIndex & /*parent_index*/) const -> int {
  Q_ASSERT(notes_pointer != nullptr);
  return static_cast<int>(notes_pointer->size());
}

auto NotesModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_NOTE_COLUMNS;
}

auto NotesModel::get_column_name(int column_number) const -> QString {
  switch (to_note_column(column_number)) {
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

auto NotesModel::data(const QModelIndex &index, int role) const -> QVariant {
  if (role == Qt::StatusTipRole) {
    Q_ASSERT(notes_pointer != nullptr);
    return get_key_text(
        *parent_chords_model_pointer, parent_chord_number,
        interval_to_double(
            notes_pointer->at(get_row_number(index)).interval));
  }
  if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return {};
  }
  Q_ASSERT(notes_pointer != nullptr);
  const auto &note = notes_pointer->at(get_row_number(index));
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

auto NotesModel::setData(const QModelIndex &index, const QVariant &new_value,
                         int role) -> bool {
  auto *undo_stack_pointer = parent_chords_model_pointer->undo_stack_pointer;
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  auto note_number = get_row_number(index);
  Q_ASSERT(notes_pointer != nullptr);
  const auto &note = notes_pointer->at(note_number);
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
  return true;
}

void insert_notes(NotesModel &notes_model, qsizetype first_note_number,
                  const QList<Note> &new_notes) {
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);

  notes_model.begin_insert_rows(first_note_number, new_notes.size());
  std::copy(new_notes.cbegin(), new_notes.cend(),
            std::inserter(*notes_pointer,
                          notes_pointer->begin() + first_note_number));
  notes_model.end_insert_rows();
};

void remove_notes(NotesModel &notes_model, qsizetype first_note_number,
                  qsizetype number_of_notes) {
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);

  notes_model.begin_remove_rows(first_note_number, number_of_notes);
  notes_pointer->erase(
      notes_pointer->begin() + static_cast<int>(first_note_number),
      notes_pointer->begin() +
          static_cast<int>(first_note_number + number_of_notes));
  notes_model.end_remove_rows();
}
