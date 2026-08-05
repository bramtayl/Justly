#pragma once

#include <QtCore/Qt>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QWidget>

#include "other/helpers.hpp"

static const auto PIANO_ROLL_AXIS_LABEL_GAP = 4.0;
static const auto PIANO_ROLL_LEGEND_SWATCH_SIZE = 10.0;

// a separate scene/view for the voice legend, laid out in its own
// fixed-width column to the right of the main view -- pinned there just
// like PianoRollAxisView is pinned to the left, so the legend stays visible
// and in the same place no matter how far the main view is scrolled, rather
// than being drawn into the scrollable main scene where it used to
// disappear off-screen
struct PianoRollLegendView {
  QGraphicsScene &scene;
  QGraphicsView &view;

  explicit PianoRollLegendView(QWidget &parent_widget)
      : scene(*(new QGraphicsScene(&parent_widget))),
        view(*(new QGraphicsView(&scene, &parent_widget))) {
    // this view never scrolls horizontally (it's a fixed-width column) and
    // is left free to scroll vertically on its own -- independent of the
    // main view -- so a long voice list stays reachable without the
    // legend's on-screen position ever moving
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFocusPolicy(Qt::NoFocus);
  }

  ~PianoRollLegendView() = default;

  NO_MOVE_COPY(PianoRollLegendView)
};
