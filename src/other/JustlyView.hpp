#pragma once

#include <QSize>
#include <QTableView>

class QWidget;

struct JustlyView : public QTableView {
  explicit JustlyView(QWidget *parent = nullptr);
  [[nodiscard]] auto viewportSizeHint() const -> QSize override;
};
