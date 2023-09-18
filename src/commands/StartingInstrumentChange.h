#pragma once

#include <qstring.h>     // for QString
#include <qundostack.h>  // for QUndoCommand

class Editor;  // lines 12-12

class StartingInstrumentChange : public QUndoCommand {
 public:
  Editor &editor;
  const QString old_starting_instrument;
  QString new_starting_instrument;
  bool first_time = true;
  explicit StartingInstrumentChange(Editor &editor,
                                    QString new_starting_instrument_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};
