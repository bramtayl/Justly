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
#include <cstdint>
#include <limits>

#include "other/PianoRoll.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/PitchedNote.hpp"
#include "widgets/piano_roll/PianoRollAxisView.hpp"
#include "widgets/piano_roll/piano_roll_helpers.hpp"

// the main scrollable graphics view: the note bars, the pitch/time axes,
// and the playhead cursor + its playback animation all live here
struct PianoRollNotesView {
  QGraphicsScene &scene;
  QGraphicsView &view;
  QGraphicsLineItem &playhead_item = *(new QGraphicsLineItem);

  QTimer &playhead_timer;
  QElapsedTimer playhead_elapsed_timer;
  double playhead_baseline_ms = 0;
  double playhead_end_ms = 0;
  bool playhead_active = false;

  // how follow_playhead() should bring a just-started playhead onto the
  // view's center, instead of snapping there instantly -- see
  // follow_playhead() for what each mode does
  enum class PlayheadTransition : std::uint8_t {
    // no transition in progress -- every tick centers the view on the
    // playhead, as usual
    none,
    // the playhead started left of center: the view holds still and waits
    // for playback's own forward motion to carry the playhead to the
    // view's (fixed) center before switching to normal following
    waiting_to_reach_center,
    // the playhead started right of center: since playback only moves it
    // further right, the view instead eases itself from its starting
    // position to where the playhead will be once the catch-up window
    // ends, so the animated scroll and the playhead's real-time motion
    // converge together exactly at the center
    catching_up,
  };
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

  // the inputs draw_time_axis() needs to redraw just the time axis' ticks
  // and labels (via redraw_time_axis_ticks()) whenever the zoom changes,
  // without re-running the full rebuild()
  double time_axis_max_time_ms = 0.0;
  double time_axis_y = PIANO_ROLL_DEFAULT_AXIS_Y;
  // the tick lines + labels currently on screen, so redraw_time_axis_ticks()
  // can remove exactly those before drawing a fresh set at the new spacing,
  // leaving the rest of the scene (notes, pitch axis, playhead) untouched
  QList<QGraphicsItem *> time_axis_items;

  // rebuilt every rebuild() call; each drawn note rect stores its index
  // into this list (via QGraphicsItem::setData) so a click on the rect can be
  // traced back to the chord/note it represents
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
    // ctrl+wheel zooms the time axis in zoom_in()/zoom_out() below, rather
    // than always zooming around the view's top-left corner
    view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // each QGraphicsView draws its own sunken frame by default, which shows
    // up as a gray seam between this view and the axis view even with the
    // layout's spacing at 0 -- dropping the frame removes that seam
    view.setFrameShape(QFrame::NoFrame);

    playhead_item.setPen(QPen(Qt::red));
    playhead_item.setZValue(1);
    playhead_item.hide();
    scene.addItem(&playhead_item);
  }

  NO_MOVE_COPY(PianoRollNotesView)

  void set_scrolling_enabled(const bool enabled) {
    view.horizontalScrollBar()->setEnabled(enabled);
    view.verticalScrollBar()->setEnabled(enabled);
  }

  // repopulates the scene with a fresh set of note bars + the pitch/time
  // axes for the given song -- called by PianoRollWidget::rebuild_scene()
  // whenever the song changes
  void rebuild(const Song &song) {
    scene.removeItem(&playhead_item);
    const auto saved_line = playhead_item.line();
    const auto was_visible = playhead_item.isVisible();

    scene.clear();
    note_items.clear();
    // scene.clear() above already deleted these items -- just drop the now-
    // dangling pointers so redraw_time_axis_ticks() doesn't try to remove
    // them again when draw_time_axis() calls it below
    time_axis_items.clear();

    const auto &pitched_voices = song.pitched_voices;
    const auto number_of_pitched_voices =
        static_cast<int>(pitched_voices.size());

    events = get_piano_roll_events(song);
    chord_start_times = get_chord_start_times(song);

    auto min_midi = std::numeric_limits<double>::max();
    auto max_midi = std::numeric_limits<double>::lowest();
    auto max_time_ms = 0.0;
    for (const auto &event : events) {
      max_time_ms =
          std::max(max_time_ms, event.start_time_ms + event.duration_ms);
      if (event.kind == PianoRollNoteKind::pitched_kind) {
        const auto midi_number = frequency_to_midi_number(event.frequency);
        min_midi = std::min(min_midi, midi_number);
        max_midi = std::max(max_midi, midi_number);
      }
    }

    // greedily pack unpitched notes into the fewest lanes with no time
    // overlap, rather than giving every unpitched voice its own fixed lane
    // -- voice identity is carried by bar color (+ the legend) instead
    QList<int> unpitched_lane_by_event(static_cast<int>(events.size()), -1);
    QList<double> lane_end_times;
    for (auto event_index = 0; event_index < events.size();
        event_index = event_index + 1) {
      const auto &event = events.at(event_index);
      if (event.kind != PianoRollNoteKind::unpitched_kind) {
        continue;
      }
      auto assigned_lane = -1;
      for (auto lane_index = 0; lane_index < lane_end_times.size();
          lane_index = lane_index + 1) {
        if (lane_end_times.at(lane_index) <= event.start_time_ms) {
          assigned_lane = lane_index;
          break;
        }
      }
      if (assigned_lane == -1) {
        assigned_lane = static_cast<int>(lane_end_times.size());
        lane_end_times.push_back(0);
      }
      lane_end_times[assigned_lane] = event.start_time_ms + event.duration_ms;
      unpitched_lane_by_event[event_index] = assigned_lane;
    }
    // both axes sit at x/y == PIANO_ROLL_AXIS_X, so the horizontal axis and
    // the t=0 time tick meet at one corner
    const auto axis_y = draw_pitch_axis(scene, min_midi, max_midi);
    draw_time_axis(max_time_ms, axis_y);
    const auto unpitched_lane_top = axis_y + PIANO_ROLL_UNPITCHED_LANE_GAP;

    for (auto event_index = 0; event_index < events.size();
        event_index = event_index + 1) {
      const auto &event = events.at(event_index);
      const auto bar_x = event.start_time_ms * PIANO_ROLL_PIXELS_PER_MS;
      const auto width =
          std::max(PIANO_ROLL_MIN_BAR_WIDTH,
                   event.duration_ms * PIANO_ROLL_PIXELS_PER_MS);

      const auto is_pitched = event.kind == PianoRollNoteKind::pitched_kind;
      const auto lane_y =
          is_pitched ? -frequency_to_midi_number(event.frequency) *
                           PIANO_ROLL_PIXELS_PER_SEMITONE
                     : unpitched_lane_top +
                           (unpitched_lane_by_event.at(event_index) *
                            PIANO_ROLL_LANE_HEIGHT);
      // pitched lane_y is the exact pitch line (one semitone = 6px), so
      // center on it symmetrically; unpitched lane_y is the top of a much
      // taller 20px band, so offset down instead. Using the unpitched
      // (band-top) offset for pitched notes too used to push low notes'
      // bars several pixels below their true pitch line -- enough to dip
      // below the horizontal axis for the lowest notes in a song.
      const auto bar_y =
          is_pitched
              ? lane_y - (PIANO_ROLL_NOTE_BAR_THICKNESS / 2)
              : lane_y + ((PIANO_ROLL_LANE_HEIGHT -
                          PIANO_ROLL_NOTE_BAR_THICKNESS) /
                         2);

      const auto global_voice_index = is_pitched
                                          ? event.voice_number
                                          : number_of_pitched_voices +
                                                event.voice_number;
      auto &note_item = get_reference(scene.addRect(
          bar_x, bar_y, width, PIANO_ROLL_NOTE_BAR_THICKNESS, QPen(Qt::NoPen),
          QBrush(get_voice_color(global_voice_index))));
      // lets the double-click event filter trace a clicked rect back to the
      // PianoRollNoteEvent (and thus chord/note) it represents
      note_item.setData(0, event_index);
      note_items.push_back(&note_item);
    }

    // sized from itemsBoundingRect() before the playhead is added back in --
    // otherwise its line (spanning the full previous scene height, restored
    // from saved_line below) would get baked into this pass' bounding box,
    // permanently inflating the scrollable area with stale blank space that
    // never shrinks back down even after the real content shrinks
    scene.setSceneRect(scene.itemsBoundingRect().adjusted(
        -PIANO_ROLL_SCENE_MARGIN, -PIANO_ROLL_SCENE_MARGIN,
        PIANO_ROLL_SCENE_MARGIN, PIANO_ROLL_SCENE_MARGIN));

    scene.addItem(&playhead_item);
    playhead_item.setLine(saved_line);
    playhead_item.setVisible(was_visible);

    // without this, scrolling this view all the way left would re-reveal
    // the same axis labels a second time (PianoRollAxisView already owns
    // that column), doubling them up
    const auto &scene_rect = scene.sceneRect();
    view.setSceneRect(PIANO_ROLL_AXIS_X, scene_rect.top(),
                      scene_rect.right() - PIANO_ROLL_AXIS_X,
                      scene_rect.height());
  }

  // draws the horizontal axis line, placed between the pitched notes above
  // and the unpitched lanes below; the line's endpoints are in scene
  // coordinates and don't depend on zoom, so unlike the ticks/labels it's
  // drawn once here rather than in redraw_time_axis_ticks()
  void draw_time_axis(const double max_time_ms, const double axis_y) {
    time_axis_max_time_ms = max_time_ms;
    time_axis_y = axis_y;
    scene.addLine(PIANO_ROLL_AXIS_X, axis_y,
                 max_time_ms * PIANO_ROLL_PIXELS_PER_MS, axis_y);
    redraw_time_axis_ticks();
  }

  // (re)draws the time axis' ticks and labels, spaced (in ms) so they land
  // roughly PIANO_ROLL_TARGET_TICK_PIXEL_SPACING apart on screen at the
  // current time_zoom_factor -- called from draw_time_axis() for the
  // initial build and from set_time_zoom() whenever the zoom changes, since
  // a spacing that looked right before a zoom change would otherwise crowd
  // together (zooming in) or spread too far apart (zooming out)
  void redraw_time_axis_ticks() {
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
                             (PIANO_ROLL_PIXELS_PER_MS * time_zoom_factor);
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
      // the pitch axis' column, which this view can no longer scroll into
      // (see the view.setSceneRect() call in rebuild()) -- so clamp every
      // label's left edge to the axis line instead
      label.setPos(std::max(tick_x - (label.boundingRect().width() / 2),
                            PIANO_ROLL_AXIS_X),
                  time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH +
                      PIANO_ROLL_AXIS_LABEL_GAP);
      time_axis_items.push_back(&label);
    }
  }

  // sets this view's horizontal scale directly (rather than accumulating
  // via QGraphicsView::scale()) so repeated zoom_in()/zoom_out() calls can't
  // drift and clamping is just one std::clamp on the absolute factor; the
  // vertical scale is always left at 1, so the pitch axis (and
  // PianoRollAxisView, which is never zoomed) stays visually fixed while
  // only the time axis expands/contracts
  void set_time_zoom(const double new_zoom_factor) {
    time_zoom_factor = std::clamp(new_zoom_factor, PIANO_ROLL_MIN_TIME_ZOOM,
                                  PIANO_ROLL_MAX_TIME_ZOOM);
    view.setTransform(QTransform::fromScale(time_zoom_factor, 1.0));
    // the tick spacing (in ms) that keeps ticks ~evenly spaced on screen
    // depends on the zoom factor, so every zoom change needs a fresh set of
    // ticks/labels -- just the time axis, not a full rebuild()
    redraw_time_axis_ticks();
  }

  void zoom_in() { set_time_zoom(time_zoom_factor * PIANO_ROLL_TIME_ZOOM_STEP); }

  void zoom_out() { set_time_zoom(time_zoom_factor / PIANO_ROLL_TIME_ZOOM_STEP); }

  // styles each note bar's pen based on is_selected (parallel to
  // events/note_items), returning the union of the highlighted bars' scene
  // bounds (a null rect if none are highlighted) so the caller can scroll
  // them into view
  [[nodiscard]] auto apply_highlight(const QList<bool> &is_selected)
      -> QRectF {
    QRectF highlighted_bounds;
    for (auto event_index = 0; event_index < note_items.size();
        event_index = event_index + 1) {
      auto &note_item = get_reference(note_items.at(event_index));
      if (is_selected.at(event_index)) {
        note_item.setPen(QPen(Qt::black, PIANO_ROLL_HIGHLIGHT_PEN_WIDTH));
        highlighted_bounds =
            highlighted_bounds.united(note_item.sceneBoundingRect());
      } else {
        note_item.setPen(QPen(Qt::NoPen));
      }
    }
    return highlighted_bounds;
  }

  // moves the playhead line to the time under the given viewport position,
  // without recentering the view on it the way position_playhead() does for
  // playback -- while the user is actively dragging, the view should hold
  // still under the mouse rather than fight the drag by scrolling; returns
  // the playhead's new x position (in scene coordinates) so the caller can
  // translate it back to a time and sync the switch table's selection
  [[nodiscard]] auto drag_playhead_to(const QPoint &viewport_pos) -> double {
    const auto playhead_x = std::max(0.0, view.mapToScene(viewport_pos).x());
    const auto &scene_rect = scene.sceneRect();
    playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                          scene_rect.bottom());
    playhead_item.show();
    return playhead_x;
  }

  // follow_view lets a caller move the playhead line without recentering the
  // view on it -- used when playback has already stopped (see
  // PianoRollWidget::apply_selection_highlight()), where forcibly
  // recentering would yank the view away from wherever the user had it
  // scrolled
  void position_playhead(const double time_ms, const bool follow_view = true) {
    const auto playhead_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
    const auto &scene_rect = scene.sceneRect();
    playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                          scene_rect.bottom());
    if (follow_view) {
      follow_playhead(playhead_x);
    }
  }

  // eases a just-started playhead onto the view's center instead of
  // snapping there instantly (see playhead_transition for the two ways it
  // does that), then keeps it centered horizontally for the rest of
  // playback, without disturbing the user's vertical scroll position --
  // centerOn() can't scroll past the view's own scene rect (set in
  // rebuild()), so near the start/end of the song, where centering the
  // playhead would need to scroll past that edge, it instead settles as
  // close to centered as the edge allows
  void follow_playhead(const double playhead_x) {
    const auto visible_scene_rect =
        view.mapToScene(view.viewport()->rect()).boundingRect();
    const auto vertical_center = visible_scene_rect.center().y();

    if (playhead_transition == PlayheadTransition::waiting_to_reach_center) {
      // view stays put; playback's own forward motion is what carries the
      // playhead across to the (fixed) center
      if (playhead_x < visible_scene_rect.center().x()) {
        return;
      }
      playhead_transition = PlayheadTransition::none;
    } else if (playhead_transition == PlayheadTransition::catching_up) {
      const auto elapsed_ms =
          static_cast<double>(playhead_elapsed_timer.elapsed());
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
            std::min(playhead_baseline_ms + PIANO_ROLL_PLAYHEAD_CATCHUP_MS,
                     playhead_end_ms) *
            PIANO_ROLL_PIXELS_PER_MS;
        const auto center_x =
            playhead_catchup_start_center_x +
            (eased_progress *
             (catchup_end_x - playhead_catchup_start_center_x));
        view.centerOn(center_x, vertical_center);
        return;
      }
      playhead_transition = PlayheadTransition::none;
    }

    view.centerOn(playhead_x, vertical_center);
  }

  void start_playhead(const double baseline_ms, const double end_ms) {
    playhead_baseline_ms = baseline_ms;
    playhead_end_ms = end_ms;
    playhead_elapsed_timer.restart();
    playhead_active = true;
    playhead_item.show();

    // decides which transition follow_playhead() should run, based on
    // where the playhead is starting relative to the view's current
    // (not-yet-moved) center -- see PlayheadTransition
    const auto initial_center_x =
        view.mapToScene(view.viewport()->rect()).boundingRect().center().x();
    const auto playhead_x = baseline_ms * PIANO_ROLL_PIXELS_PER_MS;
    if (playhead_x <= initial_center_x) {
      playhead_transition = PlayheadTransition::waiting_to_reach_center;
    } else {
      playhead_transition = PlayheadTransition::catching_up;
      playhead_catchup_start_center_x = initial_center_x;
    }

    position_playhead(baseline_ms);
    playhead_timer.start(PIANO_ROLL_TIMER_INTERVAL_MS);
  }

  void stop_playhead() {
    playhead_timer.stop();
    playhead_active = false;
    playhead_transition = PlayheadTransition::none;
  }
};
