#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QWidget>

#include "other/PianoRollNoteEvent.hpp"
#include "other/helpers.hpp"
#include "widgets/piano_roll/PlayheadTransition.hpp"

static const auto PIANO_ROLL_PIXELS_PER_MS = 0.1;
static const auto PIANO_ROLL_AXIS_TICK_LENGTH = 5.0;
static const auto PIANO_ROLL_AXIS_X = 0.0;
static const auto PIANO_ROLL_DEFAULT_AXIS_Y = 0.0;
// ticks are re-spaced on every zoom change (see
// PianoRollNotesView::redraw_time_axis_ticks()) to keep roughly this many screen
// pixels between them, rather than a fixed time interval -- otherwise
// zooming in would crowd ticks together and zooming out would spread them so
// far apart that most of the timeline carries no labels at all
static const auto PIANO_ROLL_TARGET_TICK_PIXEL_SPACING = 80.0;
static const auto PIANO_ROLL_MS_PER_SECOND = 1000.0;
static const auto PIANO_ROLL_MS_PER_MINUTE = 60000.0;
static const auto PIANO_ROLL_SECONDS_PER_MINUTE = 60LL;
static const auto PIANO_ROLL_LABEL_DECIMAL_BASE = 10;
static const auto PIANO_ROLL_NICE_STEP_ROLLOVER = 10.0;
static const auto PIANO_ROLL_MIN_TIME_ZOOM = 0.25;
static const auto PIANO_ROLL_MAX_TIME_ZOOM = 8.0;
// how long the view takes to catch up to the playhead when playback starts
// with the playhead already right of center (see
// PianoRollNotesView::position_playhead()) -- long enough to read as a
// deliberate scroll, short enough not to lag behind what's actually playing
static const auto PIANO_ROLL_PLAYHEAD_CATCHUP_MS = 400.0;
static const auto PIANO_ROLL_SELECTION_RECT_PEN_WIDTH = 1.0;
static const auto PIANO_ROLL_SELECTION_RECT_FILL_ALPHA = 60;
// behind the note bars and axis (default z 0), not in front of them --
// besides reading better as a background wash rather than a mask over the
// notes, sitting in front would make QGraphicsScene::itemAt() (used by
// PianoRollWidget's double-click handler) hit the box instead of whatever
// note is under the cursor, since the box now stays visible for as long as
// the selection does rather than only during a drag
static const auto PIANO_ROLL_SELECTION_RECT_Z_VALUE = -1.0;

// the main scrollable graphics view: the note bars, the pitch/time axes,
// and the playhead cursor + its playback animation all live here
struct PianoRollNotesView {
  QGraphicsScene &scene;
  QGraphicsView &view;
  QGraphicsLineItem &playhead_item = *(new QGraphicsLineItem);
  // the shaded box drawn behind the notes over the current selection's
  // timeline extent -- purely visual feedback for whatever the switch
  // table's own selection already is (see
  // PianoRollWidget::apply_selection_highlight()), so it never itself drives
  // selection and stays visible for as long as that selection does,
  // including after a drag's mouse release
  QGraphicsRectItem &selection_rect_item = *(new QGraphicsRectItem);

  QTimer &playhead_timer;
  QElapsedTimer playhead_elapsed_timer;
  double playhead_baseline_ms = 0;
  double playhead_end_ms = 0;
  bool playhead_active = false;

  PlayheadTransition playhead_transition = PlayheadTransition::none;
  // the view's horizontal center, in scene coordinates, at the moment a
  // catching_up transition began -- the fixed starting point the eased
  // scroll interpolates away from
  double playhead_catchup_start_center_x = 0.0;
  // true while the user is dragging the playhead with the mouse (between a
  // left-button press and release on this view); guards PianoRollWidget's
  // eventFilter MouseMove handling so ordinary mouse-move events (that
  // aren't part of a drag) don't also move the cursor
  bool playhead_dragging = false;

  // scales only this view's x axis (time), never its y axis (pitch) -- so
  // the pitch axis stays visually fixed (and stays in lockstep with
  // PianoRollAxisView, which is never zoomed) while the time axis
  // expands/contracts
  double time_zoom_factor = 1.0;

  // the inputs redraw_time_axis_ticks() needs to redraw just the time
  // axis' ticks and labels whenever the zoom changes, without re-running
  // the full PianoRollWidget::rebuild_scene()
  double time_axis_max_time_ms = 0.0;
  double time_axis_y = PIANO_ROLL_DEFAULT_AXIS_Y;
  // the absolute song time (ms) that maps to this view's x == PIANO_ROLL_AXIS_X --
  // 0 normally, but in notes mode (PianoRollWidget::rebuild_scene() scoped
  // to one chord's notes) it's that chord's own start time, so the axis
  // only spans the window during which the chord's notes actually play
  // instead of dragging along every silent millisecond since the song
  // began; see to_scene_x() in PianoRollWidget.hpp
  double time_axis_baseline_ms = 0.0;
  // the tick lines + labels currently on screen, so redraw_time_axis_ticks()
  // can remove exactly those before drawing a fresh set at the new spacing,
  // leaving the rest of the scene (notes, pitch axis, playhead) untouched
  QList<QGraphicsItem *> time_axis_items;

  // rebuilt every PianoRollWidget::rebuild_scene() call; each drawn note
  // rect stores its index into this list (via QGraphicsItem::setData) so a
  // click on the rect can be traced back to the chord/note it represents
  QList<PianoRollNoteEvent> events;
  // parallel to events -- the actual drawn item for each event, so a table
  // selection can be traced forward to the bar(s) it should highlight
  QList<QGraphicsRectItem *> note_items;
  // parallel to events -- lets PianoRollWidget::select_chord_at_playhead()
  // find which chord a cursor time falls in without rescanning the whole
  // song on every playback tick
  QList<double> chord_start_times;

  explicit PianoRollNotesView(QWidget &parent_widget)
      : scene(*(new QGraphicsScene(&parent_widget))),
        view(*(new QGraphicsView(&scene, &parent_widget))),
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
    // pitch axis' own ticks/labels, duplicating PianoRollAxisView's. Pinning
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
    scene.addItem(&playhead_item);

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
    scene.addItem(&selection_rect_item);
  }

  ~PianoRollNotesView() = default;

  NO_MOVE_COPY(PianoRollNotesView)
};
