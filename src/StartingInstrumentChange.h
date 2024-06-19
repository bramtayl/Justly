#pragma once

#include <qundostack.h>  // for QUndoCommand

#include "justly/global.h"

class SongEditor;  // lines 12-12
struct Instrument;

class JUSTLY_EXPORT StartingInstrumentChange : public QUndoCommand {
  SongEditor* editor_pointer;
  const Instrument* old_value;
  const Instrument* new_value;

 public:
  explicit StartingInstrumentChange(SongEditor*, const Instrument*,
                                    const Instrument*);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;
};
