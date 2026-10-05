#include "piano_roll/PianoRollColumnScene.hpp"

#include <QtWidgets/QGraphicsView>

PianoRollColumnScene::PianoRollColumnScene(QWidget& parent_widget)
    : QGraphicsScene(&parent_widget),
      view(*(new QGraphicsView(this, &parent_widget))) {
  // a fixed-width column never scrolls horizontally
  view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view.setFocusPolicy(Qt::NoFocus);
}
