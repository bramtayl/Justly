#pragma once

#include <QUndoStack>
#include <QtGlobal>

struct PercussionsModel;
struct PercussionSet;

struct SetPercussionSet : public QUndoCommand {
  PercussionsModel *const percussions_model_pointer;
  const qsizetype percussion_number;
  const PercussionSet *const old_percussion_set_pointer;
  const PercussionSet *const new_percussion_set_pointer;

  explicit SetPercussionSet(
      PercussionsModel *percussions_model_pointer_input,
      qsizetype percussion_number_input,
      const PercussionSet *old_percussion_set_pointer_input,
      const PercussionSet *new_percussion_set_pointer_input,
      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
