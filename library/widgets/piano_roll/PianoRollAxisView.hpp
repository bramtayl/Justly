#pragma once

#include <QtCore/Qt>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QWidget>

#include "other/helpers.hpp"

// a second, fixed-width view onto the main PianoRollNotesView's scene, locked so
// it only ever shows the pitch axis' column (x <= PIANO_ROLL_AXIS_X); its
// vertical scroll is kept in lockstep with the main view's (wired up by
// PianoRollWidget), so the pitch labels stay pinned to the left edge -- and
// lined up with their notes -- no matter how far the main view is scrolled
// horizontally
struct PianoRollAxisView {
  QGraphicsView &view;

  PianoRollAxisView(QWidget &parent_widget, QGraphicsScene &scene)
      : view(*(new QGraphicsView(&scene, &parent_widget))) {
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFocusPolicy(Qt::NoFocus);
    // each QGraphicsView draws its own sunken frame by default, which shows
    // up as a gray seam between this view and the main view even with the
    // layout's spacing at 0 -- dropping both frames removes that seam while
    // leaving the two views' contents flush against each other
    view.setFrameShape(QFrame::NoFrame);
  }

  NO_MOVE_COPY(PianoRollAxisView)
};
