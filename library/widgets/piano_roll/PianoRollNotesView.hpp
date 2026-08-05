#pragma once

#include <QtCore/QChar>
#include <QtCore/QEasingCurve>
#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QMargins>
#include <QtCore/QPoint>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtCore/QtMinMax>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>

#include "other/PianoRoll.hpp"
#include "other/helpers.hpp"
#include "widgets/piano_roll/PianoRollLegendView.hpp"
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

    playhead_item.setPen(QPen(Qt::red));
    playhead_item.setZValue(1);
    playhead_item.hide();
    scene.addItem(&playhead_item);

    static const auto selection_rect_color = QColor(60, 140, 255);
    selection_rect_item.setPen(
        QPen(selection_rect_color, PIANO_ROLL_SELECTION_RECT_PEN_WIDTH));
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

// (re)draws the time axis' ticks and labels, spaced (in ms) so they land
// roughly PIANO_ROLL_TARGET_TICK_PIXEL_SPACING apart on screen at the
// current time_zoom_factor -- called from PianoRollWidget::rebuild_scene()
// for the initial build and from set_time_zoom() whenever the zoom changes,
// since a spacing that looked right before a zoom change would otherwise
// crowd together (zooming in) or spread too far apart (zooming out)
static void redraw_time_axis_ticks(PianoRollNotesView &notes_view) {
  auto &scene = notes_view.scene;
  auto &time_axis_items = notes_view.time_axis_items;

  for (auto *const item_pointer : time_axis_items) {
    scene.removeItem(item_pointer);
    delete item_pointer; // NOLINT(cppcoreguidelines-owning-memory)
  }
  time_axis_items.clear();

  // picks a "nice" (1/2/5 * 10^n) tick interval, in ms, close to the raw
  // interval that would give PIANO_ROLL_TARGET_TICK_PIXEL_SPACING at the
  // current zoom -- so ticks land on round numbers (0.5s, 1s, 2s, ...)
  // rather than an arbitrary value like 437ms
  const auto raw_step_ms = PIANO_ROLL_TARGET_TICK_PIXEL_SPACING /
                           (PIANO_ROLL_PIXELS_PER_MS * notes_view.time_zoom_factor);
  const auto magnitude = std::pow(PIANO_ROLL_NICE_STEP_ROLLOVER,
                                  std::floor(std::log10(raw_step_ms)));
  const auto fraction = raw_step_ms / magnitude;
  auto nice_fraction = PIANO_ROLL_NICE_STEP_ROLLOVER;
  // candidate tick-step multipliers, tried in increasing order against
  // each power-of-ten magnitude -- the classic "nice numbers" progression
  // (1, 2, 5, then roll over to the next magnitude's 1) that keeps chosen
  // tick values round (0.5s, 1s, 2s, 5s, 10s, ...) instead of arbitrary
  static const QList<double> nice_step_multipliers{1.0, 2.0, 5.0};
  for (const auto multiplier : nice_step_multipliers) {
    if (fraction <= multiplier) {
      nice_fraction = multiplier;
      break;
    }
  }
  const auto step_ms = nice_fraction * magnitude;
  const auto time_axis_y = notes_view.time_axis_y;
  const auto time_axis_max_time_ms = notes_view.time_axis_max_time_ms;
  for (auto step_number = 0;
      step_number * step_ms <= time_axis_max_time_ms;
      step_number = step_number + 1) {
    const auto time_ms = step_number * step_ms;
    const auto tick_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
    time_axis_items.push_back(
        scene.addLine(tick_x, time_axis_y, tick_x,
                     time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH));

    // formats a time-axis tick label in whichever unit best suits the
    // current tick spacing (step_ms) -- milliseconds when ticks are
    // sub-second, seconds (with a decimal only when the step itself needs
    // one) once ticks are a second or more apart, and minutes:seconds
    // once they're a minute or more apart -- so labels stay round and
    // readable at every zoom level rather than always being expressed in
    // one fixed unit
    auto &label = get_reference(scene.addSimpleText([&]() -> QString {
      if (step_ms < PIANO_ROLL_MS_PER_SECOND) {
        return QString::number(std::llround(time_ms)) + "ms";
      }
      if (step_ms < PIANO_ROLL_MS_PER_MINUTE) {
        const auto has_sub_second_step =
            std::fmod(step_ms, PIANO_ROLL_MS_PER_SECOND) != 0.0;
        return QString::number(time_ms / PIANO_ROLL_MS_PER_SECOND, 'f',
                               has_sub_second_step ? 1 : 0) +
               "s";
      }
      const auto total_seconds =
          std::llround(time_ms / PIANO_ROLL_MS_PER_SECOND);
      const auto minutes = total_seconds / PIANO_ROLL_SECONDS_PER_MINUTE;
      const auto seconds = total_seconds % PIANO_ROLL_SECONDS_PER_MINUTE;
      return QString("%1:%2").arg(minutes).arg(
          seconds, 2, PIANO_ROLL_LABEL_DECIMAL_BASE, QChar('0'));
    }()));
    // keeps the label's on-screen size constant across zoom levels --
    // without this, since the label lives in the same scene as the notes
    // it gets rendered through this view's time-axis-only x scale (see
    // set_time_zoom()), stretching or squeezing its glyphs horizontally
    // instead of just moving the ticks further apart
    label.setFlag(QGraphicsItem::ItemIgnoresTransformations);
    // centering would push the "0ms"/"0s" label partway into negative x --
    // the pitch axis' column, which this view can no longer scroll into (see
    // the view.setSceneRect() call in PianoRollWidget::rebuild_scene()) --
    // so clamp every label's left edge to the axis line instead
    label.setPos(std::max(tick_x - (label.boundingRect().width() / 2),
                          PIANO_ROLL_AXIS_X),
                time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH +
                    PIANO_ROLL_AXIS_LABEL_GAP);
    time_axis_items.push_back(&label);
  }
}

// sets notes_view's horizontal scale directly (rather than accumulating
// via QGraphicsView::scale()) so repeated zoom_in()/zoom_out() calls can't
// drift and clamping is just one std::clamp on the absolute factor; the
// vertical scale is always left at 1, so the pitch axis (and
// PianoRollAxisView, which is never zoomed) stays visually fixed while
// only the time axis expands/contracts
static void set_notes_view_time_zoom(PianoRollNotesView &notes_view,
                                     const double new_zoom_factor) {
  notes_view.time_zoom_factor = std::clamp(
      new_zoom_factor, PIANO_ROLL_MIN_TIME_ZOOM, PIANO_ROLL_MAX_TIME_ZOOM);
  notes_view.view.setTransform(
      QTransform::fromScale(notes_view.time_zoom_factor, 1.0));
  // the tick spacing (in ms) that keeps ticks ~evenly spaced on screen
  // depends on the zoom factor, so every zoom change needs a fresh set of
  // ticks/labels -- just the time axis, not a full
  // PianoRollWidget::rebuild_scene()
  redraw_time_axis_ticks(notes_view);
}

// moves the playhead line to the time under the given viewport position,
// without recentering the view on it the way position_playhead() does for
// playback -- while the user is actively dragging, the view should hold
// still under the mouse rather than fight the drag by scrolling; returns
// the playhead's new x position (in scene coordinates) so the caller can
// translate it back to a time and sync the switch table's selection
[[nodiscard]] static auto drag_playhead_to(PianoRollNotesView &notes_view,
                                          const QPoint &viewport_pos)
    -> double {
  const auto playhead_x =
      std::max(0.0, notes_view.view.mapToScene(viewport_pos).x());
  const auto &scene_rect = notes_view.scene.sceneRect();
  auto &playhead_item = notes_view.playhead_item;
  playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                        scene_rect.bottom());
  playhead_item.show();
  return playhead_x;
}

// resizes/shows the shaded selection box to span from start_x to end_x (in
// either order), full scene height -- called from
// PianoRollWidget::apply_selection_highlight() to mirror whatever chord/note
// range is currently selected, so it stays put (rather than needing a
// mouse-driven show/hide of its own) whether that range came from a piano
// roll drag, a table click, or playback
static void show_selection_rect(PianoRollNotesView &notes_view,
                                const double start_x, const double end_x) {
  const auto &scene_rect = notes_view.scene.sceneRect();
  const auto left_x = std::min(start_x, end_x);
  const auto right_x = std::max(start_x, end_x);
  auto &selection_rect_item = notes_view.selection_rect_item;
  selection_rect_item.setRect(left_x, scene_rect.top(), right_x - left_x,
                              scene_rect.height());
  selection_rect_item.show();
}

static void hide_selection_rect(PianoRollNotesView &notes_view) {
  notes_view.selection_rect_item.hide();
}

// follow_view lets a caller move the playhead line without recentering the
// view on it -- used when playback has already stopped (see
// PianoRollWidget::apply_selection_highlight()), where forcibly
// recentering would yank the view away from wherever the user had it
// scrolled
static void position_playhead(PianoRollNotesView &notes_view,
                              const double time_ms,
                              const bool follow_view = true) {
  const auto playhead_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
  const auto &scene_rect = notes_view.scene.sceneRect();
  notes_view.playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                                   scene_rect.bottom());
  if (!follow_view) {
    return;
  }

  // eases a just-started playhead onto the view's center instead of
  // snapping there instantly (see PlayheadTransition for the two ways it
  // does that), then keeps it centered horizontally for the rest of
  // playback, without disturbing the user's vertical scroll position --
  // centerOn() can't scroll past the view's own scene rect (set in
  // PianoRollWidget::rebuild_scene()), so near the start/end of the song,
  // where centering the playhead would need to scroll past that edge, it
  // instead settles as close to centered as the edge allows
  auto &view = notes_view.view;
  const auto visible_scene_rect =
      view.mapToScene(view.viewport()->rect()).boundingRect();
  const auto vertical_center = visible_scene_rect.center().y();

  auto &playhead_transition = notes_view.playhead_transition;

  if (playhead_transition == PlayheadTransition::waiting_to_reach_center) {
    // view stays put; playback's own forward motion is what carries the
    // playhead across to the (fixed) center
    if (playhead_x < visible_scene_rect.center().x()) {
      return;
    }
    playhead_transition = PlayheadTransition::none;
  } else if (playhead_transition == PlayheadTransition::catching_up) {
    const auto elapsed_ms =
        static_cast<double>(notes_view.playhead_elapsed_timer.elapsed());
    if (elapsed_ms < PIANO_ROLL_PLAYHEAD_CATCHUP_MS) {
      // eases the view from where it started to exactly where the
      // playhead will be once the catch-up window ends, so the animated
      // scroll and the playhead's real-time motion converge together at
      // the center, rather than the view sliding at some arbitrary rate
      // and hoping it happens to line up
      const auto progress = elapsed_ms / PIANO_ROLL_PLAYHEAD_CATCHUP_MS;
      const auto eased_progress =
          QEasingCurve(QEasingCurve::InOutQuad).valueForProgress(progress);
      // clamped to playhead_end_ms so a clip shorter than the catch-up
      // window still eases toward where playback actually ends, rather
      // than toward a point in time it never reaches
      const auto catchup_end_x =
          std::min(notes_view.playhead_baseline_ms + PIANO_ROLL_PLAYHEAD_CATCHUP_MS,
                   notes_view.playhead_end_ms) *
          PIANO_ROLL_PIXELS_PER_MS;
      const auto center_x =
          notes_view.playhead_catchup_start_center_x +
          (eased_progress *
           (catchup_end_x - notes_view.playhead_catchup_start_center_x));
      view.centerOn(center_x, vertical_center);
      return;
    }
    playhead_transition = PlayheadTransition::none;
  }

  view.centerOn(playhead_x, vertical_center);
}

