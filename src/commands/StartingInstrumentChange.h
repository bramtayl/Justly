#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <gsl/pointers>  // for not_null

class Editor;  // lines 12-12
class Instrument;

class StartingInstrumentChange : public QUndoCommand {
 private:
  gsl::not_null<Editor*> editor_pointer;
  gsl::not_null<const Instrument*> old_starting_instrument_pointer;
  gsl::not_null<const Instrument*> new_starting_instrument_pointer;
  bool first_time = true;

 public:
  explicit StartingInstrumentChange(
      gsl::not_null<Editor*> editor_pointer_input,
      const Instrument& new_starting_instrument_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;
  [[nodiscard]] auto get_old_starting_instrument() const -> const Instrument&;
  [[nodiscard]] auto get_new_starting_instrument() const -> const Instrument&;

  void set_old_starting_instrument(const Instrument& new_instrument);

  void set_new_starting_instrument(const Instrument& new_instrument);
};
