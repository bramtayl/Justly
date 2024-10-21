#pragma once

#include <QUndoStack>
#include <QtGlobal>

struct ChordsModel;
struct PercussionSet;

struct SetChordPercussionSet : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const qsizetype chord_number;
  const PercussionSet *old_percussion_set_pointer;
  const PercussionSet *new_percussion_set_pointer;

  explicit SetChordPercussionSet(
      ChordsModel *chords_model_pointer_input, qsizetype chord_number_input,
      const PercussionSet *old_percussion_set_pointer_input,
      const PercussionSet *new_percussion_set_pointer_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
