#pragma once

#include <qstring.h>     // for QString
#include <qundostack.h>  // for QUndoCommand

#include "metatypes/Instrument.h"

class Editor;  // lines 12-12

class StartingInstrumentChange : public QUndoCommand {
 public:
  Editor &editor;
  const Instrument old_starting_instrument;
  Instrument new_starting_instrument;
  bool first_time = true;
  explicit StartingInstrumentChange(Editor &editor,
                                    Instrument new_starting_instrument_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};
