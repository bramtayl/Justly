#pragma once

#include <QtWidgets/QGraphicsScene>

#include "other/helpers.hpp"

// a scene/view for a fixed-width column beside the main PianoRollNotesScene
// view -- the pitch axis on the left and the voice legend on the right --
// pinned there so it stays visible and in the same place no matter how far
// the main view is scrolled horizontally
//
// is-a QGraphicsScene (rather than holding one by reference) so it can be
// heap-allocated and parented to parent_widget directly, the same way
// PianoRollWidget itself is heap-allocated and referenced from MainWindow --
// a QGraphicsScene isn't a widget, so unlike view below it can't pick up
// ownership by being added to a layout; it has to be parented explicitly
struct PianoRollColumnScene : public QGraphicsScene {
  QGraphicsView& view;

  explicit PianoRollColumnScene(QWidget& parent_widget);

  ~PianoRollColumnScene() override = default;

  NO_MOVE_COPY(PianoRollColumnScene)
};
