#include "note/SetNotesCells.hpp"

#include <QString>
#include <QtGlobal>

#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"

#include "rational/Rational.hpp"

static void replace_note_cells(NotesModel& notes_model,
                               qsizetype first_note_number, NoteColumn left_column,
                               NoteColumn right_column,
                               const QList<Note> &new_notes) {
  qInfo("here");
  qInfo() << "left column" << left_column;
  qInfo() << "right column" << right_column;
  auto *notes_pointer = notes_model.notes_pointer;
  Q_ASSERT(notes_pointer != nullptr);
  auto number_of_notes = new_notes.size();
  for (qsizetype replace_number = 0; replace_number < number_of_notes;
       replace_number = replace_number + 1) {
    auto &note = (*notes_pointer)[first_note_number + replace_number];
    const auto &new_note = new_notes.at(replace_number);
    for (auto note_column = left_column; note_column <= right_column;
         note_column = static_cast<NoteColumn>(note_column + 1)) {
      qInfo("here");
      qInfo() << "column" << note_column;
      switch (note_column) {
      case note_instrument_column:
        note.instrument_pointer = new_note.instrument_pointer;
        break;
      case note_interval_column:
        qInfo() << QVariant::fromValue(new_note.interval).toString();
        note.interval = new_note.interval;
        break;
      case note_beats_column:
        note.beats = new_note.beats;
        break;
      case note_velocity_ratio_column:
        note.velocity_ratio = new_note.velocity_ratio;
        break;
      case note_tempo_ratio_column:
        note.tempo_ratio = new_note.tempo_ratio;
        break;
      case note_words_column:
        note.words = new_note.words;
        break;
      default:
        Q_ASSERT(false);
        break;
      }
    }
  }
  notes_model.edited_cells(first_note_number, number_of_notes,
                                          left_column, right_column);
}

SetNotesCells::SetNotesCells(NotesModel *notes_model_pointer_input,
                             qsizetype first_note_number_input,
                             NoteColumn left_column_input,
                             NoteColumn right_column_input,
                             const QList<Note> &old_notes_input,
                             const QList<Note> &new_notes_input,
                             QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      notes_model_pointer(notes_model_pointer_input),
      first_note_number(first_note_number_input),
      left_column(left_column_input), right_column(right_column_input),
      old_notes(old_notes_input), new_notes(new_notes_input) {
  Q_ASSERT(notes_model_pointer != nullptr);
}

void SetNotesCells::undo() {
  replace_note_cells(*notes_model_pointer, first_note_number, left_column,
                     right_column, old_notes);
}

void SetNotesCells::redo() {
  replace_note_cells(*notes_model_pointer, first_note_number, left_column,
                     right_column, new_notes);
}
