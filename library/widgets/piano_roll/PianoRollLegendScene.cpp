#include "widgets/piano_roll/PianoRollLegendScene.hpp"

PianoRollLegendScene::PianoRollLegendScene(QWidget &parent_widget)
    : QGraphicsScene(&parent_widget),
      view(*(new QGraphicsView(this, &parent_widget))) {
  // this view never scrolls horizontally (it's a fixed-width column) and
  // is left free to scroll vertically on its own -- independent of the
  // main view -- so a long voice list stays reachable without the
  // legend's on-screen position ever moving
  view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view.setFocusPolicy(Qt::NoFocus);
}
