#pragma once

#include <QtGui/QUndoStack>

#include "other/Song.hpp"
#include "sound/Player.hpp"

struct ControlsColumn;
struct SwitchColumn;

struct WindowBody : public QWidget {
  Song song;
  Player player;
  QUndoStack undo_stack;
  QString current_file;
  QString current_folder;

  // debounced autosave for crash recovery -- restarted on every undo_stack
  // change; wired up by connect_recovery_timer
  QTimer& recovery_timer;

  SwitchColumn& switch_column;
  ControlsColumn& controls_column;

  explicit WindowBody();

  ~WindowBody() override;

  NO_MOVE_COPY(WindowBody)
};

[[nodiscard]] auto get_next_row(const WindowBody& window_body) -> int;
