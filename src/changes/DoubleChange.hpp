#pragma once

#include <QUndoStack>  // for QUndoCommand

#include "justly/ChangeId.hpp"  // for ChangeId
#include "justly/SongEditor.hpp"

class DoubleChange : public QUndoCommand {
  SongEditor* const song_editor_pointer;
  const ChangeId change_id;
  const double old_value;
  double new_value;

 public:
  explicit DoubleChange(SongEditor* song_editor_pointer_input,
                        ChangeId change_id_input, double old_value_input,
                        double new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto mergeWith(const QUndoCommand* next_command_pointer)
      -> bool override;

  void undo() override;
  void redo() override;
};
