#pragma once

#include <qundostack.h>  // for QUndoCommand

class InstrumentEditor;
struct Instrument;

class InstrumentChange : public QUndoCommand {
  InstrumentEditor* const starting_instrument_editor_pointer;
  const Instrument* old_value;
  const Instrument* new_value;

 public:
  explicit InstrumentChange(
      InstrumentEditor* starting_instrument_editor_pointer_input,
      const Instrument* old_value_input, const Instrument* new_value_input);

  [[nodiscard]] auto id() const -> int override;
  [[nodiscard]] auto mergeWith(const QUndoCommand* next_command_pointer)
      -> bool override;

  void undo() override;
  void redo() override;

  void set_starting_instrument_directly(const Instrument* new_value);
};
