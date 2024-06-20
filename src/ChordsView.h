#pragma once

#include <qsize.h>         // for QSize
#include <qtmetamacros.h>  // for Q_OBJECT, signals
#include <qtreeview.h>     // for QTreeView

class QWidget;

struct ChordsView : public QTreeView {
  Q_OBJECT
 public:
  explicit ChordsView(QWidget *parent = nullptr);
  [[nodiscard]] auto sizeHintForColumn(int column) const -> int override;
  [[nodiscard]] auto viewportSizeHint() const -> QSize override;
};
