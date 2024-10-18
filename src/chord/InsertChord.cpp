#include "chord/InsertChord.hpp"

#include <QList>
#include <QtGlobal>
#include <utility>

#include "chord/ChordsModel.hpp"

InsertChord::InsertChord(ChordsModel *chords_model_pointer_input,
                         qsizetype chord_number_input,
                         Chord new_chord_input,
                         QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      chord_number(chord_number_input), new_chord(std::move(new_chord_input)) {
  Q_ASSERT(chords_model_pointer != nullptr);
}

auto InsertChord::undo() -> void {
  remove_chords(*chords_model_pointer, chord_number, 1);
}

auto InsertChord::redo() -> void {
  auto &chords = chords_model_pointer->chords;

  chords_model_pointer->begin_insert_rows(chord_number, 1);
  chords.insert(chords.begin() + static_cast<int>(chord_number), new_chord);
  chords_model_pointer->end_insert_rows();
}
