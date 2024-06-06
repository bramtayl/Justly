#pragma once

#include <qsize.h>         // for QSize
#include <qtmetamacros.h>  // for Q_OBJECT, signals
#include <qtreeview.h>     // for QTreeView

#include "justly/macros.h"

class QWidget;

struct ChordsView : public QTreeView {
  Q_OBJECT
 public:
  explicit ChordsView(QWidget *parent = nullptr);
  NO_MOVE_COPY(ChordsView);
  [[nodiscard]] auto viewportSizeHint() const -> QSize override;
  [[nodiscard]] auto sizeHintForColumn(int column) const -> int override;
};
