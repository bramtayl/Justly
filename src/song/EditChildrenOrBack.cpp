#include "song/EditChildrenOrBack.hpp"

#include <QList>
#include <QtGlobal>

#include "chord/Chord.hpp"
#include "pitched_note/PitchedNotesModel.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"
#include "unpitched_note/UnpitchedNotesModel.hpp"

static void edit_children_or_back(EditChildrenOrBack &change,
                                  bool should_edit_children) {
  auto &song_editor = *change.song_editor_pointer;
  auto chord_number = change.chord_number;
  auto is_pitched = change.is_pitched;

  if (should_edit_children) {
    if (is_pitched) {
      auto &pitched_notes_model = *song_editor.pitched_notes_model_pointer;

      pitched_notes_model.parent_chord_number = chord_number;
      edit_notes(song_editor, pitched_notes_model,
                 song_editor.song.chords[chord_number].pitched_notes,
                 chord_number, true);
    } else {
      edit_notes(song_editor, *song_editor.unpitched_notes_model_pointer,
                 song_editor.song.chords[chord_number].unpitched_notes,
                 chord_number, false);
    }
  } else {
    if (is_pitched) {
      auto &pitched_notes_model = *song_editor.pitched_notes_model_pointer;

      back_to_chords_directly(song_editor);
      pitched_notes_model.set_rows_pointer(nullptr);
      pitched_notes_model.parent_chord_number = -1;
    } else {
      back_to_chords_directly(song_editor);
      song_editor.unpitched_notes_model_pointer->set_rows_pointer(nullptr);
    }
  }
};

EditChildrenOrBack::EditChildrenOrBack(SongEditor &song_editor,
                                       int chord_number_input,
                                       bool is_notes_input,
                                       bool backwards_input,
                                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input), song_editor_pointer(&song_editor),
      chord_number(chord_number_input), is_pitched(is_notes_input),
      backwards(backwards_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
}

void EditChildrenOrBack::undo() { edit_children_or_back(*this, backwards); }

void EditChildrenOrBack::redo() { edit_children_or_back(*this, !backwards); }
