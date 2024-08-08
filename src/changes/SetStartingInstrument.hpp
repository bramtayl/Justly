#pragma once

#include <QUndoStack>

struct Instrument;
class SongEditor;

class SetStartingInstrument : public QUndoCommand {
  SongEditor *const song_editor_pointer;
  const Instrument *old_value;
  const Instrument *new_value;

public:
  explicit SetStartingInstrument(SongEditor *song_editor_pointer_input,
                                 const Instrument *old_value_input,
                                 const Instrument *new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto
  mergeWith(const QUndoCommand *next_command_pointer) -> bool override;

  void undo() override;
  void redo() override;
};
