#pragma once

#include <qundostack.h>  // for QUndoCommand

#include "metatypes/Instrument.h"  // for Instrument

class Editor;  // lines 12-12

class StartingInstrumentChange : public QUndoCommand {
 public:
  Editor* editor_pointer;
  Instrument old_starting_instrument;
  Instrument new_starting_instrument;
  bool first_time = true;
  explicit StartingInstrumentChange(Editor* editor_pointer_input,
                                    Instrument new_starting_instrument_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};
