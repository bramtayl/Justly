#include "song/EditChildrenOrBack.hpp"

#include <QAction>
#include <QLabel>
#include <QList>
#include <QString>
#include <QTextStream>
#include <QtGlobal>

#include "row/RowsModel.hpp"
#include "row/chord/Chord.hpp"
#include "row/chord/ChordsModel.hpp"
#include "row/pitched_note/PitchedNotesModel.hpp"
#include "song/ModelType.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"

static void edit_children_or_back(EditChildrenOrBack &change,
                                  bool should_edit_children) {
  auto &song_editor = change.song_editor;
  auto chord_number = change.chord_number;
  auto is_pitched = change.is_pitched;

  song_editor.back_to_chords_action.setEnabled(should_edit_children);
  song_editor.open_action.setEnabled(!should_edit_children);

  if (should_edit_children) {
    QString label_text;
    QTextStream stream(&label_text);
    stream << SongEditor::tr("Editing ")
           << SongEditor::tr(is_pitched ? "pitched" : "unpitched")
           << SongEditor::tr(" notes for chord ") << chord_number + 1;
    song_editor.editing_chord_text.setText(label_text);
    Q_ASSERT(song_editor.current_model_type == chords_type);
    song_editor.current_model_type =
        is_pitched ? pitched_notes_type : unpitched_notes_type;
    song_editor.current_chord_number = chord_number;

    auto &chord = song_editor.song.chords[chord_number];
    if (is_pitched) {
      auto &pitched_notes_model = song_editor.pitched_notes_model;
      pitched_notes_model.parent_chord_number = chord_number;
      pitched_notes_model.set_rows_pointer(&chord.pitched_notes);
      set_model(song_editor, pitched_notes_model);
    } else {
      auto &unpitched_notes_model = song_editor.unpitched_notes_model;
      unpitched_notes_model.set_rows_pointer(&chord.unpitched_notes);
      set_model(song_editor, unpitched_notes_model);
    }
  } else {
    set_model(song_editor, song_editor.chords_model);

    song_editor.editing_chord_text.setText("Editing chords");
    song_editor.current_model_type = chords_type;
    song_editor.current_chord_number = -1;

    if (is_pitched) {
      auto &pitched_notes_model = song_editor.pitched_notes_model;
      pitched_notes_model.set_rows_pointer();
      pitched_notes_model.parent_chord_number = -1;
    } else {
      song_editor.unpitched_notes_model.set_rows_pointer();
    }
  }
};

EditChildrenOrBack::EditChildrenOrBack(SongEditor &song_editor_input,
                                       int chord_number_input,
                                       bool is_notes_input,
                                       bool backwards_input)
    : song_editor(song_editor_input), chord_number(chord_number_input),
      is_pitched(is_notes_input), backwards(backwards_input) {}

void EditChildrenOrBack::undo() { edit_children_or_back(*this, backwards); }

void EditChildrenOrBack::redo() { edit_children_or_back(*this, !backwards); }
