#pragma once

#include <QtCore/QChar>
#include <QtCore/QEasingCurve>
#include <QtCore/QEvent>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <utility>

#include "other/PianoRollNoteEvent.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/RowType.hpp"
#include "sound/PlayState.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/piano_roll/PianoRollAxisScene.hpp"
#include "widgets/piano_roll/PianoRollLegendScene.hpp"
#include "widgets/piano_roll/PianoRollNotesScene.hpp"
#include "widgets/piano_roll/PlayheadTransition.hpp"

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
static const auto PIANO_ROLL_UNPITCHED_LANE_GAP = 30.0;
static const auto PIANO_ROLL_LEGEND_GAP = 10.0;
static const auto PIANO_ROLL_HIGHLIGHT_PEN_WIDTH = 1.5;
static const auto PIANO_ROLL_TIME_ZOOM_STEP = 1.25;

// the functions below act on PianoRollNotesScene/PianoRollLegendScene but
// live here rather than in those structs' own headers, because every
// caller of theirs is in this file (PianoRollWidget's own methods below) --
// keeping them in PianoRollNotesScene.hpp/PianoRollLegendScene.hpp would
// leave them looking unused whenever one of those headers is compiled on
// its own (e.g. by clangd, which has no way to see this file's calls when
// it treats a header opened directly as its own translation unit)

// converts an absolute song time (ms) to this scene's x coordinate --
// identity-scaled by time_axis_baseline_ms, which is 0 outside notes mode
// (see the field's comment above) so this is a no-op there
[[nodiscard]] auto to_scene_x(const PianoRollNotesScene &notes_scene,
                              const double time_ms) -> double;

// (re)draws the time axis' ticks and labels, spaced (in ms) so they land
// roughly PIANO_ROLL_TARGET_TICK_PIXEL_SPACING apart on screen at the
// current time_zoom_factor -- called from PianoRollWidget::rebuild_scene()
// for the initial build and from set_time_zoom() whenever the zoom changes,
// since a spacing that looked right before a zoom change would otherwise
// crowd together (zooming in) or spread too far apart (zooming out)
void redraw_time_axis_ticks(PianoRollNotesScene &notes_scene);

// sets notes_scene's horizontal scale directly (rather than accumulating
// via QGraphicsView::scale()) so repeated zoom_in()/zoom_out() calls can't
// drift and clamping is just one std::clamp on the absolute factor; the
// vertical scale is always left at 1, so the pitch axis (and
// PianoRollAxisScene, which is never zoomed) stays visually fixed while
// only the time axis expands/contracts
void set_notes_view_time_zoom(PianoRollNotesScene &notes_scene,
                              const double new_zoom_factor);

// moves the playhead line to the time under the given viewport position,
// without recentering the view on it the way position_playhead() does for
// playback -- while the user is actively dragging, the view should hold
// still under the mouse rather than fight the drag by scrolling; returns
// the playhead's new x position (in scene coordinates) so the caller can
// translate it back to a time and sync the switch table's selection
[[nodiscard]] auto drag_playhead_to(PianoRollNotesScene &notes_scene,
                                    const QPoint &viewport_pos)
    -> double;

// resizes/shows the shaded selection box to span from start_x to end_x (in
// either order), full scene height -- called from
// PianoRollWidget::apply_selection_highlight() to mirror whatever chord/note
// range is currently selected, so it stays put (rather than needing a
// mouse-driven show/hide of its own) whether that range came from a piano
// roll drag, a table click, or playback
void show_selection_rect(PianoRollNotesScene &notes_scene,
                         const double start_x, const double end_x);

void hide_selection_rect(PianoRollNotesScene &notes_scene);

// follow_view lets a caller move the playhead line without recentering the
// view on it -- used when playback has already stopped (see
// PianoRollWidget::apply_selection_highlight()), where forcibly
// recentering would yank the view away from wherever the user had it
// scrolled
void position_playhead(PianoRollNotesScene &notes_scene,
                       const double time_ms,
                       const bool follow_view = true);

[[nodiscard]] auto get_voice_color(const int global_voice_index)
    -> QColor;

void draw_legend_row(QGraphicsScene &legend_scene, const QString &name,
                     const int global_voice_index, const double row_y);

// number_of_notes == -1 (default) means "every note in every chord in
// [first_chord_number, first_chord_number + number_of_chords)". A concrete
// number_of_notes restricts to a single chord's note list (number_of_chords
// should be 1 in that case), matching how the Play menu can select either a
// range of chords or a range of notes within one chord.
[[nodiscard]] auto get_piano_roll_time_bounds(
    const Song &song, const int first_chord_number,
    const int number_of_chords, const int first_note_number = 0,
    const int number_of_notes = -1,
    const std::optional<bool> pitched_filter = std::nullopt)
    -> std::pair<double, double>;

// the functions below take the specific PianoRollWidget fields they act on
// rather than the whole widget -- that's what lets them be fully defined up
// here, ahead of PianoRollWidget itself, so its own inline constructor/
// eventFilter bodies (further down) can call them without a forward
// declaration. PianoRollWidget-taking overloads of the ones also needed by
// outside callers (SongEditor, tests) follow after the struct.

// which chord's time range (as laid out in chord_start_times) contains
// time_ms -- the last chord whose start is at or before time_ms, or -1 if
// there are no chords yet or time_ms falls before the first one
[[nodiscard]] auto get_chord_number_at_time(
    const QList<double> &chord_start_times, const double time_ms) -> int;

// prefers whichever note bar is actually drawn under viewport_pos (the same
// itemAt() lookup the double-click handler uses) over pure time-based lookup
// -- a note's duration can run longer than its own chord (i.e. past the next
// chord's start_time), so its bar visually extends into the next chord's
// time range; without this, clicking that overhang would fall through to
// get_chord_number_at_time() and select the next chord instead of the one
// the visible bar actually belongs to. Falls back to get_chord_number_at_time()
// when the click doesn't land on any note bar (e.g. empty space), matching
// drag_playhead_to()'s own clamp of negative x to the axis
[[nodiscard]] auto get_chord_number_at_viewport_pos(
    const PianoRollNotesScene &piano_roll_scene, const QPoint &viewport_pos)
    -> int;

// called whenever the playhead moves on its own -- from a manual drag
// (PianoRollWidget::eventFilter) or a running playback timer tick
// (update_playhead_position() below) -- so the switch table's own
// selection follows the cursor back. Deliberately NOT called from
// position_playhead() itself, since that's also invoked reactively by
// apply_selection_highlight() after any table selection change (including
// a multi-row range); calling this from there would collapse that
// selection down to a single row the instant it was made. Only applies
// while the table is showing chords: a note or voice row has no single
// "current chord" to reselect into, and reselecting a chord there would
// kick the user out of whichever chord's notes/voices they're editing.
void select_chord_at_playhead(SwitchTable &switch_table,
                              const QList<double> &chord_start_times,
                              bool &selecting_chord_from_playhead,
                              const double time_ms);

// selects the note row (in whichever of pitched_notes_model /
// unpitched_notes_model matches the clicked bar's own kind) corresponding to
// a note bar clicked directly in the piano roll -- the notes-mode
// counterpart to select_chord_at_playhead/select_chord_range_at_playhead,
// which only act while the table is in chord mode. A no-op unless the
// switch table is already showing that exact chord's notes of that exact
// kind (e.g. clicking a pitched note's bar while the table shows unpitched
// notes, or another chord's notes, has no row to select).
void select_note_at_bar(SwitchTable &switch_table,
                        const PianoRollNoteEvent &event);

// selects every chord between anchor_chord_number and current_chord_number
// (inclusive, in either order) as one contiguous range -- the drag-select
// counterpart to select_chord_at_playhead's single-chord reselect, used
// while a playhead drag is in progress so notes dragged over in between the
// press and the current mouse position stay selected instead of being
// dropped in favor of whichever chord is under the cursor right now
void select_chord_range_at_playhead(SwitchTable &switch_table,
                                    const int number_of_chords,
                                    bool &selecting_chord_from_playhead,
                                    const int anchor_chord_number,
                                    const int current_chord_number);

void zoom_in(PianoRollNotesScene &piano_roll_scene);

void zoom_out(PianoRollNotesScene &piano_roll_scene);

void set_vertical_scrolling_enabled(QGraphicsView &view,
                                    const bool enabled);

// position_playhead() recenters the view every tick while playing, fighting
// any manual scroll (drag on the scrollbar, or wheel) the user does at the
// same time -- the two writes to the same scroll position within one 33ms
// tick used to leave rendering artifacts behind that read as extra, stuck
// red cursor lines. Disabling manual scrolling during playback removes the
// conflicting writer entirely.
void set_manual_scrolling_enabled(PianoRollNotesScene &piano_roll_scene,
                                  PianoRollAxisScene &axis_scene,
                                  const bool enabled);

// reapplies the highlight/cursor implied by the current selection_* fields
// against piano_roll_scene's current note_items -- called both from
// update_piano_roll_widget_selection() and from the end of rebuild_scene(),
// since rebuilding replaces every QGraphicsRectItem (and thus wipes any
// highlight pen set on the old ones)
void apply_selection_highlight(
    const Song &song, PianoRollNotesScene &piano_roll_scene,
    const RowType selection_row_type, const int selection_chord_number,
    const int selection_first_row_number, const int selection_number_of_rows,
    const bool selecting_chord_from_playhead);

void rebuild_scene(QWidget &widget, const SongWidget &song_widget,
                   PianoRollNotesScene &piano_roll_scene,
                   PianoRollAxisScene &axis_scene,
                   PianoRollLegendScene &legend_scene,
                   QBoxLayout &row_layout,
                   const RowType selection_row_type,
                   const int selection_chord_number,
                   const int selection_first_row_number,
                   const int selection_number_of_rows,
                   const bool selecting_chord_from_playhead);

void stop_playhead(PianoRollNotesScene &piano_roll_scene,
                   PianoRollAxisScene &axis_scene, const Song &song,
                   const RowType selection_row_type,
                   const int selection_chord_number,
                   const int selection_first_row_number,
                   const int selection_number_of_rows,
                   const bool selecting_chord_from_playhead);

void update_playhead_position(PianoRollNotesScene &piano_roll_scene,
                              PianoRollAxisScene &axis_scene,
                              SwitchTable &switch_table,
                              bool &selecting_chord_from_playhead);

struct PianoRollWidget : public QWidget {
  const SongWidget &song_widget;

  PianoRollNotesScene &piano_roll_scene = *(new PianoRollNotesScene(*this));
  // a second, fixed-width view pinned to the left edge, showing the pitch
  // axis -- see PianoRollAxisScene for details
  PianoRollAxisScene &axis_scene = *(new PianoRollAxisScene(*this));
  // a separate scene/view for the voice legend, pinned to the right edge --
  // see PianoRollLegendScene for details
  PianoRollLegendScene &legend_scene = *(new PianoRollLegendScene(*this));

  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  // true for the duration of select_chord_at_playhead()'s own call to
  // QItemSelectionModel::select() -- that select() re-enters this widget
  // synchronously via ReplaceTable's selectionChanged connection
  // (update_piano_roll_selection() -> update_piano_roll_widget_selection() ->
  // apply_selection_highlight()); without this guard, apply_selection_highlight()
  // would treat the sync as an ordinary table-driven selection change and
  // reposition the cursor to the newly-selected chord's start, snapping it
  // backwards away from wherever the drag/playback actually put it
  bool selecting_chord_from_playhead = false;

  // the switch table's current selection, mirrored here by ReplaceTable.hpp
  // (via update_piano_roll_widget_selection()) every time it changes, so rebuild_scene() can
  // reapply the same highlight/cursor after redrawing a fresh set of items.
  // number_of_rows == 0 means nothing is selected (the default at startup)
  RowType selection_row_type = RowType::chord_type;
  int selection_chord_number = -1;
  int selection_first_row_number = -1;
  int selection_number_of_rows = 0;

  // the chord under the cursor when the current playhead drag started (-1
  // when not dragging), so MouseMove can select the whole range of chords
  // spanned between there and wherever the drag is now, rather than just
  // re-targeting a single chord to the latest position and losing everything
  // dragged over in between
  int drag_start_chord_number = -1;

  // set from outside (SongEditor) once it has access to the song menu bar
  // and song widget needed to switch tables; left empty in contexts (e.g.
  // tests) that never wire it up
  std::function<void(int chord_number, int note_number, bool is_pitched)>
      note_double_clicked;

  explicit PianoRollWidget(const SongWidget &song_widget_input);

  auto eventFilter(QObject *watched_pointer, QEvent *event_pointer)
      -> bool override;
};
