#pragma once

#include <QUndoStack>

class SongEditor;

class GainChange : public QUndoCommand {
  SongEditor *const song_editor_pointer;
  const int old_value;
  int new_value;

public:
  explicit GainChange(SongEditor *song_editor_pointer_input,
                        int old_value_input,
                        int new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto
  mergeWith(const QUndoCommand *next_command_pointer) -> bool override;

  void undo() override;
  void redo() override;
};
