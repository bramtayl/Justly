#pragma once

#include <QUndoStack>

class SongEditor;

class SetStartingTempo : public QUndoCommand {
private:
  SongEditor *const song_editor_pointer;
  const double old_value;
  double new_value;

public:
  explicit SetStartingTempo(SongEditor *song_editor_pointer_input,
                            double old_value_input, double new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto
  mergeWith(const QUndoCommand *next_command_pointer) -> bool override;

  void undo() override;
  void redo() override;
};
