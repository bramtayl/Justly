#pragma once

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsScene>

#include "other/PianoRollNoteEvent.hpp"
#include "widgets/piano_roll/PlayheadTransition.hpp"

class QGraphicsView;
class QTimer;
class QWidget;

static const auto PIANO_ROLL_PIXELS_PER_MS = 0.1;
static const auto PIANO_ROLL_DEFAULT_AXIS_Y = 0.0;
static const auto PIANO_ROLL_MIN_TIME_ZOOM = 0.25;
static const auto PIANO_ROLL_MAX_TIME_ZOOM = 8.0;

// the main scrollable graphics view: the note bars, the pitch/time axes,
// and the playhead cursor + its playback animation all live here
//
// is-a QGraphicsScene (see PianoRollAxisScene's comment for why) so it can be
// heap-allocated and parented to parent_widget directly
struct PianoRollNotesScene : public QGraphicsScene {
  QGraphicsView& view;
  QGraphicsLineItem& playhead_item = *(new QGraphicsLineItem);
  // the shaded box drawn behind the notes over the current selection's
  // timeline extent -- purely visual feedback for whatever the switch
  // table's own selection already is (see
  // PianoRollWidget::apply_selection_highlight()), so it never itself drives
  // selection and stays visible for as long as that selection does,
  // including after a drag's mouse release
  QGraphicsRectItem& selection_rect_item = *(new QGraphicsRectItem);

  QTimer& playhead_timer;
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
  // PianoRollAxisScene, which is never zoomed) while the time axis
  // expands/contracts
  double time_zoom_factor = 1.0;

  // the inputs redraw_time_axis_ticks() needs to redraw just the time
  // axis' ticks and labels whenever the zoom changes, without re-running
  // the full PianoRollWidget::rebuild_scene()
  double time_axis_max_time_ms = 0.0;
  double time_axis_y = PIANO_ROLL_DEFAULT_AXIS_Y;
  // the absolute song time (ms) that maps to this view's x == PIANO_ROLL_AXIS_X
  // -- 0 normally, but in notes mode (PianoRollWidget::rebuild_scene() scoped
  // to one chord's notes) it's that chord's own start time, so the axis
  // only spans the window during which the chord's notes actually play
  // instead of dragging along every silent millisecond since the song
  // began; see to_scene_x() in PianoRollWidget.hpp
  double time_axis_baseline_ms = 0.0;
  // the tick lines + labels currently on screen, so redraw_time_axis_ticks()
  // can remove exactly those before drawing a fresh set at the new spacing,
  // leaving the rest of the scene (notes, pitch axis, playhead) untouched
  QList<QGraphicsItem*> time_axis_items;

  // rebuilt every PianoRollWidget::rebuild_scene() call; each drawn note
  // rect stores its index into this list (via QGraphicsItem::setData) so a
  // click on the rect can be traced back to the chord/note it represents
  QList<PianoRollNoteEvent> events;
  // parallel to events -- the actual drawn item for each event, so a table
  // selection can be traced forward to the bar(s) it should highlight
  QList<QGraphicsRectItem*> note_items;
  // parallel to events -- lets PianoRollWidget::select_chord_at_playhead()
  // find which chord a cursor time falls in without rescanning the whole
  // song on every playback tick
  QList<double> chord_start_times;

  explicit PianoRollNotesScene(QWidget& parent_widget);

  ~PianoRollNotesScene() override = default;

  NO_MOVE_COPY(PianoRollNotesScene)
};
