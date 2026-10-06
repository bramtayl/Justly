#pragma once

#include <QtWidgets/QWidget>

#include "other/TableSelection.hpp"

struct PianoRollColumnScene;
struct PianoRollNotesScene;
class QBoxLayout;
struct Song;
struct WindowBody;

static const auto PIANO_ROLL_TIME_ZOOM_STEP = 1.25;

// the start of the first chord, and the latest end of any of their notes,
// for [first_chord_number, first_chord_number + number_of_chords)
[[nodiscard]] auto get_chords_time_bounds(const Song& song,
                                          int first_chord_number,
                                          int number_of_chords)
    -> std::pair<double, double>;

// get_chords_time_bounds for a chord selection, or, for a note selection,
// the start of its chord and the latest end of the selected notes; voice
// selections have no timeline position, so callers must rule them out
[[nodiscard]] auto get_selection_time_bounds(const Song& song,
                                             const TableSelection& selection)
    -> std::pair<double, double>;

struct PianoRollWidget : public QWidget {
  Q_OBJECT

 public:
  const WindowBody& window_body;

  PianoRollNotesScene& piano_roll_scene;
  // the pitch axis' ticks/labels, pinned to the left edge; its vertical
  // scroll is kept in lockstep with piano_roll_scene's, and both scenes place
  // items using the same y = -midi * PIANO_ROLL_PIXELS_PER_SEMITONE formula,
  // so the pitch labels stay lined up with their notes no matter how far
  // either view is scrolled vertically
  PianoRollColumnScene& axis_scene;
  // the voice legend, pinned to the right edge; left free to scroll
  // vertically on its own so a long voice list stays reachable
  PianoRollColumnScene& legend_scene;

  QBoxLayout& row_layout;

  // true for the duration of select_chord_at_playhead()'s own call to
  // QItemSelectionModel::select() -- that select() re-enters this widget
  // synchronously via ReplaceTable's selectionChanged connection
  // (update_piano_roll_selection() -> set_piano_roll_selection() ->
  // apply_selection_highlight()); without this guard,
  // apply_selection_highlight() would treat the sync as an ordinary
  // table-driven selection change and reposition the cursor to the
  // newly-selected chord's start, snapping it backwards away from wherever the
  // drag/playback actually put it
  bool selecting_chord_from_playhead = false;

  // the switch table's current selection, mirrored here by ReplaceTable.cpp
  // (via set_piano_roll_selection()) every time it changes, so
  // rebuild_piano_roll_scene() can reapply the same highlight/cursor after
  // redrawing a fresh set of items. Starts out empty
  TableSelection selection;

  // the chord under the cursor when the current playhead drag started (-1
  // when not dragging), so MouseMove can select the whole range of chords
  // spanned between there and wherever the drag is now, rather than just
  // re-targeting a single chord to the latest position and losing everything
  // dragged over in between
  int drag_start_chord_number = -1;

  explicit PianoRollWidget(const WindowBody& window_body_input);

  auto eventFilter(QObject* watched_pointer, QEvent* event_pointer)
      -> bool override;

 signals:
  // emitted when a note in the piano roll is double-clicked; MainWindow
  // connects to this to open the pitched/unpitched notes table for the
  // note's chord, scrolled to and highlighting that note
  void note_double_clicked(int chord_number, int note_number, bool is_pitched);
};

// redraws every scene from window_body's song, e.g. after any undoable
// change, then reapplies the current selection's highlight
void rebuild_piano_roll_scene(PianoRollWidget& widget);

// mirrors the switch table's selection onto the piano roll: highlights the
// corresponding note bar(s), jumps the cursor to the selection's start, and
// scrolls to keep both in view. number_of_rows == 0 clears the highlight and
// hides the cursor (used both for "nothing selected" and for voice-row
// selections, which have no timeline position)
void set_piano_roll_selection(PianoRollWidget& widget,
                              const TableSelection& selection);

void zoom_in_piano_roll(PianoRollWidget& widget);

void zoom_out_piano_roll(PianoRollWidget& widget);

void start_piano_roll_playhead(PianoRollWidget& widget, double baseline_ms,
                               double end_ms);

void stop_piano_roll_playhead(PianoRollWidget& widget);

// one playback timer tick: moves the playhead to the elapsed time, and the
// switch table's chord selection along with it
void update_playhead_position(PianoRollWidget& widget);
