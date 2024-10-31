#include "song/EditChildrenOrBack.hpp"

#include <QAction>
#include <QLabel>
#include <QList>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "pitched_note/PitchedNotesModel.hpp"
#include "rows/RowsModel.hpp"
#include "song/ModelType.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"

static void edit_children_or_back(EditChildrenOrBack &change,
                                  bool should_edit_children) {
  auto &song_editor = change.song_editor;
  auto chord_number = change.chord_number;
  auto is_pitched = change.is_pitched;

  if (should_edit_children) {
    auto& chord = song_editor.song.chords[chord_number];
    if (is_pitched) {
      auto &pitched_notes_model = song_editor.pitched_notes_model;

      pitched_notes_model.parent_chord_number = chord_number;
      edit_notes(song_editor, pitched_notes_model,
                 chord.pitched_notes,
                 chord_number, true);
    } else {
      edit_notes(song_editor, song_editor.unpitched_notes_model,
                 chord.unpitched_notes,
                 chord_number, false);
    }
  } else {
    song_editor.editing_chord_text.setText("Editing chords");
    set_model(song_editor, song_editor.chords_model);
    song_editor.current_model_type = chords_type;
    song_editor.current_chord_number = -1;
    song_editor.back_to_chords_action.setEnabled(false);
    song_editor.open_action.setEnabled(true);
    is_chords_now(song_editor, true);
    if (is_pitched) {
      auto &pitched_notes_model = song_editor.pitched_notes_model;
      remove_rows_pointer(pitched_notes_model);
      pitched_notes_model.parent_chord_number = -1;
    } else {
      remove_rows_pointer(song_editor.unpitched_notes_model);
    }
  }
};

EditChildrenOrBack::EditChildrenOrBack(SongEditor &song_editor_input,
                                       int chord_number_input,
                                       bool is_notes_input,
                                       bool backwards_input)
    : song_editor(song_editor_input),
      chord_number(chord_number_input), is_pitched(is_notes_input),
      backwards(backwards_input) {
}

void EditChildrenOrBack::undo() { edit_children_or_back(*this, backwards); }

void EditChildrenOrBack::redo() { edit_children_or_back(*this, !backwards); }
