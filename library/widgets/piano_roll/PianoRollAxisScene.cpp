#include "widgets/piano_roll/PianoRollAxisScene.hpp"

PianoRollAxisScene::PianoRollAxisScene(QWidget &parent_widget)
    : QGraphicsScene(&parent_widget),
      view(*(new QGraphicsView(this, &parent_widget))) {
  view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view.setFocusPolicy(Qt::NoFocus);
  // must match PianoRollNotesScene's alignment (Qt::AlignLeft | AlignTop):
  // when this scene's content is shorter than the viewport (e.g. a new
  // song with one note), QGraphicsView falls back to alignment-based
  // static positioning instead of scrolling, so a mismatched alignment
  // here would decouple this view's ticks from the notes view's content
  // even though both scenes place items at the same scene y-coordinate
  view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
  // each QGraphicsView draws its own sunken frame by default, which shows
  // up as a gray seam between this view and the main view even with the
  // layout's spacing at 0 -- dropping both frames removes that seam while
  // leaving the two views' contents flush against each other
  view.setFrameShape(QFrame::NoFrame);
}
