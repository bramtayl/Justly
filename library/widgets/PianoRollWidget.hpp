#pragma once

#include <QtCore/QChar>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEvent>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMargins>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QSize> 
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtGui/QPolygonF>
#include <QtGui/QTransform>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <utility>

#include "other/PianoRoll.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "sound/PlayState.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"

static const auto PIANO_ROLL_PIXELS_PER_MS = 0.1;
static const auto PIANO_ROLL_PIXELS_PER_SEMITONE = 6;
// how far below the lowest note the horizontal axis sits -- enough that the
// lowest note's bar never reads as glued to (or nearly touching) the axis
// line, without wasting a full octave of empty space underneath it
static const auto PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES = 3.0;
static const auto PIANO_ROLL_LANE_HEIGHT = 20;
static const auto PIANO_ROLL_NOTE_BAR_THICKNESS = 3.0;
static const auto PIANO_ROLL_MIN_BAR_WIDTH = 1.0;
static const auto PIANO_ROLL_TIMER_INTERVAL_MS = 33;
static const auto PIANO_ROLL_MIN_HEIGHT = 300;
static const auto PIANO_ROLL_SCENE_MARGIN = 10.0;
static const auto PIANO_ROLL_AXIS_TICK_LENGTH = 5.0;
static const auto PIANO_ROLL_AXIS_LABEL_GAP = 4.0;
static const auto PIANO_ROLL_AXIS_X = 0.0;
static const auto PIANO_ROLL_DEFAULT_AXIS_Y = 0.0;
static const auto PIANO_ROLL_UNPITCHED_LANE_GAP = 30.0;
static const auto PIANO_ROLL_LEGEND_GAP = 10.0;
static const auto PIANO_ROLL_LEGEND_SWATCH_SIZE = 10.0;
// ticks are re-spaced on every zoom change (see redraw_time_axis_ticks())
// to keep roughly this many screen pixels between them, rather than a fixed
// time interval -- otherwise zooming in would crowd ticks together and
// zooming out would spread them so far apart that most of the timeline
// carries no labels at all
static const auto PIANO_ROLL_TARGET_TICK_PIXEL_SPACING = 80.0;
static const auto PIANO_ROLL_MS_PER_SECOND = 1000.0;
static const auto PIANO_ROLL_MS_PER_MINUTE = 60000.0;
static const auto PIANO_ROLL_SECONDS_PER_MINUTE = 60LL;
static const auto PIANO_ROLL_LABEL_DECIMAL_BASE = 10;
// candidate tick-step multipliers, tried in increasing order against each
// power-of-ten magnitude -- the classic "nice numbers" progression (1, 2, 5,
// then roll over to the next magnitude's 1) that keeps chosen tick values
// round (0.5s, 1s, 2s, 5s, 10s, ...) instead of arbitrary
[[nodiscard]] static auto get_piano_roll_nice_step_multipliers()
    -> const QList<double> & {
  static const QList<double> multipliers{1.0, 2.0, 5.0};
  return multipliers;
}
static const auto PIANO_ROLL_NICE_STEP_ROLLOVER = 10.0;
static const auto PIANO_ROLL_HIGHLIGHT_PEN_WIDTH = 1.5;
static const auto PIANO_ROLL_MIN_TIME_ZOOM = 0.25;
static const auto PIANO_ROLL_MAX_TIME_ZOOM = 8.0;
static const auto PIANO_ROLL_TIME_ZOOM_STEP = 1.25;

// fixed categorical order (never cycled) -- a voice beyond the 8th falls
// back to get_piano_roll_other_voice_color() rather than reusing an earlier hue
[[nodiscard]] static auto get_piano_roll_voice_colors() -> const QList<QColor> & {
  static const QList<QColor> voice_colors{
      QColor("#2a78d6"), QColor("#eb6834"), QColor("#1baf7a"), QColor("#eda100"),
      QColor("#e87ba4"), QColor("#008300"), QColor("#4a3aa7"), QColor("#e34948"),
  };
  return voice_colors;
}

[[nodiscard]] static auto get_piano_roll_other_voice_color() -> const QColor & {
  static const auto other_voice_color = QColor("#898781");
  return other_voice_color;
}

[[nodiscard]] static auto get_voice_color(const int global_voice_index)
    -> QColor {
  const auto &voice_colors = get_piano_roll_voice_colors();
  if (global_voice_index >= 0 && global_voice_index < voice_colors.size()) {
    return voice_colors.at(global_voice_index);
  }
  return get_piano_roll_other_voice_color();
}

// picks a "nice" (1/2/5 * 10^n) tick interval, in ms, close to the raw
// interval that would give PIANO_ROLL_TARGET_TICK_PIXEL_SPACING at the
// current zoom -- so ticks land on round numbers (0.5s, 1s, 2s, ...) rather
// than an arbitrary value like 437ms
[[nodiscard]] static auto choose_time_axis_step_ms(const double time_zoom_factor)
    -> double {
  const auto raw_step_ms = PIANO_ROLL_TARGET_TICK_PIXEL_SPACING /
                           (PIANO_ROLL_PIXELS_PER_MS * time_zoom_factor);
  const auto magnitude = std::pow(PIANO_ROLL_NICE_STEP_ROLLOVER,
                                  std::floor(std::log10(raw_step_ms)));
  const auto fraction = raw_step_ms / magnitude;
  auto nice_fraction = PIANO_ROLL_NICE_STEP_ROLLOVER;
  for (const auto multiplier : get_piano_roll_nice_step_multipliers()) {
    if (fraction <= multiplier) {
      nice_fraction = multiplier;
      break;
    }
  }
  return nice_fraction * magnitude;
}

// formats a time-axis tick label in whichever unit best suits the current
// tick spacing (step_ms) -- milliseconds when ticks are sub-second, seconds
// (with a decimal only when the step itself needs one) once ticks are a
// second or more apart, and minutes:seconds once they're a minute or more
// apart -- so labels stay round and readable at every zoom level rather
// than always being expressed in one fixed unit
[[nodiscard]] static auto format_time_axis_label(const double time_ms,
                                                 const double step_ms)
    -> QString {
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
}

// number_of_notes == -1 (default) means "every note in every chord in
// [first_chord_number, first_chord_number + number_of_chords)". A concrete
// number_of_notes restricts to a single chord's note list (number_of_chords
// should be 1 in that case), matching how the Play menu can select either a
// range of chords or a range of notes within one chord.
[[nodiscard]] static auto get_piano_roll_time_bounds(
    const Song &song, const int first_chord_number,
    const int number_of_chords, const int first_note_number = 0,
    const int number_of_notes = -1,
    const std::optional<PianoRollNoteKind> kind_filter = std::nullopt)
    -> std::pair<double, double> {
  const auto baseline_ms =
      get_play_state_at_chord(song, first_chord_number).current_time;

  auto end_ms = baseline_ms;
  const auto single_chord_note_range = number_of_notes != -1;
  for (const auto &event : get_piano_roll_events(song)) {
    if (event.chord_number < first_chord_number ||
        event.chord_number >= first_chord_number + number_of_chords) {
      continue;
    }
    if (single_chord_note_range) {
      if (event.note_number < first_note_number ||
          event.note_number >= first_note_number + number_of_notes) {
        continue;
      }
      if (kind_filter.has_value() && event.kind != *kind_filter) {
        continue;
      }
    }
    end_ms = std::max(end_ms, event.start_time_ms + event.duration_ms);
  }
  return {baseline_ms, end_ms};
}

// mirrors the filtering in get_piano_roll_time_bounds, but returns every
// matching event's index rather than just the overall time bounds, so the
// piano roll can highlight exactly the notes a table selection corresponds
// to; a chord-row selection matches every note in the selected chords, a
// note-row selection matches only same-kind notes at those row numbers
// within their one parent chord. Voice-row selections (and no selection at
// all, encoded as number_of_rows == 0) have no timeline position and always
// match nothing.
[[nodiscard]] static auto get_selected_piano_roll_event_indices(
    const QList<PianoRollNoteEvent> &events, const RowType selection_row_type,
    const int selection_chord_number, const int selection_first_row_number,
    const int selection_number_of_rows) -> QList<int> {
  QList<int> selected_indices;
  const auto is_chord_selection = selection_row_type == RowType::chord_type;
  const auto is_note_selection = selection_row_type == RowType::pitched_note_type ||
                                 selection_row_type == RowType::unpitched_note_type;
  if (!is_chord_selection && !is_note_selection) {
    return selected_indices;
  }
  const auto kind_filter = selection_row_type == RowType::pitched_note_type
                               ? PianoRollNoteKind::pitched_kind
                               : PianoRollNoteKind::unpitched_kind;
  for (auto event_index = 0; event_index < events.size();
      event_index = event_index + 1) {
    const auto &event = events.at(event_index);
    if (is_chord_selection) {
      if (event.chord_number >= selection_first_row_number &&
         event.chord_number <
             selection_first_row_number + selection_number_of_rows) {
        selected_indices.push_back(event_index);
      }
    } else if (event.chord_number == selection_chord_number &&
              event.kind == kind_filter &&
              event.note_number >= selection_first_row_number &&
              event.note_number <
                  selection_first_row_number + selection_number_of_rows) {
      selected_indices.push_back(event_index);
    }
  }
  return selected_indices;
}

struct PianoRollWidget : public QWidget {
  const SongWidget &song_widget;

  QGraphicsScene &scene = *(new QGraphicsScene(this));
  QGraphicsView &view = *(new QGraphicsView(&scene, this));
  // a second, fixed-width view onto the same scene, locked so it only ever
  // shows the pitch axis' column (x <= PIANO_ROLL_AXIS_X); its vertical
  // scroll is kept in lockstep with the main view's (see the scrollbar
  // connections in the constructor), so the pitch labels stay pinned to the
  // left edge -- and lined up with their notes -- no matter how far the main
  // view is scrolled horizontally
  QGraphicsView &axis_view = *(new QGraphicsView(&scene, this));
  // a separate scene/view for the voice legend, laid out in its own
  // fixed-width column to the right of the main view -- pinned there just
  // like axis_view is pinned to the left, so the legend stays visible and in
  // the same place no matter how far the main view is scrolled, rather than
  // being drawn into the scrollable main scene where it used to disappear
  // off-screen
  QGraphicsScene &legend_scene = *(new QGraphicsScene(this));
  QGraphicsView &legend_view = *(new QGraphicsView(&legend_scene, this));
  QBoxLayout &column_layout = *(new QVBoxLayout(this));
  QBoxLayout &row_layout = *(new QHBoxLayout());
  QGraphicsLineItem &playhead_item = *(new QGraphicsLineItem);

  QTimer &playhead_timer = *(new QTimer(this));
  QElapsedTimer playhead_elapsed_timer;
  double playhead_baseline_ms = 0;
  double playhead_end_ms = 0;
  bool playhead_active = false;
  // true while the user is dragging the playhead with the mouse (between a
  // left-button press and release on the main view); guards the eventFilter's
  // MouseMove handling so ordinary mouse-move events (that aren't part of a
  // drag) don't also move the cursor
  bool playhead_dragging = false;
  // true for the duration of select_chord_at_playhead()'s own call to
  // QItemSelectionModel::select() -- that select() re-enters this widget
  // synchronously via SongEditor's selectionChanged connection
  // (update_piano_roll_selection() -> update_selection() ->
  // apply_selection_highlight()); without this guard, apply_selection_highlight()
  // would treat the sync as an ordinary table-driven selection change and
  // reposition the cursor to the newly-selected chord's start, snapping it
  // backwards away from wherever the drag/playback actually put it
  bool selecting_chord_from_playhead = false;

  // scales only the main view's x axis (time), never its y axis (pitch) --
  // so the pitch axis stays visually fixed (and stays in lockstep with
  // axis_view, which is never zoomed) while the time axis expands/contracts
  double time_zoom_factor = 1.0;

  // the inputs draw_time_axis() needs to redraw just the time axis' ticks
  // and labels (via redraw_time_axis_ticks()) whenever the zoom changes,
  // without re-running the full rebuild_scene()
  double time_axis_max_time_ms = 0.0;
  double time_axis_y = PIANO_ROLL_DEFAULT_AXIS_Y;
  // the tick lines + labels currently on screen, so redraw_time_axis_ticks()
  // can remove exactly those before drawing a fresh set at the new spacing,
  // leaving the rest of the scene (notes, pitch axis, playhead) untouched
  QList<QGraphicsItem *> time_axis_items;

  // rebuilt every rebuild_scene() call; each drawn note rect stores its index
  // into this list (via QGraphicsItem::setData) so a click on the rect can be
  // traced back to the chord/note it represents
  QList<PianoRollNoteEvent> events;
  // parallel to events -- the actual drawn item for each event, so a table
  // selection can be traced forward to the bar(s) it should highlight
  QList<QGraphicsRectItem *> note_items;
  // parallel to song_widget.song.chords, rebuilt alongside events -- lets
  // select_chord_at_playhead() find which chord a cursor time falls in
  // without rescanning the whole song on every playback tick
  QList<double> chord_start_times;

  // the switch table's current selection, mirrored here by SongEditor
  // (via update_selection()) every time it changes, so rebuild_scene() can
  // reapply the same highlight/cursor after redrawing a fresh set of items.
  // number_of_rows == 0 means nothing is selected (the default at startup)
  RowType selection_row_type = RowType::chord_type;
  int selection_chord_number = -1;
  int selection_first_row_number = -1;
  int selection_number_of_rows = 0;

  // set from outside (SongEditor) once it has access to the song menu bar
  // and song widget needed to switch tables; left empty in contexts (e.g.
  // tests) that never wire it up
  std::function<void(int chord_number, int note_number,
                     PianoRollNoteKind kind)>
      note_double_clicked;

  explicit PianoRollWidget(const SongWidget &song_widget_input)
      : song_widget(song_widget_input) {
    // a bottom dock would otherwise default to a cramped sliver; this keeps
    // it usable out of the box while still letting the user drag it taller
    // (or shorter, down to this floor) via the splitter
    setMinimumHeight(PIANO_ROLL_MIN_HEIGHT);

    row_layout.setSpacing(0);
    row_layout.addWidget(&axis_view);
    row_layout.addWidget(&view);
    row_layout.addWidget(&legend_view);
    column_layout.addLayout(&row_layout);

    // legend_view never scrolls horizontally (it's a fixed-width column) and
    // is left free to scroll vertically on its own -- independent of the
    // main view -- so a long voice list stays reachable without the legend's
    // on-screen position ever moving
    legend_view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    legend_view.setFocusPolicy(Qt::NoFocus);

    // axis_view is a read-only mirror of the main view's vertical position
    // -- it never scrolls (or gets scrolled) on its own, horizontally or
    // vertically; see the scrollbar connections below for how it tracks
    // the main view instead
    axis_view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    axis_view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    axis_view.setFocusPolicy(Qt::NoFocus);
    // each QGraphicsView draws its own sunken frame by default, which shows
    // up as a gray seam between axis_view and view even with the layout's
    // spacing at 0 -- dropping both frames removes that seam while leaving
    // the two views' contents flush against each other
    axis_view.setFrameShape(QFrame::NoFrame);
    view.setFrameShape(QFrame::NoFrame);

    QObject::connect(view.verticalScrollBar(), &QScrollBar::valueChanged,
                     this, [this](const int value) -> auto {
                       axis_view.verticalScrollBar()->setValue(value);
                     });
    // kept symmetric so that scrolling with the mouse wheel while hovered
    // over the axis column (still possible despite the hidden scrollbar)
    // moves the main view along with it, rather than desyncing the two
    QObject::connect(axis_view.verticalScrollBar(), &QScrollBar::valueChanged,
                     this, [this](const int value) -> auto {
                       view.verticalScrollBar()->setValue(value);
                     });

    playhead_item.setPen(QPen(Qt::red));
    playhead_item.setZValue(1);
    playhead_item.hide();
    scene.addItem(&playhead_item);

    QObject::connect(&playhead_timer, &QTimer::timeout, this,
                     [this]() -> auto { update_playhead_position(); });

    // keeps the scene point under the cursor fixed on screen while
    // ctrl+wheel zooms the time axis in zoom_in()/zoom_out() below, rather
    // than always zooming around the view's top-left corner
    view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // the view has no interactivity of its own (no item selection, no
    // custom QGraphicsView subclass), so double-clicks and ctrl+wheel zoom
    // are picked up via an event filter on the viewport rather than
    // overriding QGraphicsView
    view.viewport()->installEventFilter(this);

    rebuild_scene();
  }

  auto eventFilter(QObject *watched_pointer, QEvent *event_pointer)
      -> bool override {
    if (event_pointer->type() == QEvent::Wheel &&
       watched_pointer == view.viewport()) {
      auto &wheel_event = get_reference(dynamic_cast<QWheelEvent *>(event_pointer));
      if (wheel_event.modifiers().testFlag(Qt::ControlModifier)) {
        const auto angle_delta_y = wheel_event.angleDelta().y();
        if (angle_delta_y > 0) {
          zoom_in();
        } else if (angle_delta_y < 0) {
          zoom_out();
        }
        return true;
      }
    }
    if (event_pointer->type() == QEvent::MouseButtonDblClick &&
       watched_pointer == view.viewport()) {
      const auto &mouse_event =
          get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
      auto *const item_pointer = scene.itemAt(
          view.mapToScene(mouse_event.pos()), view.transform());
      if (item_pointer != nullptr) {
        const auto event_index_data = item_pointer->data(0);
        if (event_index_data.isValid() && note_double_clicked) {
          const auto &event = events.at(event_index_data.toInt());
          note_double_clicked(event.chord_number, event.note_number,
                             event.kind);
        }
      }
    }
    if (event_pointer->type() == QEvent::MouseButtonPress &&
       watched_pointer == view.viewport()) {
      const auto &mouse_event =
          get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
      if (mouse_event.button() == Qt::LeftButton) {
        // a manual click/drag takes over the cursor from playback's timer-
        // driven animation, the same way it takes over from a stale
        // selection-driven position in drag_playhead_to() below
        if (playhead_active) {
          stop_playhead();
        }
        playhead_dragging = true;
        drag_playhead_to(mouse_event.pos());
        return true;
      }
    }
    if (event_pointer->type() == QEvent::MouseMove && playhead_dragging &&
       watched_pointer == view.viewport()) {
      const auto &mouse_event =
          get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
      drag_playhead_to(mouse_event.pos());
      return true;
    }
    if (event_pointer->type() == QEvent::MouseButtonRelease &&
       playhead_dragging && watched_pointer == view.viewport()) {
      playhead_dragging = false;
      return true;
    }
    return QWidget::eventFilter(watched_pointer, event_pointer);
  }

  // moves the playhead line to the time under the given viewport position,
  // without recentering the view on it the way position_playhead() does for
  // playback -- while the user is actively dragging, the view should hold
  // still under the mouse rather than fight the drag by scrolling
  void drag_playhead_to(const QPoint &viewport_pos) {
    const auto scene_x = view.mapToScene(viewport_pos).x();
    const auto playhead_x = std::max(0.0, scene_x);
    const auto &scene_rect = scene.sceneRect();
    playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                          scene_rect.bottom());
    playhead_item.show();
    select_chord_at_playhead(playhead_x / PIANO_ROLL_PIXELS_PER_MS);
  }

  // called whenever the playhead moves on its own -- from a manual drag
  // (drag_playhead_to() above) or a running playback timer tick
  // (update_playhead_position() below) -- so the switch table's own
  // selection follows the cursor back. Deliberately NOT called from
  // position_playhead() itself, since that's also invoked reactively by
  // apply_selection_highlight() after any table selection change (including
  // a multi-row range); calling this from there would collapse that
  // selection down to a single row the instant it was made. Only applies
  // while the table is showing chords: a note or voice row has no single
  // "current chord" to reselect into, and reselecting a chord there would
  // kick the user out of whichever chord's notes/voices they're editing.
  void select_chord_at_playhead(const double time_ms) {
    auto &switch_table = song_widget.switch_column.switch_table;
    if (switch_table.delegate.current_row_type != RowType::chord_type) {
      return;
    }
    const auto chord_number =
        get_chord_number_at_time(chord_start_times, time_ms);
    if (chord_number < 0) {
      return;
    }
    auto &selection_model = get_selection_model(switch_table);
    const auto selected_rows = selection_model.selectedRows();
    if (selected_rows.size() == 1 && selected_rows.at(0).row() == chord_number) {
      return;
    }
    const auto chord_index = switch_table.chords_model.index(chord_number, 0);
    selecting_chord_from_playhead = true;
    selection_model.select(chord_index, QItemSelectionModel::Select |
                                            QItemSelectionModel::Clear |
                                            QItemSelectionModel::Rows);
    selecting_chord_from_playhead = false;
    switch_table.scrollTo(chord_index);
  }

  void rebuild_scene() {
    scene.removeItem(&playhead_item);
    const auto saved_line = playhead_item.line();
    const auto was_visible = playhead_item.isVisible();

    scene.clear();
    note_items.clear();
    // scene.clear() above already deleted these items -- just drop the now-
    // dangling pointers so redraw_time_axis_ticks() doesn't try to remove
    // them again when draw_time_axis() calls it below
    time_axis_items.clear();

    const auto &song = song_widget.song;
    const auto &pitched_voices = song.pitched_voices;
    const auto &unpitched_voices = song.unpitched_voices;
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
    const auto axis_y = draw_pitch_axis(min_midi, max_midi);
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

    legend_scene.clear();
    draw_legend(pitched_voices, unpitched_voices);
    // size legend_view to exactly fit its content (plus a margin), so the
    // fixed-width column stays as narrow as the longest voice name rather
    // than an arbitrary guessed width
    const auto legend_bounds = legend_scene.itemsBoundingRect().adjusted(
        -PIANO_ROLL_LEGEND_GAP, -PIANO_ROLL_LEGEND_GAP, PIANO_ROLL_LEGEND_GAP,
        PIANO_ROLL_LEGEND_GAP);
    legend_scene.setSceneRect(legend_bounds);
    legend_view.setFixedWidth(static_cast<int>(std::ceil(legend_bounds.width())) +
                              (2 * legend_view.frameWidth()));

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

    // only the pitch axis' ticks/labels have negative x, so the scene rect's
    // left edge is exactly the widest label's left edge; giving axis_view
    // that same rect (but overriding its own, view-local scene rect rather
    // than the shared QGraphicsScene's) as both its fixed width and its
    // scrollable area keeps it permanently framed on just the axis column,
    // with zero horizontal scroll range
    const auto &scene_rect = scene.sceneRect();
    const auto axis_column_width = PIANO_ROLL_AXIS_X - scene_rect.left();
    axis_view.setFixedWidth(static_cast<int>(std::ceil(axis_column_width)) +
                            (2 * axis_view.frameWidth()));
    axis_view.setSceneRect(scene_rect.left(), scene_rect.top(),
                           axis_column_width, scene_rect.height());
    // and the mirror image for the main view: without this, scrolling it
    // all the way left would re-reveal the same axis labels a second time
    // (axis_view already owns that column), doubling them up
    view.setSceneRect(PIANO_ROLL_AXIS_X, scene_rect.top(),
                      scene_rect.right() - PIANO_ROLL_AXIS_X,
                      scene_rect.height());

    update_max_height(scene_rect.height());

    apply_selection_highlight();
  }

  // a short song (few voices, narrow pitch range) doesn't need nearly as
  // much vertical space as PIANO_ROLL_MIN_HEIGHT reserves -- letting the dock
  // grow well past the content just leaves blank gray space below the last
  // note. Capping it at the content's own height (plus the chrome around it)
  // means dragging the dock splitter taller stops being useful once the
  // whole piano roll is already on screen, rather than dragging in dead
  // space. Still floored at PIANO_ROLL_MIN_HEIGHT so an empty/near-empty
  // piano roll doesn't collapse to a sliver.
  void update_max_height(const double content_height) {
    const auto chrome_height =
        (2 * view.frameWidth()) + view.horizontalScrollBar()->sizeHint().height() +
        column_layout.contentsMargins().top() +
        column_layout.contentsMargins().bottom();
    setMaximumHeight(static_cast<int>(std::max(
        static_cast<double>(PIANO_ROLL_MIN_HEIGHT),
        std::ceil(content_height + chrome_height))));
  }

  // sets the main view's horizontal scale directly (rather than accumulating
  // via QGraphicsView::scale()) so repeated zoom_in()/zoom_out() calls can't
  // drift and clamping is just one std::clamp on the absolute factor; the
  // vertical scale is always left at 1, so the pitch axis (and axis_view,
  // which is never zoomed) stays visually fixed while only the time axis
  // expands/contracts
  void set_time_zoom(const double new_zoom_factor) {
    time_zoom_factor = std::clamp(new_zoom_factor, PIANO_ROLL_MIN_TIME_ZOOM,
                                  PIANO_ROLL_MAX_TIME_ZOOM);
    view.setTransform(QTransform::fromScale(time_zoom_factor, 1.0));
    // the tick spacing (in ms) that keeps ticks ~evenly spaced on screen
    // depends on the zoom factor, so every zoom change needs a fresh set of
    // ticks/labels -- just the time axis, not a full rebuild_scene()
    redraw_time_axis_ticks();
  }

  void zoom_in() { set_time_zoom(time_zoom_factor * PIANO_ROLL_TIME_ZOOM_STEP); }

  void zoom_out() { set_time_zoom(time_zoom_factor / PIANO_ROLL_TIME_ZOOM_STEP); }

  // called by SongEditor whenever the switch table's selection changes, so
  // the piano roll can mirror it: highlight the corresponding note bar(s),
  // jump the cursor to the selection's start, and scroll to keep both in
  // view. number_of_rows == 0 clears the highlight and hides the cursor
  // (used both for "nothing selected" and for voice-row selections, which
  // have no timeline position).
  void update_selection(const RowType row_type, const int chord_number,
                        const int first_row_number,
                        const int number_of_rows) {
    selection_row_type = row_type;
    selection_chord_number = chord_number;
    selection_first_row_number = first_row_number;
    selection_number_of_rows = number_of_rows;
    apply_selection_highlight();
  }

  // reapplies the highlight/cursor implied by the current selection_* fields
  // against the current note_items -- called both from update_selection()
  // and from the end of rebuild_scene(), since rebuilding replaces every
  // QGraphicsRectItem (and thus wipes any highlight pen set on the old ones)
  void apply_selection_highlight() {
    const auto selected_indices = get_selected_piano_roll_event_indices(
        events, selection_row_type, selection_chord_number,
        selection_first_row_number, selection_number_of_rows);

    QList<bool> is_selected(static_cast<int>(events.size()), false);
    for (const auto selected_index : selected_indices) {
      is_selected[selected_index] = true;
    }

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

    const auto is_chord_selection = selection_row_type == RowType::chord_type;
    const auto is_note_selection =
        selection_row_type == RowType::pitched_note_type ||
        selection_row_type == RowType::unpitched_note_type;
    const auto has_selection = (is_chord_selection || is_note_selection) &&
                               selection_number_of_rows > 0;

    if (!has_selection) {
      if (!playhead_active) {
        playhead_item.hide();
      }
      return;
    }

    // playback already owns the cursor line while it's running, and so does
    // an in-progress manual drag (which set it more precisely than a chord's
    // start time); a selection change caused by select_chord_at_playhead()
    // itself shouldn't reposition the cursor either, since it's just
    // reporting where the cursor already is -- don't yank it away from any
    // of the three just because the table selection changed underneath it
    if (!playhead_active && !playhead_dragging &&
       !selecting_chord_from_playhead) {
      const auto baseline_ms =
          get_piano_roll_time_bounds(
              song_widget.song,
              is_chord_selection ? selection_first_row_number
                                 : selection_chord_number,
              is_chord_selection ? selection_number_of_rows : 1,
              is_chord_selection ? 0 : selection_first_row_number,
              is_chord_selection ? -1 : selection_number_of_rows,
              is_chord_selection
                  ? std::nullopt
                  : std::make_optional(
                        selection_row_type == RowType::pitched_note_type
                            ? PianoRollNoteKind::pitched_kind
                            : PianoRollNoteKind::unpitched_kind))
              .first;
      playhead_item.show();
      position_playhead(baseline_ms);
    }
    if (!highlighted_bounds.isNull()) {
      view.ensureVisible(highlighted_bounds,
                         static_cast<int>(PIANO_ROLL_SCENE_MARGIN),
                         static_cast<int>(PIANO_ROLL_SCENE_MARGIN));
    }
  }

  // lists every voice (pitched first, then unpitched) as a colored swatch +
  // name, in the same order used to assign global_voice_index for coloring.
  // Drawn into legend_scene (not the scrollable main scene) so the legend
  // stays fixed in place regardless of how far the piano roll is scrolled
  void draw_legend(const QList<PitchedVoice> &pitched_voices,
                   const QList<UnpitchedVoice> &unpitched_voices) {
    auto row_y = 0.0;
    auto global_voice_index = 0;
    for (const auto &voice : pitched_voices) {
      draw_legend_row(voice.name, global_voice_index, row_y);
      row_y = row_y + PIANO_ROLL_LANE_HEIGHT;
      global_voice_index = global_voice_index + 1;
    }
    for (const auto &voice : unpitched_voices) {
      draw_legend_row(voice.name, global_voice_index, row_y);
      row_y = row_y + PIANO_ROLL_LANE_HEIGHT;
      global_voice_index = global_voice_index + 1;
    }
  }

  void draw_legend_row(const QString &name, const int global_voice_index,
                       const double row_y) {
    legend_scene.addRect(0, row_y, PIANO_ROLL_LEGEND_SWATCH_SIZE,
                 PIANO_ROLL_LEGEND_SWATCH_SIZE, QPen(Qt::NoPen),
                 QBrush(get_voice_color(global_voice_index)));
    auto &label = get_reference(legend_scene.addSimpleText(name));
    label.setPos(PIANO_ROLL_LEGEND_SWATCH_SIZE + PIANO_ROLL_AXIS_LABEL_GAP,
                row_y - ((label.boundingRect().height() -
                         PIANO_ROLL_LEGEND_SWATCH_SIZE) /
                        2));
  }

  // draws a tick + note-name label at every octave (C) at or above the
  // lowest pitched note present, up through the highest octave that still
  // fits within a fixed margin above the highest note -- ticks beyond either
  // margin would just sit off the visible graph, so they're skipped rather
  // than drawn there; returns the y position of the horizontal time axis, a
  // fixed few semitones below the lowest note (not snapped to any tick), so
  // the lowest note's bar never reads as glued to the axis line
  [[nodiscard]] auto draw_pitch_axis(const double min_midi,
                                     const double max_midi) -> double {
    if (min_midi > max_midi) {
      return PIANO_ROLL_DEFAULT_AXIS_Y;
    }
    const auto axis_pitch =
        min_midi - PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES;
    const auto top_pitch =
        max_midi + PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES;
    const auto axis_y = -axis_pitch * PIANO_ROLL_PIXELS_PER_SEMITONE;
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

    return axis_y;
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

    const auto step_ms = choose_time_axis_step_ms(time_zoom_factor);
    for (auto step_number = 0;
        step_number * step_ms <= time_axis_max_time_ms;
        step_number = step_number + 1) {
      const auto time_ms = step_number * step_ms;
      const auto tick_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
      time_axis_items.push_back(
          scene.addLine(tick_x, time_axis_y, tick_x,
                       time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH));

      auto &label = get_reference(
          scene.addSimpleText(format_time_axis_label(time_ms, step_ms)));
      // keeps the label's on-screen size constant across zoom levels --
      // without this, since the label lives in the same scene as the notes
      // it gets rendered through the main view's time-axis-only x scale
      // (see set_time_zoom()), stretching or squeezing its glyphs
      // horizontally instead of just moving the ticks further apart
      label.setFlag(QGraphicsItem::ItemIgnoresTransformations);
      // centering would push the "0ms"/"0s" label partway into negative x --
      // the pitch axis' column, which the main view can no longer scroll
      // into (see the view.setSceneRect() call in rebuild_scene()) -- so
      // clamp every label's left edge to the axis line instead
      label.setPos(std::max(tick_x - (label.boundingRect().width() / 2),
                            PIANO_ROLL_AXIS_X),
                  time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH +
                      PIANO_ROLL_AXIS_LABEL_GAP);
      time_axis_items.push_back(&label);
    }
  }

  void position_playhead(const double time_ms) {
    const auto playhead_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
    const auto &scene_rect = scene.sceneRect();
    playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                          scene_rect.bottom());
    follow_playhead(playhead_x);
  }

  // keeps the moving playhead centered horizontally, without disturbing the
  // user's vertical scroll position -- centerOn() can't scroll past the
  // view's own scene rect (set in rebuild_scene()), so near the start/end of
  // the song, where centering the playhead would need to scroll past that
  // edge, it instead settles as close to centered as the edge allows
  void follow_playhead(const double playhead_x) {
    const auto visible_scene_rect =
        view.mapToScene(view.viewport()->rect()).boundingRect();
    view.centerOn(playhead_x, visible_scene_rect.center().y());
  }

  // follow_playhead() calls ensureVisible() every tick while playing,
  // fighting any manual scroll (drag on the scrollbar, or wheel) the user
  // does at the same time -- the two writes to the same scroll position
  // within one 33ms tick used to leave rendering artifacts behind that read
  // as extra, stuck red cursor lines. Disabling manual scrolling during
  // playback removes the conflicting writer entirely.
  void set_manual_scrolling_enabled(const bool enabled) {
    view.horizontalScrollBar()->setEnabled(enabled);
    view.verticalScrollBar()->setEnabled(enabled);
    axis_view.verticalScrollBar()->setEnabled(enabled);
  }

  void start_playhead(const double baseline_ms, const double end_ms) {
    playhead_baseline_ms = baseline_ms;
    playhead_end_ms = end_ms;
    playhead_elapsed_timer.restart();
    playhead_active = true;
    playhead_item.show();
    set_manual_scrolling_enabled(false);
    position_playhead(baseline_ms);
    playhead_timer.start(PIANO_ROLL_TIMER_INTERVAL_MS);
  }

  void stop_playhead() {
    playhead_timer.stop();
    playhead_active = false;
    playhead_item.hide();
    set_manual_scrolling_enabled(true);
  }

  void update_playhead_position() {
    if (!playhead_active) {
      return;
    }
    const auto current_ms =
        playhead_baseline_ms +
        static_cast<double>(playhead_elapsed_timer.elapsed());
    if (current_ms >= playhead_end_ms) {
      playhead_active = false;
      playhead_timer.stop();
      set_manual_scrolling_enabled(true);
      position_playhead(playhead_end_ms);
      select_chord_at_playhead(playhead_end_ms);
      return;
    }
    position_playhead(current_ms);
    select_chord_at_playhead(current_ms);
  }
};
