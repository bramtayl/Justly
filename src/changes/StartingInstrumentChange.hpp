#pragma once

#include <qundostack.h>  // for QUndoCommand

class SongEditor;  // lines 12-12
struct Instrument;

class StartingInstrumentChange : public QUndoCommand {
  SongEditor* editor_pointer;
  const Instrument* old_value;
  const Instrument* new_value;

 public:
  explicit StartingInstrumentChange(SongEditor* editor_pointer_input,
                                    const Instrument* old_value_input,
                                    const Instrument* new_value_input);
  
  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;

  void undo() override;
  void redo() override;
};
