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

  // how position_playhead() should bring a just-started playhead onto the
  // view's center, instead of snapping there instantly -- see
  // position_playhead() for what each mode does
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

  // the inputs redraw_time_axis_ticks() needs to redraw just the time
  // axis' ticks and labels whenever the zoom changes, without re-running
  // the full rebuild_notes_view()
  double time_axis_max_time_ms = 0.0;
  double time_axis_y = PIANO_ROLL_DEFAULT_AXIS_Y;
  // the tick lines + labels currently on screen, so redraw_time_axis_ticks()
  // can remove exactly those before drawing a fresh set at the new spacing,
  // leaving the rest of the scene (notes, pitch axis, playhead) untouched
  QList<QGraphicsItem *> time_axis_items;

  // rebuilt every rebuild_notes_view() call; each drawn note rect stores its index
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
  }

  NO_MOVE_COPY(PianoRollNotesView)
};

// (re)draws the time axis' ticks and labels, spaced (in ms) so they land
// roughly PIANO_ROLL_TARGET_TICK_PIXEL_SPACING apart on screen at the
// current time_zoom_factor -- called from rebuild_notes_view() for the initial build
// and from set_time_zoom() whenever the zoom changes, since a spacing that
// looked right before a zoom change would otherwise crowd together
// (zooming in) or spread too far apart (zooming out)
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
    // the pitch axis' column, which this view can no longer scroll into
    // (see the view.setSceneRect() call in rebuild_notes_view()) -- so clamp every
    // label's left edge to the axis line instead
    label.setPos(std::max(tick_x - (label.boundingRect().width() / 2),
                          PIANO_ROLL_AXIS_X),
                time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH +
                    PIANO_ROLL_AXIS_LABEL_GAP);
    time_axis_items.push_back(&label);
  }
}

// repopulates the scene with a fresh set of note bars + the pitch/time
// axes for the given song -- called by PianoRollWidget::rebuild_scene()
// whenever the song changes
static void rebuild_notes_view(PianoRollNotesView &notes_view, const Song &song) {
  auto &scene = notes_view.scene;
  auto &playhead_item = notes_view.playhead_item;
  auto &view = notes_view.view;

  scene.removeItem(&playhead_item);
  const auto saved_line = playhead_item.line();
  const auto was_visible = playhead_item.isVisible();

  scene.clear();
  notes_view.note_items.clear();
  // scene.clear() above already deleted these items -- just drop the now-
  // dangling pointers so redraw_time_axis_ticks() doesn't try to remove
  // them again below
  notes_view.time_axis_items.clear();

  const auto &pitched_voices = song.pitched_voices;
  const auto number_of_pitched_voices =
      static_cast<int>(pitched_voices.size());

  auto &events = notes_view.events;
  events = get_piano_roll_events(song);
  notes_view.chord_start_times = get_chord_start_times(song);

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
  //
  // draws a tick + note-name label at every octave (C) at or above the
  // lowest pitched note present, up through the highest octave that still
  // fits within a fixed margin above the highest note -- ticks beyond either
  // margin would just sit off the visible graph, so they're skipped rather
  // than drawn there; axis_y ends up a fixed few semitones below the lowest
  // note (not snapped to any tick), so the lowest note's bar never reads as
  // glued to the axis line
  const auto axis_y = [&]() -> double {
    if (min_midi > max_midi) {
      return PIANO_ROLL_DEFAULT_AXIS_Y;
    }
    const auto axis_pitch = min_midi - PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES;
    const auto top_pitch = max_midi + PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES;
    const auto pitch_axis_y = -axis_pitch * PIANO_ROLL_PIXELS_PER_SEMITONE;
    const auto first_octave =
        C_0_MIDI + to_int(std::ceil((axis_pitch - C_0_MIDI) /
                                    HALFSTEPS_PER_OCTAVE)) *
                       HALFSTEPS_PER_OCTAVE;
    const auto last_octave =
        C_0_MIDI + to_int(std::floor((top_pitch - C_0_MIDI) /
                                     HALFSTEPS_PER_OCTAVE)) *
                       HALFSTEPS_PER_OCTAVE;

    for (auto midi_value = first_octave; midi_value <= last_octave;
        midi_value = midi_value + HALFSTEPS_PER_OCTAVE) {
      const auto tick_y = -midi_value * PIANO_ROLL_PIXELS_PER_SEMITONE;
      scene.addLine(PIANO_ROLL_AXIS_X - PIANO_ROLL_AXIS_TICK_LENGTH, tick_y,
                   PIANO_ROLL_AXIS_X, tick_y);

      auto &label =
          get_reference(scene.addSimpleText(get_note_name(midi_value)));
      const auto &label_rect = label.boundingRect();
      label.setPos(PIANO_ROLL_AXIS_X - PIANO_ROLL_AXIS_TICK_LENGTH -
                      PIANO_ROLL_AXIS_LABEL_GAP - label_rect.width(),
                  tick_y - (label_rect.height() / 2));
    }

    return pitch_axis_y;
  }();
  // draws the horizontal axis line, placed between the pitched notes above
  // and the unpitched lanes below; the line's endpoints are in scene
  // coordinates and don't depend on zoom, so unlike the ticks/labels
  // (redrawn by redraw_time_axis_ticks() below) it's only ever drawn once
  notes_view.time_axis_max_time_ms = max_time_ms;
  notes_view.time_axis_y = axis_y;
  scene.addLine(PIANO_ROLL_AXIS_X, axis_y, max_time_ms * PIANO_ROLL_PIXELS_PER_MS,
               axis_y);
  redraw_time_axis_ticks(notes_view);
  const auto unpitched_lane_top = axis_y + PIANO_ROLL_UNPITCHED_LANE_GAP;

  auto &note_items = notes_view.note_items;
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
  // ticks/labels -- just the time axis, not a full rebuild_notes_view()
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
  // rebuild_notes_view()), so near the start/end of the song, where centering the
  // playhead would need to scroll past that edge, it instead settles as
  // close to centered as the edge allows
  auto &view = notes_view.view;
  const auto visible_scene_rect =
      view.mapToScene(view.viewport()->rect()).boundingRect();
  const auto vertical_center = visible_scene_rect.center().y();

  using PlayheadTransition = PianoRollNotesView::PlayheadTransition;
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

