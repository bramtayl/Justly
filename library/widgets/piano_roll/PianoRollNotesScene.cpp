#include "widgets/piano_roll/PianoRollNotesScene.hpp"

PianoRollNotesScene::PianoRollNotesScene(QWidget &parent_widget)
    : QGraphicsScene(&parent_widget),
      view(*(new QGraphicsView(this, &parent_widget))),
      playhead_timer(*(new QTimer(&parent_widget))) {
  // keeps the scene point under the cursor fixed on screen while
  // ctrl+wheel zooms the time axis (see PianoRollWidget::zoom_in()/
  // zoom_out()), rather than always zooming around the view's top-left
  // corner
  view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  // each QGraphicsView draws its own sunken frame by default, which shows
  // up as a gray seam between this view and the axis view even with the
  // layout's spacing at 0 -- dropping the frame removes that seam
  view.setFrameShape(QFrame::NoFrame);
  // QGraphicsView's default alignment (Qt::AlignCenter) centers the
  // view.setSceneRect() bounds (set in PianoRollWidget::rebuild_scene(),
  // starting at PIANO_ROLL_AXIS_X) within the viewport whenever that
  // content is narrower than the viewport itself -- e.g. a song with only
  // one short note. That padding isn't clipped, so it reveals whatever
  // the shared scene actually has to the left of PIANO_ROLL_AXIS_X: the
  // pitch axis' own ticks/labels, duplicating PianoRollAxisScene's. Pinning
  // to the top-left keeps any leftover space on the right/bottom instead,
  // where the scene has nothing to leak through.
  view.setAlignment(Qt::AlignLeft | Qt::AlignTop);

  // cosmetic pens keep their stroke width in device pixels regardless of
  // the view's horizontal zoom transform (see set_notes_view_time_zoom),
  // rather than stretching along with it
  auto playhead_pen = QPen(Qt::red);
  playhead_pen.setCosmetic(true);
  playhead_item.setPen(playhead_pen);
  playhead_item.setZValue(1);
  playhead_item.hide();
  addItem(&playhead_item);

  static const auto selection_rect_color = QColor(60, 140, 255);
  auto selection_rect_pen =
      QPen(selection_rect_color, PIANO_ROLL_SELECTION_RECT_PEN_WIDTH);
  selection_rect_pen.setCosmetic(true);
  selection_rect_item.setPen(selection_rect_pen);
  selection_rect_item.setBrush(QBrush(
      QColor(selection_rect_color.red(), selection_rect_color.green(),
            selection_rect_color.blue(),
            PIANO_ROLL_SELECTION_RECT_FILL_ALPHA)));
  selection_rect_item.setZValue(PIANO_ROLL_SELECTION_RECT_Z_VALUE);
  selection_rect_item.hide();
  addItem(&selection_rect_item);
}
