#include "commands/SetCells.hpp"

#include <QString>
#include <QtGlobal>
#include <cstddef>
#include <variant>

#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "other/ChordsModel.hpp"
#include "other/templates.hpp"
#include "rational/Rational.hpp"

static void replace_cell_ranges(ChordsModel *chords_model_pointer,
                                const std::vector<RowRange> &row_ranges,
                                const std::vector<NoteChord> &note_chords,
                                NoteChordColumn left_column,
                                NoteChordColumn right_column) {
  Q_ASSERT(chords_model_pointer != nullptr);
  auto &chords = chords_model_pointer->chords;
  size_t note_chord_number = 0;
  for (const auto &row_range : row_ranges) {
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
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
      chords_model_pointer->edited_chords_cells(
          first_child_number, number_of_children, left_column, right_column);
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
      chords_model_pointer->edited_notes_cells(chord_number, first_child_number,
                                               number_of_children, left_column,
                                               right_column);
    }
    note_chord_number = note_chord_number + number_of_children;
  }
}

SetCells::SetCells(ChordsModel *chords_model_pointer_input,
                   const std::vector<RowRange> &row_ranges_input,
                   const std::vector<NoteChord> &old_note_chords_input,
                   const std::vector<NoteChord> &new_note_chords_input,
                   NoteChordColumn left_column_input,
                   NoteChordColumn right_column_input,
                   QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      row_ranges(row_ranges_input), old_note_chords(old_note_chords_input),
      new_note_chords(new_note_chords_input), left_column(left_column_input),
      right_column(right_column_input) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

void SetCells::undo() {
  replace_cell_ranges(chords_model_pointer, row_ranges, old_note_chords,
                      left_column, right_column);
}

void SetCells::redo() {
  replace_cell_ranges(chords_model_pointer, row_ranges, new_note_chords,
                      left_column, right_column);
}
