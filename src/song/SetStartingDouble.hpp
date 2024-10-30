#pragma once

#include "song/ControlId.hpp"
#include <QUndoStack>

struct SongEditor;

struct SetStartingDouble : public QUndoCommand {
  SongEditor& song_editor;
  const ControlId command_id;
  const double old_value;
  double new_value;

  explicit SetStartingDouble(SongEditor& song_editor_input,
                             ControlId command_id_input,
                             double new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto
  mergeWith(const QUndoCommand *next_command_pointer) -> bool override;

  void undo() override;
  void redo() override;
};
