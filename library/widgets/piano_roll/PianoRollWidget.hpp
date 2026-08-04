#pragma once

#include <QtCore/QEvent>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <QtCore/Qt>
#include <QtGui/QPen>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QBoxLayout>
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
#include "rows/RowType.hpp"
#include "sound/PlayState.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/piano_roll/PianoRollAxisView.hpp"
#include "widgets/piano_roll/PianoRollLegendView.hpp"
#include "widgets/piano_roll/PianoRollNotesView.hpp"
#include "widgets/piano_roll/PlayheadTransition.hpp"
#include "widgets/piano_roll/piano_roll_helpers.hpp"

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

struct PianoRollWidget;

// forward-declared so PianoRollWidget's own inline constructor/eventFilter
// bodies (below) can call them -- their definitions, after the struct, need
// its complete type
static void select_chord_at_playhead(PianoRollWidget &widget,
                                     double time_ms);
static void rebuild_scene(PianoRollWidget &widget);
static void zoom_in(PianoRollWidget &widget);
static void zoom_out(PianoRollWidget &widget);
static void stop_playhead(PianoRollWidget &widget);
static void update_playhead_position(PianoRollWidget &widget);

struct PianoRollWidget : public QWidget {
  const SongWidget &song_widget;

  PianoRollNotesView piano_roll_view = PianoRollNotesView(*this);
  // a second, fixed-width view onto piano_roll_view's scene, pinned to the
  // left edge -- see PianoRollAxisView for details
  PianoRollAxisView axis_view = PianoRollAxisView(*this, piano_roll_view.scene);
  // a separate scene/view for the voice legend, pinned to the right edge --
  // see PianoRollLegendView for details
  PianoRollLegendView legend_view = PianoRollLegendView(*this);

  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  // true for the duration of select_chord_at_playhead()'s own call to
  // QItemSelectionModel::select() -- that select() re-enters this widget
  // synchronously via SongEditor's selectionChanged connection
  // (update_piano_roll_selection() -> update_piano_roll_widget_selection() ->
  // apply_selection_highlight()); without this guard, apply_selection_highlight()
  // would treat the sync as an ordinary table-driven selection change and
  // reposition the cursor to the newly-selected chord's start, snapping it
  // backwards away from wherever the drag/playback actually put it
  bool selecting_chord_from_playhead = false;

  // the switch table's current selection, mirrored here by SongEditor
  // (via update_piano_roll_widget_selection()) every time it changes, so rebuild_scene() can
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
    row_layout.addWidget(&axis_view.view);
    row_layout.addWidget(&piano_roll_view.view);
    row_layout.addWidget(&legend_view.view);

    QObject::connect(piano_roll_view.view.verticalScrollBar(),
                     &QScrollBar::valueChanged, this,
                     [this](const int value) -> auto {
                       axis_view.view.verticalScrollBar()->setValue(value);
                     });
    // kept symmetric so that scrolling with the mouse wheel while hovered
    // over the axis column (still possible despite the hidden scrollbar)
    // moves the main view along with it, rather than desyncing the two
    QObject::connect(axis_view.view.verticalScrollBar(),
                     &QScrollBar::valueChanged, this,
                     [this](const int value) -> auto {
                       piano_roll_view.view.verticalScrollBar()->setValue(
                           value);
                     });

    QObject::connect(&piano_roll_view.playhead_timer, &QTimer::timeout, this,
                     [this]() -> auto { update_playhead_position(*this); });

    // the view has no interactivity of its own (no item selection, no
    // custom QGraphicsView subclass), so double-clicks and ctrl+wheel zoom
    // are picked up via an event filter on the viewport rather than
    // overriding QGraphicsView
    piano_roll_view.view.viewport()->installEventFilter(this);

    rebuild_scene(*this);
  }

  auto eventFilter(QObject *watched_pointer, QEvent *event_pointer)
      -> bool override {
    auto &view = piano_roll_view.view;
    if (event_pointer->type() == QEvent::Wheel &&
       watched_pointer == view.viewport()) {
      auto &wheel_event = get_reference(dynamic_cast<QWheelEvent *>(event_pointer));
      if (wheel_event.modifiers().testFlag(Qt::ControlModifier)) {
        const auto angle_delta_y = wheel_event.angleDelta().y();
        if (angle_delta_y > 0) {
          zoom_in(*this);
        } else if (angle_delta_y < 0) {
          zoom_out(*this);
        }
        return true;
      }
    }
    if (event_pointer->type() == QEvent::MouseButtonDblClick &&
       watched_pointer == view.viewport()) {
      const auto &mouse_event =
          get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
      auto *const item_pointer = piano_roll_view.scene.itemAt(
          view.mapToScene(mouse_event.pos()), view.transform());
      if (item_pointer != nullptr) {
        const auto event_index_data = item_pointer->data(0);
        if (event_index_data.isValid() && note_double_clicked) {
          const auto &event =
              piano_roll_view.events.at(event_index_data.toInt());
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
        // selection-driven position in drag_playhead_to()
        if (piano_roll_view.playhead_active) {
          stop_playhead(*this);
        }
        piano_roll_view.playhead_dragging = true;
        const auto playhead_x = drag_playhead_to(piano_roll_view, mouse_event.pos());
        select_chord_at_playhead(*this, playhead_x / PIANO_ROLL_PIXELS_PER_MS);
        return true;
      }
    }
    if (event_pointer->type() == QEvent::MouseMove &&
       piano_roll_view.playhead_dragging && watched_pointer == view.viewport()) {
      const auto &mouse_event =
          get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
      const auto playhead_x = drag_playhead_to(piano_roll_view, mouse_event.pos());
      select_chord_at_playhead(*this, playhead_x / PIANO_ROLL_PIXELS_PER_MS);
      return true;
    }
    if (event_pointer->type() == QEvent::MouseButtonRelease &&
       piano_roll_view.playhead_dragging && watched_pointer == view.viewport()) {
      piano_roll_view.playhead_dragging = false;
      return true;
    }
    return QWidget::eventFilter(watched_pointer, event_pointer);
  }
};

// reapplies the highlight/cursor implied by the current selection_* fields
// against piano_roll_view's current note_items -- called both from
// update_piano_roll_widget_selection() and from the end of rebuild_scene(),
// since rebuilding replaces every QGraphicsRectItem (and thus wipes any
// highlight pen set on the old ones)
static void apply_selection_highlight(PianoRollWidget &widget) {
  auto &piano_roll_view = widget.piano_roll_view;
  const auto &events = piano_roll_view.events;

  const auto is_chord_selection =
      widget.selection_row_type == RowType::chord_type;
  const auto is_note_selection =
      widget.selection_row_type == RowType::pitched_note_type ||
      widget.selection_row_type == RowType::unpitched_note_type;

  // a chord-row selection highlights every note in the selected chords, a
  // note-row selection highlights only same-kind notes at those row
  // numbers within their one parent chord. Voice-row selections (and no
  // selection at all, encoded as selection_number_of_rows == 0) have no
  // timeline position and always highlight nothing.
  QList<bool> is_selected(static_cast<int>(events.size()), false);
  if (is_chord_selection || is_note_selection) {
    const auto kind_filter =
        widget.selection_row_type == RowType::pitched_note_type
            ? PianoRollNoteKind::pitched_kind
            : PianoRollNoteKind::unpitched_kind;
    for (auto event_index = 0; event_index < events.size();
        event_index = event_index + 1) {
      const auto &event = events.at(event_index);
      if (is_chord_selection) {
        if (event.chord_number >= widget.selection_first_row_number &&
           event.chord_number <
               widget.selection_first_row_number +
                   widget.selection_number_of_rows) {
          is_selected[event_index] = true;
        }
      } else if (event.chord_number == widget.selection_chord_number &&
                event.kind == kind_filter &&
                event.note_number >= widget.selection_first_row_number &&
                event.note_number <
                    widget.selection_first_row_number +
                        widget.selection_number_of_rows) {
        is_selected[event_index] = true;
      }
    }
  }

  // styles each note bar's pen based on is_selected (parallel to
  // events/note_items), leaving highlighted_bounds as the union of the
  // highlighted bars' scene bounds (a null rect if none are highlighted) so
  // it can be scrolled into view below
  QRectF highlighted_bounds;
  auto &note_items = piano_roll_view.note_items;
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

  const auto has_selection = (is_chord_selection || is_note_selection) &&
                             widget.selection_number_of_rows > 0;

  if (!has_selection) {
    if (!piano_roll_view.playhead_active) {
      piano_roll_view.playhead_item.hide();
    }
    return;
  }

  // playback already owns the cursor line while it's running, and so does
  // an in-progress manual drag (which set it more precisely than a chord's
  // start time); a selection change caused by select_chord_at_playhead()
  // itself shouldn't reposition the cursor either, since it's just
  // reporting where the cursor already is -- don't yank it away from any
  // of the three just because the table selection changed underneath it
  if (!piano_roll_view.playhead_active && !piano_roll_view.playhead_dragging &&
     !widget.selecting_chord_from_playhead) {
    const auto baseline_ms =
        get_piano_roll_time_bounds(
            widget.song_widget.song,
            is_chord_selection ? widget.selection_first_row_number
                               : widget.selection_chord_number,
            is_chord_selection ? widget.selection_number_of_rows : 1,
            is_chord_selection ? 0 : widget.selection_first_row_number,
            is_chord_selection ? -1 : widget.selection_number_of_rows,
            is_chord_selection
                ? std::nullopt
                : std::make_optional(
                      widget.selection_row_type == RowType::pitched_note_type
                          ? PianoRollNoteKind::pitched_kind
                          : PianoRollNoteKind::unpitched_kind))
            .first;
    piano_roll_view.playhead_item.show();
    position_playhead(piano_roll_view, baseline_ms, false);
  }
  if (!highlighted_bounds.isNull()) {
    piano_roll_view.view.ensureVisible(highlighted_bounds,
                       static_cast<int>(PIANO_ROLL_SCENE_MARGIN),
                       static_cast<int>(PIANO_ROLL_SCENE_MARGIN));
  }
}

// called whenever the playhead moves on its own -- from a manual drag
// (PianoRollWidget::eventFilter above) or a running playback timer tick
// (update_playhead_position() below) -- so the switch table's own
// selection follows the cursor back. Deliberately NOT called from
// position_playhead() itself, since that's also invoked reactively by
// apply_selection_highlight() after any table selection change (including
// a multi-row range); calling this from there would collapse that
// selection down to a single row the instant it was made. Only applies
// while the table is showing chords: a note or voice row has no single
// "current chord" to reselect into, and reselecting a chord there would
// kick the user out of whichever chord's notes/voices they're editing.
static void select_chord_at_playhead(PianoRollWidget &widget,
                                     const double time_ms) {
  auto &switch_table = widget.song_widget.switch_column.switch_table;
  if (switch_table.delegate.current_row_type != RowType::chord_type) {
    return;
  }
  // which chord's time range (as laid out by get_chord_start_times)
  // contains time_ms -- the last chord whose start is at or before
  // time_ms, or -1 if there are no chords yet or time_ms falls before the
  // first one
  const auto &chord_start_times = widget.piano_roll_view.chord_start_times;
  const auto first_later_iterator =
      std::ranges::upper_bound(chord_start_times, time_ms);
  const auto chord_number =
      static_cast<int>(first_later_iterator - chord_start_times.begin()) -
      1;
  if (chord_number < 0) {
    return;
  }
  auto &selection_model = get_selection_model(switch_table);
  const auto selected_rows = selection_model.selectedRows();
  if (selected_rows.size() == 1 && selected_rows.at(0).row() == chord_number) {
    return;
  }
  const auto chord_index = switch_table.chords_model.index(chord_number, 0);
  widget.selecting_chord_from_playhead = true;
  selection_model.select(chord_index, QItemSelectionModel::Select |
                                          QItemSelectionModel::Clear |
                                          QItemSelectionModel::Rows);
  widget.selecting_chord_from_playhead = false;
  switch_table.scrollTo(chord_index);
}

static void rebuild_scene(PianoRollWidget &widget) {
  const auto &song = widget.song_widget.song;

  // repopulates the notes view's scene with a fresh set of note bars + the
  // pitch/time axes for the current song
  {
    auto &notes_view = widget.piano_roll_view;
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
    const auto &notes_scene_rect = scene.sceneRect();
    view.setSceneRect(PIANO_ROLL_AXIS_X, notes_scene_rect.top(),
                      notes_scene_rect.right() - PIANO_ROLL_AXIS_X,
                      notes_scene_rect.height());
  }

  // lists every voice (pitched first, then unpitched) as a colored swatch +
  // name, in the same order used to assign global_voice_index for coloring,
  // then sizes legend_view to exactly fit its content (plus a margin), so
  // the fixed-width column stays as narrow as the longest voice name rather
  // than an arbitrary guessed width
  {
    auto &legend_view = widget.legend_view;
    auto &scene = legend_view.scene;
    auto &view = legend_view.view;

    scene.clear();
    auto row_y = 0.0;
    auto global_voice_index = 0;
    for (const auto &voice : song.pitched_voices) {
      draw_legend_row(scene, voice.name, global_voice_index, row_y);
      row_y = row_y + PIANO_ROLL_LANE_HEIGHT;
      global_voice_index = global_voice_index + 1;
    }
    for (const auto &voice : song.unpitched_voices) {
      draw_legend_row(scene, voice.name, global_voice_index, row_y);
      row_y = row_y + PIANO_ROLL_LANE_HEIGHT;
      global_voice_index = global_voice_index + 1;
    }

    const auto legend_bounds = scene.itemsBoundingRect().adjusted(
        -PIANO_ROLL_LEGEND_GAP, -PIANO_ROLL_LEGEND_GAP, PIANO_ROLL_LEGEND_GAP,
        PIANO_ROLL_LEGEND_GAP);
    scene.setSceneRect(legend_bounds);
    view.setFixedWidth(static_cast<int>(std::ceil(legend_bounds.width())) +
                       (2 * view.frameWidth()));
  }

  const auto &scene_rect = widget.piano_roll_view.scene.sceneRect();
  // only the pitch axis' ticks/labels have negative x, so the scene
  // rect's left edge is exactly the widest label's left edge; giving
  // axis_view that same rect (but overriding its own, view-local scene
  // rect rather than the shared QGraphicsScene's) as both its fixed
  // width and its scrollable area keeps it permanently framed on just
  // the axis column, with zero horizontal scroll range
  const auto axis_column_width = PIANO_ROLL_AXIS_X - scene_rect.left();
  auto &axis_view = widget.axis_view;
  axis_view.view.setFixedWidth(
      static_cast<int>(std::ceil(axis_column_width)) +
      (2 * axis_view.view.frameWidth()));
  axis_view.view.setSceneRect(scene_rect.left(), scene_rect.top(),
                              axis_column_width, scene_rect.height());

  // a short song (few voices, narrow pitch range) doesn't need nearly as
  // much vertical space as PIANO_ROLL_MIN_HEIGHT reserves -- letting the
  // dock grow well past the content just leaves blank gray space below the
  // last note. Capping it at the content's own height (plus the chrome
  // around it) means dragging the dock splitter taller stops being useful
  // once the whole piano roll is already on screen, rather than dragging
  // in dead space. Still floored at PIANO_ROLL_MIN_HEIGHT so an
  // empty/near-empty piano roll doesn't collapse to a sliver.
  auto &notes_view_view = widget.piano_roll_view.view;
  const auto chrome_height =
      (2 * notes_view_view.frameWidth()) +
      notes_view_view.horizontalScrollBar()->sizeHint().height() +
      widget.row_layout.contentsMargins().top() +
      widget.row_layout.contentsMargins().bottom();
  widget.setMaximumHeight(static_cast<int>(std::max(
      static_cast<double>(PIANO_ROLL_MIN_HEIGHT),
      std::ceil(scene_rect.height() + chrome_height))));

  apply_selection_highlight(widget);
}

static void zoom_in(PianoRollWidget &widget) {
  auto &piano_roll_view = widget.piano_roll_view;
  set_notes_view_time_zoom(
      piano_roll_view, piano_roll_view.time_zoom_factor * PIANO_ROLL_TIME_ZOOM_STEP);
}

static void zoom_out(PianoRollWidget &widget) {
  auto &piano_roll_view = widget.piano_roll_view;
  set_notes_view_time_zoom(
      piano_roll_view, piano_roll_view.time_zoom_factor / PIANO_ROLL_TIME_ZOOM_STEP);
}

static void set_time_zoom(PianoRollWidget &widget,
                          const double new_zoom_factor) {
  set_notes_view_time_zoom(widget.piano_roll_view, new_zoom_factor);
}

// called by SongEditor whenever the switch table's selection changes, so
// the piano roll can mirror it: highlight the corresponding note bar(s),
// jump the cursor to the selection's start, and scroll to keep both in
// view. number_of_rows == 0 clears the highlight and hides the cursor
// (used both for "nothing selected" and for voice-row selections, which
// have no timeline position).
static void update_piano_roll_widget_selection(PianoRollWidget &widget,
                                               const RowType row_type,
                                               const int chord_number,
                                               const int first_row_number,
                                               const int number_of_rows) {
  widget.selection_row_type = row_type;
  widget.selection_chord_number = chord_number;
  widget.selection_first_row_number = first_row_number;
  widget.selection_number_of_rows = number_of_rows;
  apply_selection_highlight(widget);
}

// position_playhead() recenters the view every tick while playing, fighting
// any manual scroll (drag on the scrollbar, or wheel) the user does at the
// same time -- the two writes to the same scroll position within one 33ms
// tick used to leave rendering artifacts behind that read as extra, stuck
// red cursor lines. Disabling manual scrolling during playback removes the
// conflicting writer entirely.
static void set_manual_scrolling_enabled(PianoRollWidget &widget,
                                         const bool enabled) {
  widget.piano_roll_view.view.horizontalScrollBar()->setEnabled(enabled);
  set_vertical_scrolling_enabled(widget.piano_roll_view.view, enabled);
  set_vertical_scrolling_enabled(widget.axis_view.view, enabled);
}

static void start_playhead(PianoRollWidget &widget, const double baseline_ms,
                          const double end_ms) {
  set_manual_scrolling_enabled(widget, false);

  auto &piano_roll_view = widget.piano_roll_view;
  piano_roll_view.playhead_baseline_ms = baseline_ms;
  piano_roll_view.playhead_end_ms = end_ms;
  piano_roll_view.playhead_elapsed_timer.restart();
  piano_roll_view.playhead_active = true;
  piano_roll_view.playhead_item.show();

  // decides which transition position_playhead() should run, based on
  // where the playhead is starting relative to the view's current
  // (not-yet-moved) center -- see PlayheadTransition
  auto &view = piano_roll_view.view;
  const auto initial_center_x =
      view.mapToScene(view.viewport()->rect()).boundingRect().center().x();
  const auto playhead_x = baseline_ms * PIANO_ROLL_PIXELS_PER_MS;
  if (playhead_x <= initial_center_x) {
    piano_roll_view.playhead_transition = PlayheadTransition::waiting_to_reach_center;
  } else {
    piano_roll_view.playhead_transition = PlayheadTransition::catching_up;
    piano_roll_view.playhead_catchup_start_center_x = initial_center_x;
  }

  position_playhead(piano_roll_view, baseline_ms);
  piano_roll_view.playhead_timer.start(PIANO_ROLL_TIMER_INTERVAL_MS);
}

static void stop_playhead(PianoRollWidget &widget) {
  auto &piano_roll_view = widget.piano_roll_view;
  piano_roll_view.playhead_timer.stop();
  piano_roll_view.playhead_active = false;
  piano_roll_view.playhead_transition = PlayheadTransition::none;

  set_manual_scrolling_enabled(widget, true);
  apply_selection_highlight(widget);
}

static void update_playhead_position(PianoRollWidget &widget) {
  auto &piano_roll_view = widget.piano_roll_view;
  if (!piano_roll_view.playhead_active) {
    return;
  }
  const auto current_ms =
      piano_roll_view.playhead_baseline_ms +
      static_cast<double>(piano_roll_view.playhead_elapsed_timer.elapsed());
  if (current_ms >= piano_roll_view.playhead_end_ms) {
    piano_roll_view.playhead_active = false;
    piano_roll_view.playhead_timer.stop();
    set_manual_scrolling_enabled(widget, true);
    position_playhead(piano_roll_view, piano_roll_view.playhead_end_ms);
    select_chord_at_playhead(widget, piano_roll_view.playhead_end_ms);
    return;
  }
  position_playhead(piano_roll_view, current_ms);
  select_chord_at_playhead(widget, current_ms);
}
