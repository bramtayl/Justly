#include "piano_roll/PianoRollNotesScene.hpp"

#include <QtCore/QEasingCurve>
#include <QtCore/QTimer>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsView>

PianoRollNotesScene::PianoRollNotesScene(QWidget& parent_widget)
    : QGraphicsScene(&parent_widget),
      view(*(new QGraphicsView(this, &parent_widget))),
      playhead_item(*(new QGraphicsLineItem)),
      selection_rect_item(*(new QGraphicsRectItem)),
      playhead_timer(*(new QTimer(&parent_widget))) {
  static const auto PIANO_ROLL_SELECTION_RECT_PEN_WIDTH = 1.0;
  static const auto PIANO_ROLL_SELECTION_RECT_FILL_ALPHA = 60;
  // behind the note bars and axis (default z 0), not in front of them, so
  // it reads as a background wash rather than a mask over the notes, since
  // the box stays visible for as long as the selection does rather than
  // only during a drag
  static const auto PIANO_ROLL_SELECTION_RECT_Z_VALUE = -1.0;

  // keeps the scene point under the cursor fixed on screen while
  // ctrl+wheel zooms the time axis (see zoom_in_piano_roll()/
  // zoom_out_piano_roll()), rather than always zooming around the view's
  // top-left corner
  view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  // each QGraphicsView draws its own sunken frame by default, which shows
  // up as a gray seam between this view and the axis view even with the
  // layout's spacing at 0 -- dropping the frame removes that seam
  view.setFrameShape(QFrame::NoFrame);
  // QGraphicsView's default alignment (Qt::AlignCenter) centers the
  // view.setSceneRect() bounds (set in rebuild_piano_roll_scene(),
  // starting at PIANO_ROLL_AXIS_X) within the viewport whenever that
  // content is narrower than the viewport itself -- e.g. a song with only
  // one short note. That padding isn't clipped, so it reveals whatever
  // the shared scene actually has to the left of PIANO_ROLL_AXIS_X: the
  // pitch axis' own ticks/labels, duplicating axis_scene's. Pinning
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
  selection_rect_item.setBrush(QBrush(QColor(
      selection_rect_color.red(), selection_rect_color.green(),
      selection_rect_color.blue(), PIANO_ROLL_SELECTION_RECT_FILL_ALPHA)));
  selection_rect_item.setZValue(PIANO_ROLL_SELECTION_RECT_Z_VALUE);
  selection_rect_item.hide();
  addItem(&selection_rect_item);
}

auto to_scene_x(const PianoRollNotesScene& notes_scene, const double time_ms)
    -> double {
  return (time_ms - notes_scene.time_axis_baseline_ms) *
         PIANO_ROLL_PIXELS_PER_MS;
}

namespace {

// (re)draws the time axis' ticks and labels, spaced (in ms) so they land
// roughly PIANO_ROLL_TARGET_TICK_PIXEL_SPACING apart on screen at the
// current time_zoom_factor -- called from rebuild_piano_roll_scene() for the
// initial build and from set_notes_view_time_zoom() whenever the zoom changes,
// since a spacing that looked right before a zoom change would otherwise crowd
// together (zooming in) or spread too far apart (zooming out)
void redraw_time_axis_ticks(PianoRollNotesScene& notes_scene) {
  // ticks are re-spaced on every zoom change to keep roughly this many
  // screen pixels between them, rather than a fixed time interval --
  // otherwise zooming in would crowd ticks together and zooming out would
  // spread them so far apart that most of the timeline carries no labels at
  // all
  static const auto PIANO_ROLL_TARGET_TICK_PIXEL_SPACING = 80.0;
  static const auto PIANO_ROLL_MS_PER_SECOND = 1000.0;
  static const auto PIANO_ROLL_NICE_STEP_ROLLOVER = 10.0;

  auto& scene = notes_scene;
  auto& time_axis_items = notes_scene.time_axis_items;

  for (auto* const item_pointer : time_axis_items) {
    scene.removeItem(item_pointer);
    delete item_pointer;  // NOLINT(cppcoreguidelines-owning-memory)
  }
  time_axis_items.clear();

  // picks a "nice" (1/2/5 * 10^n) tick interval, in ms, close to the raw
  // interval that would give PIANO_ROLL_TARGET_TICK_PIXEL_SPACING at the
  // current zoom -- so ticks land on round numbers (0.5s, 1s, 2s, ...)
  // rather than an arbitrary value like 437ms
  const auto raw_step_ms =
      PIANO_ROLL_TARGET_TICK_PIXEL_SPACING /
      (PIANO_ROLL_PIXELS_PER_MS * notes_scene.time_zoom_factor);
  const auto magnitude = std::pow(PIANO_ROLL_NICE_STEP_ROLLOVER,
                                  std::floor(std::log10(raw_step_ms)));
  const auto fraction = raw_step_ms / magnitude;
  auto nice_fraction = PIANO_ROLL_NICE_STEP_ROLLOVER;
  // candidate tick-step multipliers, tried in increasing order against
  // each power-of-ten magnitude -- the classic "nice numbers" progression
  // (1, 2, 5, then roll over to the next magnitude's 1) that keeps chosen
  // tick values round (0.5s, 1s, 2s, 5s, 10s, ...) instead of arbitrary
  static const QList<double> nice_step_multipliers{1.0, 2.0, 5.0};
  // cosmetic so the tick stroke width stays in device pixels rather than
  // stretching with the view's horizontal zoom transform (see
  // set_notes_view_time_zoom), matching the playhead/selection-box pens
  static const auto tick_pen = []() -> QPen {
    auto pen = QPen();
    pen.setCosmetic(true);
    return pen;
  }();
  const auto nice_multiplier_iterator = std::ranges::find_if(
      nice_step_multipliers, [fraction](const double multiplier) -> auto {
        return fraction <= multiplier;
      });
  if (nice_multiplier_iterator != nice_step_multipliers.end()) {
    nice_fraction = *nice_multiplier_iterator;
  }
  const auto step_ms = nice_fraction * magnitude;
  const auto time_axis_y = notes_scene.time_axis_y;
  const auto time_axis_max_time_ms = notes_scene.time_axis_max_time_ms;
  for (auto step_number = 0; step_number * step_ms <= time_axis_max_time_ms;
       ++step_number) {
    const auto time_ms = step_number * step_ms;
    const auto tick_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
    time_axis_items.push_back(
        scene.addLine(tick_x, time_axis_y, tick_x,
                      time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH, tick_pen));

    // formats a time-axis tick label in whichever unit best suits the
    // current tick spacing (step_ms) -- milliseconds when ticks are
    // sub-second, and whole seconds once ticks are a second or more apart
    // (a nice step that big is always a whole number of seconds) -- so
    // labels stay round and readable at every zoom level rather than always
    // being expressed in one fixed unit. PIANO_ROLL_MIN_TIME_ZOOM caps the
    // step at a few seconds, so ticks never get far enough apart to need
    // minutes
    auto& label = get_reference(scene.addSimpleText([&]() -> QString {
      if (step_ms < PIANO_ROLL_MS_PER_SECOND) {
        return QString::number(std::llround(time_ms)) + "ms";
      }
      return QString::number(std::llround(time_ms / PIANO_ROLL_MS_PER_SECOND)) +
             "s";
    }()));
    // keeps the label's on-screen size constant across zoom levels --
    // without this, since the label lives in the same scene as the notes
    // it gets rendered through this view's time-axis-only x scale (see
    // set_notes_view_time_zoom()), stretching or squeezing its glyphs
    // horizontally instead of just moving the ticks further apart
    label.setFlag(QGraphicsItem::ItemIgnoresTransformations);
    // centering would push the "0ms"/"0s" label partway into negative x --
    // the pitch axis' column, which this view can no longer scroll into (see
    // the view.setSceneRect() call in rebuild_piano_roll_scene()) --
    // so clamp every label's left edge to the axis line instead. the label's
    // boundingRect() is in device pixels (it ignores the view's transform --
    // see the ItemIgnoresTransformations flag above), while tick_x is in
    // scene units that the view later scales by time_zoom_factor, so the
    // half-width has to be converted into scene units before it's compared
    // against/subtracted from tick_x -- otherwise at high zoom a fixed
    // device-pixel width looks huge next to the shrunken scene-unit tick_x,
    // clamping far more than just the first tick and stacking several
    // labels on top of each other at the axis line
    label.setPos(
        std::max(tick_x - (label.boundingRect().width() / 2) /
                              notes_scene.time_zoom_factor,
                 PIANO_ROLL_AXIS_X),
        time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH + PIANO_ROLL_AXIS_LABEL_GAP);
    time_axis_items.push_back(&label);
  }
}

}  // namespace

void set_notes_view_time_zoom(PianoRollNotesScene& notes_scene,
                              const double new_zoom_factor) {
  notes_scene.time_zoom_factor = std::clamp(
      new_zoom_factor, PIANO_ROLL_MIN_TIME_ZOOM, PIANO_ROLL_MAX_TIME_ZOOM);
  notes_scene.view.setTransform(
      QTransform::fromScale(notes_scene.time_zoom_factor, 1.0));
  // the tick spacing (in ms) that keeps ticks ~evenly spaced on screen
  // depends on the zoom factor, so every zoom change needs a fresh set of
  // ticks/labels -- just the time axis, not a full
  // rebuild_piano_roll_scene()
  redraw_time_axis_ticks(notes_scene);
}

// draws the horizontal axis line, placed between the pitched notes above and
// the unpitched lanes below; both axes sit at x/y == PIANO_ROLL_AXIS_X, so
// the horizontal axis and the t=0 time tick meet at one corner. The line's
// endpoints are in scene coordinates and don't depend on zoom, so unlike the
// ticks/labels (redrawn by redraw_time_axis_ticks()) it's only ever drawn
// once per rebuild
void draw_time_axis(PianoRollNotesScene& notes_scene, const double axis_y,
                    const double max_time_ms) {
  notes_scene.time_axis_max_time_ms = max_time_ms;
  notes_scene.time_axis_y = axis_y;
  notes_scene.addLine(PIANO_ROLL_AXIS_X, axis_y,
                      max_time_ms * PIANO_ROLL_PIXELS_PER_MS, axis_y);
  redraw_time_axis_ticks(notes_scene);
}

void drag_playhead_to(PianoRollNotesScene& notes_scene,
                      const QPoint& viewport_pos) {
  const auto playhead_x =
      std::max(0.0, notes_scene.view.mapToScene(viewport_pos).x());
  const auto& scene_rect = notes_scene.sceneRect();
  auto& playhead_item = notes_scene.playhead_item;
  playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                        scene_rect.bottom());
  playhead_item.show();
}

void show_selection_rect(PianoRollNotesScene& notes_scene, const double start_x,
                         const double end_x) {
  Q_ASSERT(start_x <= end_x);
  const auto& scene_rect = notes_scene.sceneRect();
  auto& selection_rect_item = notes_scene.selection_rect_item;
  selection_rect_item.setRect(start_x, scene_rect.top(), end_x - start_x,
                              scene_rect.height());
  selection_rect_item.show();
}

void hide_selection_rect(PianoRollNotesScene& notes_scene) {
  notes_scene.selection_rect_item.hide();
}

void position_playhead(PianoRollNotesScene& notes_scene, const double time_ms,
                       const bool follow_view) {
  // how long the view takes to catch up to the playhead when playback
  // starts with the playhead already right of center -- long enough to read
  // as a deliberate scroll, short enough not to lag behind what's actually
  // playing
  static const auto PIANO_ROLL_PLAYHEAD_CATCHUP_MS = 400.0;

  const auto playhead_x = to_scene_x(notes_scene, time_ms);
  const auto& scene_rect = notes_scene.sceneRect();
  notes_scene.playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                                    scene_rect.bottom());
  if (!follow_view) {
    return;
  }

  // eases a just-started playhead onto the view's center instead of
  // snapping there instantly (see PlayheadTransition for the two ways it
  // does that), then keeps it centered horizontally for the rest of
  // playback, without disturbing the user's vertical scroll position --
  // centerOn() can't scroll past the view's own scene rect (set in
  // rebuild_piano_roll_scene()), so near the start/end of the song,
  // where centering the playhead would need to scroll past that edge, it
  // instead settles as close to centered as the edge allows
  auto& view = notes_scene.view;
  const auto visible_scene_rect =
      view.mapToScene(get_reference(view.viewport()).rect()).boundingRect();
  const auto vertical_center = visible_scene_rect.center().y();

  auto& playhead_transition = notes_scene.playhead_transition;

  if (playhead_transition == PlayheadTransition::waiting_to_reach_center) {
    // view stays put; playback's own forward motion is what carries the
    // playhead across to the (fixed) center
    if (playhead_x < visible_scene_rect.center().x()) {
      return;
    }
    playhead_transition = PlayheadTransition::none;
  } else if (playhead_transition == PlayheadTransition::catching_up) {
    const auto elapsed_ms =
        static_cast<double>(notes_scene.playhead_elapsed_timer.elapsed());
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
          to_scene_x(notes_scene, std::min(notes_scene.playhead_baseline_ms +
                                               PIANO_ROLL_PLAYHEAD_CATCHUP_MS,
                                           notes_scene.playhead_end_ms));
      const auto center_x =
          notes_scene.playhead_catchup_start_center_x +
          (eased_progress *
           (catchup_end_x - notes_scene.playhead_catchup_start_center_x));
      view.centerOn(center_x, vertical_center);
      return;
    }
    playhead_transition = PlayheadTransition::none;
  }

  view.centerOn(playhead_x, vertical_center);
}
