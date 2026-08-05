#pragma once

#include <QtCore/Qt>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QWidget>

#include "other/helpers.hpp"

// a second, fixed-width view pinned to the left edge, showing just the pitch
// axis' ticks/labels (drawn into its own scene by
// PianoRollWidget::rebuild_scene()); its vertical scroll is kept in lockstep
// with the main PianoRollNotesView's (wired up by PianoRollWidget), and both
// scenes place items using the same y = -midi * PIANO_ROLL_PIXELS_PER_SEMITONE
// formula, so the pitch labels stay lined up with their notes no matter how
// far either view is scrolled vertically
struct PianoRollAxisView {
  QGraphicsScene &scene;
  QGraphicsView &view;

  explicit PianoRollAxisView(QWidget &parent_widget)
      : scene(*(new QGraphicsScene(&parent_widget))),
        view(*(new QGraphicsView(&scene, &parent_widget))) {
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFocusPolicy(Qt::NoFocus);
    // each QGraphicsView draws its own sunken frame by default, which shows
    // up as a gray seam between this view and the main view even with the
    // layout's spacing at 0 -- dropping both frames removes that seam while
    // leaving the two views' contents flush against each other
    view.setFrameShape(QFrame::NoFrame);
  }

  ~PianoRollAxisView() = default;

  NO_MOVE_COPY(PianoRollAxisView)
};
