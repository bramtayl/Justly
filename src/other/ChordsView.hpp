#pragma once

#include <QObject>
#include <QSize>
#include <QTreeView>

struct ChordsModel;
class QUndoStack;
class QWidget;

struct ChordsView : public QTreeView {
  Q_OBJECT
public:
  ChordsModel *const chords_model_pointer;

  explicit ChordsView(QUndoStack *undo_stack_pointer_input,
                      QWidget *parent = nullptr);
  [[nodiscard]] auto viewportSizeHint() const -> QSize override;
};
