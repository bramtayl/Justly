#include "widgets/WindowBody.hpp"

#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtWidgets/QHBoxLayout>

#include "widgets/ControlsColumn.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

WindowBody::WindowBody()
    : player(*this),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      recovery_timer(*(new QTimer(this))),
      switch_column(*(new SwitchColumn(undo_stack, song))),
      controls_column(*(new ControlsColumn(song, player.synth, undo_stack,
                                           switch_column.switch_table))) {
  auto& row_layout =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row_layout.addWidget(&controls_column, 0, Qt::AlignTop);
  row_layout.addWidget(&switch_column, 0, Qt::AlignTop);
}

WindowBody::~WindowBody() { undo_stack.disconnect(); }

auto get_next_row(const WindowBody& window_body) -> int {
  return get_only_range(window_body.switch_column.switch_table).bottom() + 1;
}
