#include "widgets/piano_roll/PianoRollWidget.hpp"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QChar>
#include <QtCore/QEasingCurve>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEvent>
#include <QtCore/QFlags>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMargins>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtCore/qcoreevent.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qobjectdefs.h>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtGui/QPolygon>
#include <QtGui/QTransform>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QScrollBar>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <optional>
#include <qboxlayout.h>
#include <utility>

#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "other/PianoRollNoteEvent.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "sound/PlayState.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/piano_roll/PianoRollAxisScene.hpp"
#include "widgets/piano_roll/PianoRollLegendScene.hpp"
#include "widgets/piano_roll/PianoRollNotesScene.hpp"
#include "widgets/piano_roll/PlayheadTransition.hpp"

auto to_scene_x(const PianoRollNotesScene &notes_scene,
                const double time_ms) -> double {
  return (time_ms - notes_scene.time_axis_baseline_ms) * PIANO_ROLL_PIXELS_PER_MS;
}

// (re)draws the time axis' ticks and labels, spaced (in ms) so they land
// roughly PIANO_ROLL_TARGET_TICK_PIXEL_SPACING apart on screen at the
// current time_zoom_factor -- called from PianoRollWidget::rebuild_scene()
// for the initial build and from set_time_zoom() whenever the zoom changes,
// since a spacing that looked right before a zoom change would otherwise
// crowd together (zooming in) or spread too far apart (zooming out)
namespace {

void redraw_time_axis_ticks(PianoRollNotesScene &notes_scene) {
  auto &scene = notes_scene;
  auto &time_axis_items = notes_scene.time_axis_items;

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
      nice_step_multipliers,
      [fraction](const double multiplier) -> auto { return fraction <= multiplier; });
  if (nice_multiplier_iterator != nice_step_multipliers.end()) {
    nice_fraction = *nice_multiplier_iterator;
  }
  const auto step_ms = nice_fraction * magnitude;
  const auto time_axis_y = notes_scene.time_axis_y;
  const auto time_axis_max_time_ms = notes_scene.time_axis_max_time_ms;
  for (auto step_number = 0;
      step_number * step_ms <= time_axis_max_time_ms;
      step_number = step_number + 1) {
    const auto time_ms = step_number * step_ms;
    const auto tick_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
    time_axis_items.push_back(
        scene.addLine(tick_x, time_axis_y, tick_x,
                     time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH, tick_pen));

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
    // the pitch axis' column, which this view can no longer scroll into (see
    // the view.setSceneRect() call in PianoRollWidget::rebuild_scene()) --
    // so clamp every label's left edge to the axis line instead. the label's
    // boundingRect() is in device pixels (it ignores the view's transform --
    // see the ItemIgnoresTransformations flag above), while tick_x is in
    // scene units that the view later scales by time_zoom_factor, so the
    // half-width has to be converted into scene units before it's compared
    // against/subtracted from tick_x -- otherwise at high zoom a fixed
    // device-pixel width looks huge next to the shrunken scene-unit tick_x,
    // clamping far more than just the first tick and stacking several
    // labels on top of each other at the axis line
    label.setPos(std::max(tick_x - (label.boundingRect().width() / 2) /
                                       notes_scene.time_zoom_factor,
                          PIANO_ROLL_AXIS_X),
                time_axis_y + PIANO_ROLL_AXIS_TICK_LENGTH +
                    PIANO_ROLL_AXIS_LABEL_GAP);
    time_axis_items.push_back(&label);
  }
}

}  // namespace

void set_notes_view_time_zoom(PianoRollNotesScene &notes_scene,
                              const double new_zoom_factor) {
  notes_scene.time_zoom_factor = std::clamp(
      new_zoom_factor, PIANO_ROLL_MIN_TIME_ZOOM, PIANO_ROLL_MAX_TIME_ZOOM);
  notes_scene.view.setTransform(
      QTransform::fromScale(notes_scene.time_zoom_factor, 1.0));
  // the tick spacing (in ms) that keeps ticks ~evenly spaced on screen
  // depends on the zoom factor, so every zoom change needs a fresh set of
  // ticks/labels -- just the time axis, not a full
  // PianoRollWidget::rebuild_scene()
  redraw_time_axis_ticks(notes_scene);
}

namespace {

auto drag_playhead_to(PianoRollNotesScene &notes_scene,
                      const QPoint &viewport_pos) -> double {
  const auto playhead_x =
      std::max(0.0, notes_scene.view.mapToScene(viewport_pos).x());
  const auto &scene_rect = notes_scene.sceneRect();
  auto &playhead_item = notes_scene.playhead_item;
  playhead_item.setLine(playhead_x, scene_rect.top(), playhead_x,
                        scene_rect.bottom());
  playhead_item.show();
  return playhead_x;
}

void show_selection_rect(PianoRollNotesScene &notes_scene,
                         const double start_x, const double end_x) {
  const auto &scene_rect = notes_scene.sceneRect();
  const auto left_x = std::min(start_x, end_x);
  const auto right_x = std::max(start_x, end_x);
  auto &selection_rect_item = notes_scene.selection_rect_item;
  selection_rect_item.setRect(left_x, scene_rect.top(), right_x - left_x,
                              scene_rect.height());
  selection_rect_item.show();
}

void hide_selection_rect(PianoRollNotesScene &notes_scene) {
  notes_scene.selection_rect_item.hide();
}

}  // namespace

void position_playhead(PianoRollNotesScene &notes_scene,
                       const double time_ms,
                       const bool follow_view) {
  const auto playhead_x = to_scene_x(notes_scene, time_ms);
  const auto &scene_rect = notes_scene.sceneRect();
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
  // PianoRollWidget::rebuild_scene()), so near the start/end of the song,
  // where centering the playhead would need to scroll past that edge, it
  // instead settles as close to centered as the edge allows
  auto &view = notes_scene.view;
  const auto visible_scene_rect =
      view.mapToScene(get_reference(view.viewport()).rect()).boundingRect();
  const auto vertical_center = visible_scene_rect.center().y();

  auto &playhead_transition = notes_scene.playhead_transition;

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
      const auto catchup_end_x = to_scene_x(
          notes_scene,
          std::min(notes_scene.playhead_baseline_ms + PIANO_ROLL_PLAYHEAD_CATCHUP_MS,
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

namespace {

auto get_voice_color(const int global_voice_index) -> QColor {
  // fixed categorical order (never cycled) -- a voice beyond the 8th falls
  // back to a shared "other" color rather than reusing an earlier hue
  static const QList<QColor> voice_colors{
      QColor("#2a78d6"), QColor("#eb6834"), QColor("#1baf7a"), QColor("#eda100"),
      QColor("#e87ba4"), QColor("#008300"), QColor("#4a3aa7"), QColor("#e34948"),
  };
  if (global_voice_index >= 0 && global_voice_index < voice_colors.size()) {
    return voice_colors.at(global_voice_index);
  }
  static const auto other_voice_color = QColor("#898781");
  return other_voice_color;
}

void draw_legend_row(QGraphicsScene &legend_scene, const QString &name,
                     const int global_voice_index, const double row_y) {
  legend_scene.addRect(0, row_y, PIANO_ROLL_LEGEND_SWATCH_SIZE,
               PIANO_ROLL_LEGEND_SWATCH_SIZE, QPen(Qt::NoPen),
               QBrush(get_voice_color(global_voice_index)));
  auto &label = get_reference(legend_scene.addSimpleText(name));
  label.setPos(PIANO_ROLL_LEGEND_SWATCH_SIZE + PIANO_ROLL_AXIS_LABEL_GAP,
              row_y - ((label.boundingRect().height() -
                       PIANO_ROLL_LEGEND_SWATCH_SIZE) /
                      2));
}

}  // namespace

auto get_piano_roll_time_bounds(
    const Song &song, const int first_chord_number,
    const int number_of_chords, const int first_note_number,
    const int number_of_notes,
    const std::optional<bool> pitched_filter)
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
      if (pitched_filter.has_value() && event.is_pitched != *pitched_filter) {
        continue;
      }
    }
    end_ms = std::max(end_ms, event.start_time_ms + event.duration_ms);
  }
  return {baseline_ms, end_ms};
}

namespace {

auto get_chord_number_at_time(
    const QList<double> &chord_start_times, const double time_ms) -> int {
  const auto first_later_iterator =
      std::ranges::upper_bound(chord_start_times, time_ms);
  return static_cast<int>(first_later_iterator - chord_start_times.begin()) -
         1;
}

auto get_chord_number_at_viewport_pos(
    const PianoRollNotesScene &piano_roll_scene, const QPoint &viewport_pos)
    -> int {
  const auto scene_pos = piano_roll_scene.view.mapToScene(viewport_pos);
  auto *const item_pointer = piano_roll_scene.itemAt(
      scene_pos, piano_roll_scene.view.transform());
  if (item_pointer != nullptr) {
    const auto event_index_data = item_pointer->data(0);
    if (event_index_data.isValid()) {
      return piano_roll_scene.events.at(event_index_data.toInt()).chord_number;
    }
  }
  return get_chord_number_at_time(piano_roll_scene.chord_start_times,
                                  std::max(0.0, scene_pos.x()) /
                                      PIANO_ROLL_PIXELS_PER_MS);
}

}  // namespace

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
namespace {

void select_chord_at_playhead(SwitchTable &switch_table,
                              const QList<double> &chord_start_times,
                              bool &selecting_chord_from_playhead,
                              const double time_ms) {
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

// selects the note row (in whichever of pitched_notes_model /
// unpitched_notes_model matches the clicked bar's own kind) corresponding to
// a note bar clicked directly in the piano roll -- the notes-mode
// counterpart to select_chord_at_playhead/select_chord_range_at_playhead,
// which only act while the table is in chord mode. A no-op unless the
// switch table is already showing that exact chord's notes of that exact
// kind (e.g. clicking a pitched note's bar while the table shows unpitched
// notes, or another chord's notes, has no row to select).
void select_note_at_bar(SwitchTable &switch_table,
                        const PianoRollNoteEvent &event) {
  const auto current_row_type = switch_table.delegate.current_row_type;
  const auto is_pitched = event.is_pitched;
  if (is_pitched
          ? (current_row_type != RowType::pitched_note_type ||
             switch_table.pitched_notes_model.parent_chord_number !=
                 event.chord_number)
          : (current_row_type != RowType::unpitched_note_type ||
             switch_table.unpitched_notes_model.parent_chord_number !=
                 event.chord_number)) {
    return;
  }

  auto &selection_model = get_selection_model(switch_table);
  const auto note_index =
      is_pitched
          ? switch_table.pitched_notes_model.index(event.note_number, 0)
          : switch_table.unpitched_notes_model.index(event.note_number, 0);
  const auto selected_rows = selection_model.selectedRows();
  if (selected_rows.size() == 1 && selected_rows.at(0) == note_index) {
    return;
  }
  selection_model.select(note_index, QItemSelectionModel::Select |
                                          QItemSelectionModel::Clear |
                                          QItemSelectionModel::Rows);
  switch_table.scrollTo(note_index);
}

void select_chord_range_at_playhead(SwitchTable &switch_table,
                                    const int number_of_chords,
                                    bool &selecting_chord_from_playhead,
                                    const int anchor_chord_number,
                                    const int current_chord_number) {
  if (switch_table.delegate.current_row_type != RowType::chord_type) {
    return;
  }
  if (number_of_chords == 0) {
    return;
  }
  const auto first_chord_number = std::clamp(
      std::min(anchor_chord_number, current_chord_number), 0,
      number_of_chords - 1);
  const auto last_chord_number = std::clamp(
      std::max(anchor_chord_number, current_chord_number), 0,
      number_of_chords - 1);

  auto &selection_model = get_selection_model(switch_table);
  const auto &selection = selection_model.selection();
  if (!selection.empty()) {
    const auto &range = get_only(selection);
    if (range.top() == first_chord_number &&
       range.bottom() == last_chord_number) {
      return;
    }
  }

  auto &chords_model = switch_table.chords_model;
  selecting_chord_from_playhead = true;
  selection_model.select(
      QItemSelection(chords_model.index(first_chord_number, 0),
                    chords_model.index(last_chord_number, 0)),
      QItemSelectionModel::Select | QItemSelectionModel::Clear |
          QItemSelectionModel::Rows);
  selecting_chord_from_playhead = false;
  switch_table.scrollTo(chords_model.index(
      std::clamp(current_chord_number, 0, number_of_chords - 1), 0));
}

}  // namespace

void zoom_in(PianoRollNotesScene &piano_roll_scene) {
  set_notes_view_time_zoom(
      piano_roll_scene, piano_roll_scene.time_zoom_factor * PIANO_ROLL_TIME_ZOOM_STEP);
}

void zoom_out(PianoRollNotesScene &piano_roll_scene) {
  set_notes_view_time_zoom(
      piano_roll_scene, piano_roll_scene.time_zoom_factor / PIANO_ROLL_TIME_ZOOM_STEP);
}

namespace {

void set_vertical_scrolling_enabled(QGraphicsView &view,
                                    const bool enabled) {
  get_reference(view.verticalScrollBar()).setEnabled(enabled);
}

}  // namespace

void set_manual_scrolling_enabled(PianoRollNotesScene &piano_roll_scene,
                                  PianoRollAxisScene &axis_scene,
                                  const bool enabled) {
  get_reference(piano_roll_scene.view.horizontalScrollBar()).setEnabled(enabled);
  set_vertical_scrolling_enabled(piano_roll_scene.view, enabled);
  set_vertical_scrolling_enabled(axis_scene.view, enabled);
}

void apply_selection_highlight(
    const Song &song, PianoRollNotesScene &piano_roll_scene,
    const RowType selection_row_type, const int selection_chord_number,
    const int selection_first_row_number, const int selection_number_of_rows,
    const bool selecting_chord_from_playhead) {
  const auto &events = piano_roll_scene.events;

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
    const auto pitched_filter =
        selection_row_type == RowType::pitched_note_type;
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
                event.is_pitched == pitched_filter &&
                event.note_number >= selection_first_row_number &&
                event.note_number <
                    selection_first_row_number + selection_number_of_rows) {
        is_selected[event_index] = true;
      }
    }
  }

  // styles each note bar's pen based on is_selected (parallel to
  // events/note_items), leaving highlighted_bounds as the union of the
  // highlighted bars' scene bounds (a null rect if none are highlighted) so
  // it can be scrolled into view below
  QRectF highlighted_bounds;
  auto &note_items = piano_roll_scene.note_items;
  for (auto event_index = 0; event_index < note_items.size();
      event_index = event_index + 1) {
    auto &note_item = get_reference(note_items.at(event_index));
    if (is_selected.at(event_index)) {
      // cosmetic so the highlight stroke stays a constant device-pixel
      // width instead of stretching with the notes view's horizontal zoom
      // transform (see set_notes_view_time_zoom) -- an uncapped width at
      // high zoom oversized the selected note's right edge enough to look
      // like a stray, unlabeled extra tick past the note's true end time
      auto highlight_pen = QPen(Qt::black, PIANO_ROLL_HIGHLIGHT_PEN_WIDTH);
      highlight_pen.setCosmetic(true);
      note_item.setPen(highlight_pen);
      highlighted_bounds =
          highlighted_bounds.united(note_item.sceneBoundingRect());
    } else {
      note_item.setPen(QPen(Qt::NoPen));
    }
  }

  const auto has_selection = (is_chord_selection || is_note_selection) &&
                             selection_number_of_rows > 0;

  if (!has_selection) {
    if (!piano_roll_scene.playhead_active) {
      piano_roll_scene.playhead_item.hide();
    }
    hide_selection_rect(piano_roll_scene);
    return;
  }

  const auto [range_start_ms, range_end_ms] = get_piano_roll_time_bounds(
      song,
      is_chord_selection ? selection_first_row_number
                         : selection_chord_number,
      is_chord_selection ? selection_number_of_rows : 1,
      is_chord_selection ? 0 : selection_first_row_number,
      is_chord_selection ? -1 : selection_number_of_rows,
      is_chord_selection
          ? std::nullopt
          : std::make_optional(
                selection_row_type == RowType::pitched_note_type));

  // a shaded box over the selected range's own timeline extent -- driven
  // straight off the committed selection (rather than raw drag position),
  // so it stays visible after the mouse is released, tracks table-driven
  // selection changes too, and disappears only once the selection itself
  // is cleared or moves to a voice table
  show_selection_rect(piano_roll_scene, to_scene_x(piano_roll_scene, range_start_ms),
                      to_scene_x(piano_roll_scene, range_end_ms));

  // playback already owns the cursor line while it's running, and so does
  // an in-progress manual drag (which set it more precisely than a chord's
  // start time); a selection change caused by select_chord_at_playhead()
  // itself shouldn't reposition the cursor either, since it's just
  // reporting where the cursor already is -- don't yank it away from any
  // of the three just because the table selection changed underneath it
  if (!piano_roll_scene.playhead_active && !piano_roll_scene.playhead_dragging &&
     !selecting_chord_from_playhead) {
    piano_roll_scene.playhead_item.show();
    position_playhead(piano_roll_scene, range_start_ms, false);
  }
  if (!highlighted_bounds.isNull()) {
    piano_roll_scene.view.ensureVisible(highlighted_bounds,
                       static_cast<int>(PIANO_ROLL_SCENE_MARGIN),
                       static_cast<int>(PIANO_ROLL_SCENE_MARGIN));
  }
}

void rebuild_scene(QWidget &widget, const SongWidget &song_widget,
                   PianoRollNotesScene &piano_roll_scene,
                   PianoRollAxisScene &axis_scene,
                   PianoRollLegendScene &legend_scene,
                   QBoxLayout &row_layout,
                   const RowType selection_row_type,
                   const int selection_chord_number,
                   const int selection_first_row_number,
                   const int selection_number_of_rows,
                   const bool selecting_chord_from_playhead) {
  const auto &song = song_widget.song;

  // repopulates the notes scene with a fresh set of note bars + the
  // pitch/time axes for the current song
  {
    auto &notes_scene = piano_roll_scene;
    auto &scene = notes_scene;
    auto &playhead_item = notes_scene.playhead_item;
    auto &selection_rect_item = notes_scene.selection_rect_item;

    scene.removeItem(&playhead_item);
    const auto saved_line = playhead_item.line();
    const auto was_visible = playhead_item.isVisible();

    scene.removeItem(&selection_rect_item);
    const auto saved_selection_rect = selection_rect_item.rect();
    const auto selection_rect_was_visible = selection_rect_item.isVisible();

    scene.clear();
    notes_scene.note_items.clear();
    // scene.clear() above already deleted these items -- just drop the now-
    // dangling pointers so redraw_time_axis_ticks() doesn't try to remove
    // them again below
    notes_scene.time_axis_items.clear();

    axis_scene.clear();

    const auto &pitched_voices = song.pitched_voices;
    const auto number_of_pitched_voices =
        static_cast<int>(pitched_voices.size());

    auto &events = notes_scene.events;
    events = get_piano_roll_events(song);
    // in notes mode the switch table only shows one chord's notes at a
    // time, so the piano roll should mirror that rather than keep drawing
    // every other chord's notes alongside them
    const auto notes_mode_chord_number =
        get_parent_chord_number(song_widget.switch_column.switch_table);
    if (notes_mode_chord_number != -1) {
      QList<PianoRollNoteEvent> chord_events;
      std::ranges::copy_if(
          events, std::back_inserter(chord_events),
          [notes_mode_chord_number](const PianoRollNoteEvent &event) -> auto {
            return event.chord_number == notes_mode_chord_number;
          });
      events = std::move(chord_events);
    }
    // each chord's start time, in chord order -- chords are laid out back-
    // to-back with no gaps, so a chord's own end time is simply the next
    // chord's start (or, for the last chord, whatever the caller already
    // knows the song's end time to be)
    auto &chord_start_times_out = notes_scene.chord_start_times;
    chord_start_times_out.clear();
    {
      PlayState play_state;
      initialize_playstate(song, play_state, 0);
      for (const auto &chord : song.chords) {
        modulate(play_state, chord);
        chord_start_times_out.push_back(play_state.current_time);
        move_time(play_state, chord);
      }
    }

    // in notes mode the axis should only span the window during which this
    // chord's own notes play, not every silent millisecond since the song
    // began -- so rebase time 0 to the chord's own start time (0 outside
    // notes mode, leaving the axis as the whole song's timeline)
    const auto &chord_start_times = notes_scene.chord_start_times;
    const auto time_axis_baseline_ms =
        notes_mode_chord_number != -1 &&
                notes_mode_chord_number <
                    static_cast<int>(chord_start_times.size())
            ? chord_start_times.at(notes_mode_chord_number)
            : 0.0;
    notes_scene.time_axis_baseline_ms = time_axis_baseline_ms;

    auto min_midi = std::numeric_limits<double>::max();
    auto max_midi = std::numeric_limits<double>::lowest();
    auto max_time_ms = 0.0;
    for (const auto &event : events) {
      max_time_ms = std::max(max_time_ms, event.start_time_ms +
                                              event.duration_ms -
                                              time_axis_baseline_ms);
      if (event.is_pitched) {
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
      if (event.is_pitched) {
        continue;
      }
      auto assigned_lane = -1;
      const auto lane_iterator = std::ranges::find_if(
          lane_end_times,
          [start_time_ms = event.start_time_ms](const double end_time) -> auto {
            return end_time <= start_time_ms;
          });
      if (lane_iterator != lane_end_times.end()) {
        assigned_lane = static_cast<int>(lane_iterator - lane_end_times.begin());
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

      // drawn into axis_scene (not the notes scene) -- it's the
      // same y = -midi * PIANO_ROLL_PIXELS_PER_SEMITONE coordinate formula
      // as the note bars below, so the two views' vertical scrollbars
      // (kept in lockstep by PianoRollWidget's constructor) line the ticks
      // up with their notes despite living in separate scenes
      for (auto midi_value = first_octave; midi_value <= last_octave;
          midi_value = midi_value + HALFSTEPS_PER_OCTAVE) {
        const auto tick_y = -midi_value * PIANO_ROLL_PIXELS_PER_SEMITONE;
        axis_scene.addLine(PIANO_ROLL_AXIS_X - PIANO_ROLL_AXIS_TICK_LENGTH, tick_y,
                     PIANO_ROLL_AXIS_X, tick_y);

        auto &label =
            get_reference(axis_scene.addSimpleText(get_note_name(midi_value)));
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
    notes_scene.time_axis_max_time_ms = max_time_ms;
    notes_scene.time_axis_y = axis_y;
    scene.addLine(PIANO_ROLL_AXIS_X, axis_y, max_time_ms * PIANO_ROLL_PIXELS_PER_MS,
                 axis_y);
    redraw_time_axis_ticks(notes_scene);
    const auto unpitched_lane_top = axis_y + PIANO_ROLL_UNPITCHED_LANE_GAP;

    auto &note_items = notes_scene.note_items;
    for (auto event_index = 0; event_index < events.size();
        event_index = event_index + 1) {
      const auto &event = events.at(event_index);
      const auto bar_x = to_scene_x(notes_scene, event.start_time_ms);
      const auto width =
          std::max(PIANO_ROLL_MIN_BAR_WIDTH,
                   event.duration_ms * PIANO_ROLL_PIXELS_PER_MS);

      const auto is_pitched = event.is_pitched;
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

    scene.addItem(&selection_rect_item);
    selection_rect_item.setRect(saved_selection_rect);
    selection_rect_item.setVisible(selection_rect_was_visible);
  }

  // lists every voice (pitched first, then unpitched) as a colored swatch +
  // name, in the same order used to assign global_voice_index for coloring,
  // then sizes legend_scene to exactly fit its content (plus a margin), so
  // the fixed-width column stays as narrow as the longest voice name rather
  // than an arbitrary guessed width
  {
    auto &scene = legend_scene;
    auto &view = legend_scene.view;

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

  const auto &scene_rect = piano_roll_scene.sceneRect();
  // the pitch ticks/labels' own scene has no notes to size itself against,
  // so its bounding rect's left edge is exactly the widest label's left
  // edge (mirroring the -MARGIN breathing room the notes scene gives
  // itself); vertically, though, the two views' scrollbars are kept in
  // lockstep (wired up in the constructor), so their scene rects have to
  // share one vertical range -- the union of both scenes' own content --
  // or the two would drift out of alignment whenever a tick/label pokes
  // slightly above or below the notes scene's own margin
  const auto axis_left = axis_scene.itemsBoundingRect().left() -
                         PIANO_ROLL_SCENE_MARGIN;
  const auto axis_column_width = PIANO_ROLL_AXIS_X - axis_left;
  const auto vertical_rect = scene_rect.united(axis_scene.itemsBoundingRect());

  axis_scene.view.setFixedWidth(
      static_cast<int>(std::ceil(axis_column_width)) +
      (2 * axis_scene.view.frameWidth()));
  axis_scene.view.setSceneRect(axis_left, vertical_rect.top(), axis_column_width,
                              vertical_rect.height());

  // without this, scrolling the notes view all the way left would reveal
  // blank scene space to the left of the axis line (the -MARGIN gutter
  // baked into scene_rect) rather than clipping flush against it, since
  // that gutter is meant for axis_scene's column, not this one
  piano_roll_scene.view.setSceneRect(
      PIANO_ROLL_AXIS_X, vertical_rect.top(),
      scene_rect.right() - PIANO_ROLL_AXIS_X, vertical_rect.height());

  // a short song (few voices, narrow pitch range) doesn't need nearly as
  // much vertical space as PIANO_ROLL_MIN_HEIGHT reserves -- letting the
  // dock grow well past the content just leaves blank gray space below the
  // last note. Capping it at the content's own height (plus the chrome
  // around it) means dragging the dock splitter taller stops being useful
  // once the whole piano roll is already on screen, rather than dragging
  // in dead space. Still floored at PIANO_ROLL_MIN_HEIGHT so an
  // empty/near-empty piano roll doesn't collapse to a sliver.
  auto &notes_scene_view = piano_roll_scene.view;
  const auto chrome_height =
      (2 * notes_scene_view.frameWidth()) +
      get_reference(notes_scene_view.horizontalScrollBar()).sizeHint().height() +
      row_layout.contentsMargins().top() +
      row_layout.contentsMargins().bottom();
  widget.setMaximumHeight(static_cast<int>(std::max(
      static_cast<double>(PIANO_ROLL_MIN_HEIGHT),
      std::ceil(vertical_rect.height() + chrome_height))));

  apply_selection_highlight(song, piano_roll_scene, selection_row_type,
                            selection_chord_number, selection_first_row_number,
                            selection_number_of_rows,
                            selecting_chord_from_playhead);
}

void stop_playhead(PianoRollNotesScene &piano_roll_scene,
                   PianoRollAxisScene &axis_scene, const Song &song,
                   const RowType selection_row_type,
                   const int selection_chord_number,
                   const int selection_first_row_number,
                   const int selection_number_of_rows,
                   const bool selecting_chord_from_playhead) {
  piano_roll_scene.playhead_timer.stop();
  piano_roll_scene.playhead_active = false;
  piano_roll_scene.playhead_transition = PlayheadTransition::none;

  set_manual_scrolling_enabled(piano_roll_scene, axis_scene, true);
  apply_selection_highlight(song, piano_roll_scene, selection_row_type,
                            selection_chord_number, selection_first_row_number,
                            selection_number_of_rows,
                            selecting_chord_from_playhead);
}

void update_playhead_position(PianoRollNotesScene &piano_roll_scene,
                              PianoRollAxisScene &axis_scene,
                              SwitchTable &switch_table,
                              bool &selecting_chord_from_playhead) {
  if (!piano_roll_scene.playhead_active) {
    return;
  }
  const auto current_ms =
      piano_roll_scene.playhead_baseline_ms +
      static_cast<double>(piano_roll_scene.playhead_elapsed_timer.elapsed());
  if (current_ms >= piano_roll_scene.playhead_end_ms) {
    piano_roll_scene.playhead_active = false;
    piano_roll_scene.playhead_timer.stop();
    set_manual_scrolling_enabled(piano_roll_scene, axis_scene, true);
    position_playhead(piano_roll_scene, piano_roll_scene.playhead_end_ms);
    select_chord_at_playhead(switch_table, piano_roll_scene.chord_start_times,
                             selecting_chord_from_playhead,
                             piano_roll_scene.playhead_end_ms);
    return;
  }
  position_playhead(piano_roll_scene, current_ms);
  select_chord_at_playhead(switch_table, piano_roll_scene.chord_start_times,
                           selecting_chord_from_playhead, current_ms);
}

PianoRollWidget::PianoRollWidget(const SongWidget &song_widget_input)
    : song_widget(song_widget_input) {
  // a bottom dock would otherwise default to a cramped sliver; this keeps
  // it usable out of the box while still letting the user drag it taller
  // (or shorter, down to this floor) via the splitter
  setMinimumHeight(PIANO_ROLL_MIN_HEIGHT);

  row_layout.setSpacing(0);
  row_layout.addWidget(&axis_scene.view);
  row_layout.addWidget(&piano_roll_scene.view);
  row_layout.addWidget(&legend_scene.view);

  QObject::connect(piano_roll_scene.view.verticalScrollBar(),
                   &QScrollBar::valueChanged, this,
                   [this](const int value) -> auto {
                     get_reference(axis_scene.view.verticalScrollBar()).setValue(value);
                   });
  // kept symmetric so that scrolling with the mouse wheel while hovered
  // over the axis column (still possible despite the hidden scrollbar)
  // moves the main view along with it, rather than desyncing the two
  QObject::connect(axis_scene.view.verticalScrollBar(),
                   &QScrollBar::valueChanged, this,
                   [this](const int value) -> auto {
                     get_reference(piano_roll_scene.view.verticalScrollBar())
                         .setValue(value);
                   });

  QObject::connect(&piano_roll_scene.playhead_timer, &QTimer::timeout, this,
                   [this]() -> auto {
                     update_playhead_position(
                         piano_roll_scene, axis_scene,
                         song_widget.switch_column.switch_table,
                         selecting_chord_from_playhead);
                   });

  // the view has no interactivity of its own (no item selection, no
  // custom QGraphicsView subclass), so double-clicks and ctrl+wheel zoom
  // are picked up via an event filter on the viewport rather than
  // overriding QGraphicsView
  get_reference(piano_roll_scene.view.viewport()).installEventFilter(this);

  rebuild_scene(*this, song_widget, piano_roll_scene, axis_scene, legend_scene,
               row_layout, selection_row_type, selection_chord_number,
               selection_first_row_number, selection_number_of_rows,
               selecting_chord_from_playhead);
}

auto PianoRollWidget::eventFilter(QObject *watched_pointer, QEvent *event_pointer)
    -> bool {
  auto &view = piano_roll_scene.view;
  auto &switch_table = song_widget.switch_column.switch_table;
  if (get_reference(event_pointer).type() == QEvent::Wheel &&
     watched_pointer == view.viewport()) {
    auto &wheel_event = get_reference(dynamic_cast<QWheelEvent *>(event_pointer));
    if (wheel_event.modifiers().testFlag(Qt::ControlModifier)) {
      const auto angle_delta_y = wheel_event.angleDelta().y();
      if (angle_delta_y > 0) {
        zoom_in(piano_roll_scene);
      } else if (angle_delta_y < 0) {
        zoom_out(piano_roll_scene);
      }
      return true;
    }
  }
  if (get_reference(event_pointer).type() == QEvent::MouseButtonDblClick &&
     watched_pointer == view.viewport()) {
    const auto &mouse_event =
        get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
    auto *const item_pointer = piano_roll_scene.itemAt(
        view.mapToScene(mouse_event.pos()), view.transform());
    if (item_pointer != nullptr) {
      const auto event_index_data = item_pointer->data(0);
      if (event_index_data.isValid() && note_double_clicked) {
        const auto &event =
            piano_roll_scene.events.at(event_index_data.toInt());
        note_double_clicked(event.chord_number, event.note_number,
                           event.is_pitched);
      }
    }
  }
  if (get_reference(event_pointer).type() == QEvent::MouseButtonPress &&
     watched_pointer == view.viewport()) {
    const auto &mouse_event =
        get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
    if (mouse_event.button() == Qt::LeftButton) {
      // a manual click/drag takes over the cursor from playback's timer-
      // driven animation, the same way it takes over from a stale
      // selection-driven position in drag_playhead_to()
      if (piano_roll_scene.playhead_active) {
        stop_playhead(piano_roll_scene, axis_scene, song_widget.song,
                     selection_row_type, selection_chord_number,
                     selection_first_row_number, selection_number_of_rows,
                     selecting_chord_from_playhead);
      }
      piano_roll_scene.playhead_dragging = true;
      // resolved before drag_playhead_to() moves the playhead line onto
      // this exact click position -- that line sits in front of the note
      // bars (z=1 vs their z=0), so running the hit-test after would have
      // it hit the playhead line itself instead of whatever note bar is
      // actually drawn under the click
      drag_start_chord_number =
          get_chord_number_at_viewport_pos(piano_roll_scene, mouse_event.pos());
      auto *const item_pointer = piano_roll_scene.itemAt(
          view.mapToScene(mouse_event.pos()), view.transform());
      const auto event_index_data =
          item_pointer == nullptr ? QVariant() : item_pointer->data(0);
      static_cast<void>(drag_playhead_to(piano_roll_scene, mouse_event.pos()));
      select_chord_range_at_playhead(
          switch_table,
          static_cast<int>(piano_roll_scene.chord_start_times.size()),
          selecting_chord_from_playhead, drag_start_chord_number,
          drag_start_chord_number);
      // while in note mode, clicking directly on a note bar also selects
      // that note's own row in the switch table -- mirroring the
      // above chord-mode range select, but keyed to the exact bar
      // clicked (via the same item hit-test note_double_clicked uses)
      // rather than nearest-chord-by-time, since a click that misses
      // every bar has no single note row to select
      if (event_index_data.isValid()) {
        select_note_at_bar(
            switch_table, piano_roll_scene.events.at(event_index_data.toInt()));
      }
      return true;
    }
  }
  if (get_reference(event_pointer).type() == QEvent::MouseMove &&
     piano_roll_scene.playhead_dragging && watched_pointer == view.viewport()) {
    const auto &mouse_event =
        get_reference(dynamic_cast<QMouseEvent *>(event_pointer));
    const auto current_chord_number =
        get_chord_number_at_viewport_pos(piano_roll_scene, mouse_event.pos());
    static_cast<void>(drag_playhead_to(piano_roll_scene, mouse_event.pos()));
    select_chord_range_at_playhead(
        switch_table,
        static_cast<int>(piano_roll_scene.chord_start_times.size()),
        selecting_chord_from_playhead, drag_start_chord_number,
        current_chord_number);
    return true;
  }
  if (get_reference(event_pointer).type() == QEvent::MouseButtonRelease &&
     piano_roll_scene.playhead_dragging && watched_pointer == view.viewport()) {
    piano_roll_scene.playhead_dragging = false;
    drag_start_chord_number = -1;
    return true;
  }
  return QWidget::eventFilter(watched_pointer, event_pointer);
}
