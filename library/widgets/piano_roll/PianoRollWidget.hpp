#pragma once

#include <QtWidgets/QWidget>

#include "rows/RowType.hpp"

struct PianoRollAxisScene;
struct PianoRollLegendScene;
struct PianoRollNotesScene;
class QBoxLayout;
struct Song;
struct SongWidget;
struct SwitchTable;

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
[[nodiscard]] auto to_scene_x(const PianoRollNotesScene& notes_scene,
                              double time_ms) -> double;

// sets notes_scene's horizontal scale directly (rather than accumulating
// via QGraphicsView::scale()) so repeated zoom_in()/zoom_out() calls can't
// drift and clamping is just one std::clamp on the absolute factor; the
// vertical scale is always left at 1, so the pitch axis (and
// PianoRollAxisScene, which is never zoomed) stays visually fixed while
// only the time axis expands/contracts
void set_notes_view_time_zoom(PianoRollNotesScene& notes_scene,
                              double new_zoom_factor);

// follow_view lets a caller move the playhead line without recentering the
// view on it -- used when playback has already stopped (see
// PianoRollWidget::apply_selection_highlight()), where forcibly
// recentering would yank the view away from wherever the user had it
// scrolled
void position_playhead(PianoRollNotesScene& notes_scene, double time_ms,
                       bool follow_view = true);

// number_of_notes == -1 (default) means "every note in every chord in
// [first_chord_number, first_chord_number + number_of_chords)". A concrete
// number_of_notes restricts to a single chord's note list (number_of_chords
// should be 1 in that case), matching how the Play menu can select either a
// range of chords or a range of notes within one chord.
[[nodiscard]] auto get_piano_roll_time_bounds(
    const Song& song, int first_chord_number, int number_of_chords,
    int first_note_number = 0, int number_of_notes = -1,
    std::optional<bool> pitched_filter = std::nullopt)
    -> std::pair<double, double>;

void zoom_in(PianoRollNotesScene& piano_roll_scene);

void zoom_out(PianoRollNotesScene& piano_roll_scene);

// position_playhead() recenters the view every tick while playing, fighting
// any manual scroll (drag on the scrollbar, or wheel) the user does at the
// same time -- the two writes to the same scroll position within one 33ms
// tick used to leave rendering artifacts behind that read as extra, stuck
// red cursor lines. Disabling manual scrolling during playback removes the
// conflicting writer entirely.
void set_manual_scrolling_enabled(PianoRollNotesScene& piano_roll_scene,
                                  PianoRollAxisScene& axis_scene, bool enabled);

// reapplies the highlight/cursor implied by the current selection_* fields
// against piano_roll_scene's current note_items -- called both from
// update_piano_roll_widget_selection() and from the end of rebuild_scene(),
// since rebuilding replaces every QGraphicsRectItem (and thus wipes any
// highlight pen set on the old ones)
void apply_selection_highlight(const Song& song,
                               PianoRollNotesScene& piano_roll_scene,
                               RowType selection_row_type,
                               int selection_chord_number,
                               int selection_first_row_number,
                               int selection_number_of_rows,
                               bool selecting_chord_from_playhead);

void rebuild_scene(QWidget& widget, const SongWidget& song_widget,
                   PianoRollNotesScene& piano_roll_scene,
                   PianoRollAxisScene& axis_scene,
                   PianoRollLegendScene& legend_scene, QBoxLayout& row_layout,
                   RowType selection_row_type, int selection_chord_number,
                   int selection_first_row_number, int selection_number_of_rows,
                   bool selecting_chord_from_playhead);

void stop_playhead(PianoRollNotesScene& piano_roll_scene,
                   PianoRollAxisScene& axis_scene, const Song& song,
                   RowType selection_row_type, int selection_chord_number,
                   int selection_first_row_number, int selection_number_of_rows,
                   bool selecting_chord_from_playhead);

void update_playhead_position(PianoRollNotesScene& piano_roll_scene,
                              PianoRollAxisScene& axis_scene,
                              SwitchTable& switch_table,
                              bool& selecting_chord_from_playhead);

struct PianoRollWidget : public QWidget {
  Q_OBJECT

 public:
  const SongWidget& song_widget;

  PianoRollNotesScene& piano_roll_scene;
  // a second, fixed-width view pinned to the left edge, showing the pitch
  // axis -- see PianoRollAxisScene for details
  PianoRollAxisScene& axis_scene;
  // a separate scene/view for the voice legend, pinned to the right edge --
  // see PianoRollLegendScene for details
  PianoRollLegendScene& legend_scene;

  QBoxLayout& row_layout;

  // true for the duration of select_chord_at_playhead()'s own call to
  // QItemSelectionModel::select() -- that select() re-enters this widget
  // synchronously via ReplaceTable's selectionChanged connection
  // (update_piano_roll_selection() -> update_piano_roll_widget_selection() ->
  // apply_selection_highlight()); without this guard,
  // apply_selection_highlight() would treat the sync as an ordinary
  // table-driven selection change and reposition the cursor to the
  // newly-selected chord's start, snapping it backwards away from wherever the
  // drag/playback actually put it
  bool selecting_chord_from_playhead = false;

  // the switch table's current selection, mirrored here by ReplaceTable.hpp
  // (via update_piano_roll_widget_selection()) every time it changes, so
  // rebuild_scene() can reapply the same highlight/cursor after redrawing a
  // fresh set of items. number_of_rows == 0 means nothing is selected (the
  // default at startup)
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

  explicit PianoRollWidget(const SongWidget& song_widget_input);

  auto eventFilter(QObject* watched_pointer, QEvent* event_pointer)
      -> bool override;

 signals:
  // emitted when a note in the piano roll is double-clicked; SongEditor
  // connects to this to open the pitched/unpitched notes table for the
  // note's chord, scrolled to and highlighting that note
  void note_double_clicked(int chord_number, int note_number, bool is_pitched);
};
