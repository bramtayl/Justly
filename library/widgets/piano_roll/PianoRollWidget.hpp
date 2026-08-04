#pragma once

#include <QtCore/QEvent>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/Qt>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <utility>

#include "other/PianoRoll.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/RowType.hpp"
#include "sound/PlayState.hpp"
#include "widgets/piano_roll/PianoRollAxisView.hpp"
#include "widgets/piano_roll/PianoRollLegendView.hpp"
#include "widgets/piano_roll/PianoRollNotesView.hpp"
#include "widgets/piano_roll/piano_roll_helpers.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"

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
  // (update_piano_roll_selection() -> update_selection() ->
  // apply_selection_highlight()); without this guard, apply_selection_highlight()
  // would treat the sync as an ordinary table-driven selection change and
  // reposition the cursor to the newly-selected chord's start, snapping it
  // backwards away from wherever the drag/playback actually put it
  bool selecting_chord_from_playhead = false;

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
                     [this]() -> auto { update_playhead_position(); });

    // the view has no interactivity of its own (no item selection, no
    // custom QGraphicsView subclass), so double-clicks and ctrl+wheel zoom
    // are picked up via an event filter on the viewport rather than
    // overriding QGraphicsView
    piano_roll_view.view.viewport()->installEventFilter(this);

    rebuild_scene();
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
        // selection-driven position in PianoRollNotesView::drag_playhead_to()
        if (piano_roll_view.playhead_active) {
          stop_playhead();
        }
        piano_roll_view.playhead_dragging = true;
        const auto playhead_x = piano_roll_view.drag_playhead_to(mouse_event.pos());
        select_chord_at_playhead(playhead_x / PIANO_ROLL_PIXELS_PER_MS);
        return true;
      }
    }
    if (event_pointer->type() == QEvent::MouseMove &&
       piano_roll_view.playhead_dragging && watched_pointer == view.viewport()) {
      const auto &mouse_event =
          get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
      const auto playhead_x = piano_roll_view.drag_playhead_to(mouse_event.pos());
      select_chord_at_playhead(playhead_x / PIANO_ROLL_PIXELS_PER_MS);
      return true;
    }
    if (event_pointer->type() == QEvent::MouseButtonRelease &&
       piano_roll_view.playhead_dragging && watched_pointer == view.viewport()) {
      piano_roll_view.playhead_dragging = false;
      return true;
    }
    return QWidget::eventFilter(watched_pointer, event_pointer);
  }

  // called whenever the playhead moves on its own -- from a manual drag
  // (the eventFilter above) or a running playback timer tick
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
    // which chord's time range (as laid out by get_chord_start_times)
    // contains time_ms -- the last chord whose start is at or before
    // time_ms, or -1 if there are no chords yet or time_ms falls before the
    // first one
    const auto &chord_start_times = piano_roll_view.chord_start_times;
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
    selecting_chord_from_playhead = true;
    selection_model.select(chord_index, QItemSelectionModel::Select |
                                            QItemSelectionModel::Clear |
                                            QItemSelectionModel::Rows);
    selecting_chord_from_playhead = false;
    switch_table.scrollTo(chord_index);
  }

  void rebuild_scene() {
    const auto &song = song_widget.song;
    piano_roll_view.rebuild(song);
    legend_view.rebuild(song.pitched_voices, song.unpitched_voices);

    const auto &scene_rect = piano_roll_view.scene.sceneRect();
    // only the pitch axis' ticks/labels have negative x, so the scene
    // rect's left edge is exactly the widest label's left edge; giving
    // axis_view that same rect (but overriding its own, view-local scene
    // rect rather than the shared QGraphicsScene's) as both its fixed
    // width and its scrollable area keeps it permanently framed on just
    // the axis column, with zero horizontal scroll range
    const auto axis_column_width = PIANO_ROLL_AXIS_X - scene_rect.left();
    axis_view.view.setFixedWidth(
        static_cast<int>(std::ceil(axis_column_width)) +
        (2 * axis_view.view.frameWidth()));
    axis_view.view.setSceneRect(scene_rect.left(), scene_rect.top(),
                                axis_column_width, scene_rect.height());

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
    auto &view = piano_roll_view.view;
    const auto chrome_height =
        (2 * view.frameWidth()) + view.horizontalScrollBar()->sizeHint().height() +
        row_layout.contentsMargins().top() +
        row_layout.contentsMargins().bottom();
    setMaximumHeight(static_cast<int>(std::max(
        static_cast<double>(PIANO_ROLL_MIN_HEIGHT),
        std::ceil(content_height + chrome_height))));
  }

  void zoom_in() { piano_roll_view.zoom_in(); }

  void zoom_out() { piano_roll_view.zoom_out(); }

  void set_time_zoom(const double new_zoom_factor) {
    piano_roll_view.set_time_zoom(new_zoom_factor);
  }

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
  // against piano_roll_view's current note_items -- called both from
  // update_selection() and from the end of rebuild_scene(), since rebuilding
  // replaces every QGraphicsRectItem (and thus wipes any highlight pen set
  // on the old ones)
  void apply_selection_highlight() {
    const auto &events = piano_roll_view.events;

    const auto is_chord_selection = selection_row_type == RowType::chord_type;
    const auto is_note_selection =
        selection_row_type == RowType::pitched_note_type ||
        selection_row_type == RowType::unpitched_note_type;

    // a chord-row selection highlights every note in the selected chords, a
    // note-row selection highlights only same-kind notes at those row
    // numbers within their one parent chord. Voice-row selections (and no
    // selection at all, encoded as selection_number_of_rows == 0) have no
    // timeline position and always highlight nothing.
    QList<bool> is_selected(static_cast<int>(events.size()), false);
    if (is_chord_selection || is_note_selection) {
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
            is_selected[event_index] = true;
          }
        } else if (event.chord_number == selection_chord_number &&
                  event.kind == kind_filter &&
                  event.note_number >= selection_first_row_number &&
                  event.note_number <
                      selection_first_row_number + selection_number_of_rows) {
          is_selected[event_index] = true;
        }
      }
    }

    const auto highlighted_bounds = piano_roll_view.apply_highlight(is_selected);

    const auto has_selection = (is_chord_selection || is_note_selection) &&
                               selection_number_of_rows > 0;

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
      piano_roll_view.playhead_item.show();
      piano_roll_view.position_playhead(baseline_ms, false);
    }
    if (!highlighted_bounds.isNull()) {
      piano_roll_view.view.ensureVisible(highlighted_bounds,
                         static_cast<int>(PIANO_ROLL_SCENE_MARGIN),
                         static_cast<int>(PIANO_ROLL_SCENE_MARGIN));
    }
  }

  // follow_playhead() calls ensureVisible() every tick while playing,
  // fighting any manual scroll (drag on the scrollbar, or wheel) the user
  // does at the same time -- the two writes to the same scroll position
  // within one 33ms tick used to leave rendering artifacts behind that read
  // as extra, stuck red cursor lines. Disabling manual scrolling during
  // playback removes the conflicting writer entirely.
  void set_manual_scrolling_enabled(const bool enabled) {
    piano_roll_view.set_scrolling_enabled(enabled);
    axis_view.set_scrolling_enabled(enabled);
  }

  void start_playhead(const double baseline_ms, const double end_ms) {
    set_manual_scrolling_enabled(false);
    piano_roll_view.start_playhead(baseline_ms, end_ms);
  }

  void stop_playhead() {
    piano_roll_view.stop_playhead();
    set_manual_scrolling_enabled(true);
    apply_selection_highlight();
  }

  void update_playhead_position() {
    if (!piano_roll_view.playhead_active) {
      return;
    }
    const auto current_ms =
        piano_roll_view.playhead_baseline_ms +
        static_cast<double>(piano_roll_view.playhead_elapsed_timer.elapsed());
    if (current_ms >= piano_roll_view.playhead_end_ms) {
      piano_roll_view.playhead_active = false;
      piano_roll_view.playhead_timer.stop();
      set_manual_scrolling_enabled(true);
      piano_roll_view.position_playhead(piano_roll_view.playhead_end_ms);
      select_chord_at_playhead(piano_roll_view.playhead_end_ms);
      return;
    }
    piano_roll_view.position_playhead(current_ms);
    select_chord_at_playhead(current_ms);
  }
};
