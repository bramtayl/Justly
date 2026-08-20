#include <QDockWidget>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsView>

#include "Tester.hpp"
#include "other/PianoRollNoteEvent.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/piano_roll/PianoRollNotesScene.hpp"
#include "widgets/piano_roll/PianoRollWidget.hpp"

namespace {
// checks that exactly the events matching the given criteria (mirroring
// get_selected_piano_roll_event_indices) are drawn with a highlight pen,
// and every other event is drawn plain
void check_piano_roll_highlight(PianoRollWidget& piano_roll_widget,
                                const RowType selection_row_type,
                                const int selection_chord_number,
                                const int selection_note_number) {
  const auto& events = piano_roll_widget.piano_roll_scene.events;
  const auto& note_items = piano_roll_widget.piano_roll_scene.note_items;
  for (auto event_index = 0; event_index < events.size();
       event_index = event_index + 1) {
    const auto& event = events.at(event_index);
    const auto is_highlighted =
        selection_row_type == RowType::chord_type
            ? event.chord_number == selection_chord_number
            : event.chord_number == selection_chord_number &&
                  event.note_number == selection_note_number &&
                  event.is_pitched ==
                      (selection_row_type == RowType::pitched_note_type);
    QCOMPARE(
        get_reference(note_items.at(event_index)).pen().style() != Qt::NoPen,
        is_highlighted);
  }
}
}  // namespace

void Tester::test_piano_roll_events_data() {
  QTest::addColumn<int>("note_number");
  QTest::addColumn<double>("frequency");
  QTest::addColumn<double>("duration_ms");
  QTest::addColumn<double>("velocity");

  QTest::newRow("pitched note 0") << 0 << 660.0 << 200.0 << 30.0;
  QTest::newRow("pitched note 1") << 1 << 1980.0 << 600.0 << 90.0;
}

void Tester::test_piano_roll_events() const {
  QFETCH(const int, note_number);
  QFETCH(const double, frequency);
  QFETCH(const double, duration_ms);
  QFETCH(const double, velocity);

  const auto events = get_piano_roll_events(song_editor.song_widget.song);
  const auto matching_event = std::ranges::find_if(
      events, [note_number](const PianoRollNoteEvent& event) -> auto {
        return event.chord_number == 1 && event.note_number == note_number &&
               event.is_pitched;
      });
  QVERIFY(matching_event != events.cend());
  QCOMPARE(matching_event->start_time_ms, 600.0);
  QCOMPARE(matching_event->duration_ms, duration_ms);
  QCOMPARE(matching_event->frequency, frequency);
  QCOMPARE(matching_event->velocity, velocity);
}

void Tester::test_piano_roll_events_total_count() const {
  QCOMPARE(get_piano_roll_events(song_editor.song_widget.song).size(), 12);
}

void Tester::test_piano_roll_time_bounds() const {
  const auto [baseline_ms, end_ms] =
      get_piano_roll_time_bounds(song_editor.song_widget.song, 1, 1);
  QCOMPARE(baseline_ms, 600.0);
  QCOMPARE(end_ms, 1200.0);
}

void Tester::test_piano_roll_dock_toggle() {
  auto& piano_roll_dock = song_editor.piano_roll_dock;
  auto& show_piano_roll_action =
      song_editor.song_menu_bar.view_menu.show_piano_roll_action;

  // song_editor is never shown() in these headless tests, so isVisible()
  // would always be false regardless of the dock's own shown/hidden state
  // (it also depends on the whole ancestor chain being on-screen);
  // isHidden() reflects the dock's own explicit show/hide state instead.
  QVERIFY(piano_roll_dock.isHidden());
  show_piano_roll_action.trigger();
  QVERIFY(!piano_roll_dock.isHidden());
  show_piano_roll_action.trigger();
  QVERIFY(piano_roll_dock.isHidden());
}

void Tester::test_piano_roll_rebuilds_on_edit() {
  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;
  auto& scene = song_editor.piano_roll_widget.piano_roll_scene;

  switch_to(song_editor, RowType::pitched_note_type, 1);
  const auto old_item_count = scene.items().size();

  select_cell(switch_table, 0, 0);
  song_editor.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
  QCOMPARE(scene.items().size(), old_item_count + 1);

  undo_stack.undo();  // undo insert
  QCOMPARE(scene.items().size(), old_item_count);

  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
}

void Tester::test_piano_roll_double_click_selects_note_data() {
  QTest::addColumn<bool>("is_pitched");
  QTest::addColumn<int>("note_number");
  QTest::addColumn<RowType>("expected_row_type");

  QTest::newRow("pitched") << true << 2 << RowType::pitched_note_type;
  QTest::newRow("unpitched") << false << 1 << RowType::unpitched_note_type;
}

void Tester::test_piano_roll_double_click_selects_note() {
  QFETCH(const bool, is_pitched);
  QFETCH(const int, note_number);
  QFETCH(const RowType, expected_row_type);

  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;
  auto& undo_stack = song_editor.song_widget.undo_stack;

  // chord number 1 (from test_song.xml) has both pitched and unpitched
  // notes, matching the fixture used by the other piano-roll tests above
  const auto& events = piano_roll_widget.piano_roll_scene.events;
  const auto event_iterator = std::ranges::find_if(
      events,
      [is_pitched, note_number](const PianoRollNoteEvent& event) -> auto {
        return event.chord_number == 1 && event.note_number == note_number &&
               event.is_pitched == is_pitched;
      });
  QVERIFY(event_iterator != events.cend());
  const auto event_index = static_cast<int>(event_iterator - events.cbegin());

  const QGraphicsItem* note_item_pointer = nullptr;
  for (auto* const item_pointer : piano_roll_widget.piano_roll_scene.items()) {
    const auto item_data = get_reference(item_pointer).data(0);
    if (item_data.isValid() && item_data.toInt() == event_index) {
      note_item_pointer = item_pointer;
      break;
    }
  }
  QVERIFY(note_item_pointer != nullptr);

  // drives the actual production event filter with a real QMouseEvent,
  // rather than calling add_replace_table directly, so this exercises the
  // full click-to-scene-item-to-callback path
  const auto view_pos = piano_roll_widget.piano_roll_scene.view.mapFromScene(
      note_item_pointer->sceneBoundingRect().center());
  const auto global_pos =
      get_reference(piano_roll_widget.piano_roll_scene.view.viewport())
          .mapToGlobal(view_pos);
  QMouseEvent double_click_event(QEvent::MouseButtonDblClick, QPointF(view_pos),
                                 QPointF(global_pos), Qt::LeftButton,
                                 Qt::LeftButton, Qt::NoModifier);
  piano_roll_widget.eventFilter(
      piano_roll_widget.piano_roll_scene.view.viewport(), &double_click_event);

  QCOMPARE(switch_table.delegate.current_row_type, expected_row_type);
  QCOMPARE(get_parent_chord_number(switch_table), 1);
  QCOMPARE(get_only_range(switch_table).top(), note_number);

  undo_stack.undo();
}

void Tester::test_piano_roll_click_selects_note_data() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<bool>("is_pitched");
  QTest::addColumn<int>("note_number");

  QTest::newRow("pitched") << RowType::pitched_note_type << true << 2;
  QTest::newRow("unpitched") << RowType::unpitched_note_type << false << 1;
}

void Tester::test_piano_roll_click_selects_note() {
  QFETCH(const RowType, row_type);
  QFETCH(const bool, is_pitched);
  QFETCH(const int, note_number);

  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;
  auto& undo_stack = song_editor.song_widget.undo_stack;

  // enters note mode for chord 1, matching the fixture used by
  // test_piano_roll_double_click_selects_note above, then starts on
  // a different row so the click below has to actually move the
  // selection rather than leave an already-correct one alone
  switch_to(song_editor, row_type, 1);
  select_cell(switch_table, 0, 0);

  const auto& events = piano_roll_widget.piano_roll_scene.events;
  const auto event_iterator = std::ranges::find_if(
      events,
      [is_pitched, note_number](const PianoRollNoteEvent& event) -> auto {
        return event.chord_number == 1 && event.note_number == note_number &&
               event.is_pitched == is_pitched;
      });
  QVERIFY(event_iterator != events.cend());
  const auto event_index = static_cast<int>(event_iterator - events.cbegin());

  const QGraphicsItem* note_item_pointer = nullptr;
  for (auto* const item_pointer : piano_roll_widget.piano_roll_scene.items()) {
    const auto item_data = get_reference(item_pointer).data(0);
    if (item_data.isValid() && item_data.toInt() == event_index) {
      note_item_pointer = item_pointer;
      break;
    }
  }
  QVERIFY(note_item_pointer != nullptr);

  // drives the actual production event filter with a real QMouseEvent,
  // the same way test_piano_roll_drag_selects_chord exercises a chord-
  // mode click, so this covers the full click-to-scene-item-to-
  // table-selection path rather than calling select_note_at_bar directly
  const auto view_pos = piano_roll_widget.piano_roll_scene.view.mapFromScene(
      note_item_pointer->sceneBoundingRect().center());
  const auto global_pos =
      get_reference(piano_roll_widget.piano_roll_scene.view.viewport())
          .mapToGlobal(view_pos);
  QMouseEvent press_event(QEvent::MouseButtonPress, QPointF(view_pos),
                          QPointF(global_pos), Qt::LeftButton, Qt::LeftButton,
                          Qt::NoModifier);
  piano_roll_widget.eventFilter(
      piano_roll_widget.piano_roll_scene.view.viewport(), &press_event);

  QCOMPARE(switch_table.delegate.current_row_type, row_type);
  QCOMPARE(get_parent_chord_number(switch_table), 1);
  QCOMPARE(get_only_range(switch_table).top(), note_number);

  QMouseEvent release_event(QEvent::MouseButtonRelease, QPointF(view_pos),
                            QPointF(global_pos), Qt::NoButton, Qt::NoButton,
                            Qt::NoModifier);
  piano_roll_widget.eventFilter(
      piano_roll_widget.piano_roll_scene.view.viewport(), &release_event);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_piano_roll_notes_mode_shows_only_chord_notes() {
  auto& song_widget = song_editor.song_widget;
  auto& piano_roll_widget = song_editor.piano_roll_widget;

  // two chords, each with a single note of its own, so entering notes
  // mode for one chord can be checked to hide the other chord's note
  // rather than keep showing every chord's notes on the timeline
  static const QString text =
      make_voice_song_xml({"A"}, {"D"}, {{{0}, {}}, {{0}, {}}});
  open_text(song_editor, text);

  QCOMPARE(get_piano_roll_events(song_widget.song).size(), 2);

  switch_to(song_editor, RowType::pitched_note_type, 0);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.size(), 1);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.at(0).chord_number, 0);
  maybe_switch_back_to_chords(song_widget.undo_stack,
                              RowType::pitched_note_type);

  switch_to(song_editor, RowType::pitched_note_type, 1);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.size(), 1);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.at(0).chord_number, 1);
  maybe_switch_back_to_chords(song_widget.undo_stack,
                              RowType::pitched_note_type);

  // back in chord mode, both chords' notes are shown again
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.size(), 2);

  // restore the fixture used by the other tests
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_piano_roll_notes_mode_axis_starts_at_chord_start() {
  auto& song_widget = song_editor.song_widget;
  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  auto& undo_stack = song_widget.undo_stack;

  // outside notes mode the axis spans the whole song, starting at time 0
  QCOMPARE(piano_roll_scene.time_axis_baseline_ms, 0.0);

  // chord 1 starts at 600ms and its notes run through 1200ms (see
  // test_piano_roll_time_bounds() above) -- in notes mode the axis should
  // be rebased to that chord's own start, so it only spans the 600ms
  // during which chord 1's notes actually play rather than dragging along
  // the silent 600ms before them
  switch_to(song_editor, RowType::pitched_note_type, 1);
  QCOMPARE(piano_roll_scene.time_axis_baseline_ms, 600.0);
  QCOMPARE(piano_roll_scene.time_axis_max_time_ms, 600.0);

  const auto& events = piano_roll_scene.events;
  const auto& note_items = piano_roll_scene.note_items;
  for (auto event_index = 0; event_index < events.size();
       event_index = event_index + 1) {
    const auto& event = events.at(event_index);
    QCOMPARE(get_reference(note_items.at(event_index)).rect().x(),
             (event.start_time_ms - 600.0) * PIANO_ROLL_PIXELS_PER_MS);
  }

  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
  QCOMPARE(piano_roll_scene.time_axis_baseline_ms, 0.0);
}

void Tester::test_piano_roll_selection_highlights_chord() {
  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;

  // chord number 1 (from test_song.xml) has both pitched and unpitched
  // notes, matching the fixture used by the other piano-roll tests above
  select_cell(switch_table, 1, 0);

  check_piano_roll_highlight(piano_roll_widget, RowType::chord_type, 1, -1);

  QVERIFY(piano_roll_widget.piano_roll_scene.playhead_item.isVisible());
  QCOMPARE(piano_roll_widget.piano_roll_scene.playhead_item.line().x1(),
           600.0 * PIANO_ROLL_PIXELS_PER_MS);
}

void Tester::test_piano_roll_selection_highlights_note_data() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("note_number");

  QTest::newRow("pitched") << RowType::pitched_note_type << 2;
  QTest::newRow("unpitched") << RowType::unpitched_note_type << 1;
}

void Tester::test_piano_roll_selection_highlights_note() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, note_number);

  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;
  auto& undo_stack = song_editor.song_widget.undo_stack;

  switch_to(song_editor, row_type, 1);
  select_cell(switch_table, note_number, 0);

  check_piano_roll_highlight(piano_roll_widget, row_type, 1, note_number);

  QVERIFY(piano_roll_widget.piano_roll_scene.playhead_item.isVisible());
  // both chord 1's pitched and unpitched notes start where the chord
  // itself starts -- see test_piano_roll_time_bounds() above -- but in
  // notes mode the axis is rebased to that same start time (see
  // test_piano_roll_notes_mode_axis_starts_at_chord_start() below), so
  // the playhead sits at 0 rather than at chord 1's absolute start time
  QCOMPARE(piano_roll_widget.piano_roll_scene.playhead_item.line().x1(), 0.0);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_piano_roll_selection_ignores_voice_table() {
  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;
  auto& undo_stack = song_editor.song_widget.undo_stack;

  // put a highlight/cursor up first, so switching to a voice table (which
  // has no timeline position) has to actually clear it rather than just
  // never having set it
  select_cell(switch_table, 1, 0);
  QVERIFY(piano_roll_widget.piano_roll_scene.playhead_item.isVisible());

  switch_to(song_editor, RowType::pitched_voice_type, -1);
  select_cell(switch_table, 0, 0);

  QVERIFY(!piano_roll_widget.piano_roll_scene.playhead_item.isVisible());
  for (auto* const note_item_pointer :
       piano_roll_widget.piano_roll_scene.note_items) {
    QCOMPARE(get_reference(note_item_pointer).pen().style(), Qt::NoPen);
  }

  maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);
}

void Tester::test_piano_roll_selection_preserves_multi_row_range() {
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;

  // selecting a range of chords (e.g. for "Play selection") must not get
  // collapsed down to a single row by the piano roll's cursor-follows-
  // selection sync -- regression test for a bug where every table
  // selection change (not just the cursor actually moving) fed back
  // through select_chord_at_playhead() and forced a single-row reselect
  auto& model = get_model(switch_table);
  get_selection_model(switch_table)
      .select(QItemSelection(model.index(1, 0), model.index(3, 0)),
              SELECT_AND_CLEAR);

  const auto& range = get_only_range(switch_table);
  QCOMPARE(range.top(), 1);
  QCOMPARE(range.bottom(), 3);
}

void Tester::test_piano_roll_drag_selects_chord() {
  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;

  // start on chord 1 (600ms-1200ms, per test_piano_roll_time_bounds()
  // above) so the drag below has to actually move the selection rather
  // than leave an already-correct one alone
  select_cell(switch_table, 1, 0);

  // chord 2 starts where chord 1 ends, at 1200ms
  const auto view_pos = piano_roll_widget.piano_roll_scene.view.mapFromScene(
      QPointF(1200.0 * PIANO_ROLL_PIXELS_PER_MS, 0));
  const auto global_pos =
      get_reference(piano_roll_widget.piano_roll_scene.view.viewport())
          .mapToGlobal(view_pos);

  QMouseEvent press_event(QEvent::MouseButtonPress, QPointF(view_pos),
                          QPointF(global_pos), Qt::LeftButton, Qt::LeftButton,
                          Qt::NoModifier);
  piano_roll_widget.eventFilter(
      piano_roll_widget.piano_roll_scene.view.viewport(), &press_event);

  QCOMPARE(switch_table.delegate.current_row_type, RowType::chord_type);
  QCOMPARE(get_only_range(switch_table).top(), 2);

  QMouseEvent release_event(QEvent::MouseButtonRelease, QPointF(view_pos),
                            QPointF(global_pos), Qt::NoButton, Qt::NoButton,
                            Qt::NoModifier);
  piano_roll_widget.eventFilter(
      piano_roll_widget.piano_roll_scene.view.viewport(), &release_event);
}

void Tester::test_piano_roll_drag_selects_chord_range() {
  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;
  auto& view = piano_roll_widget.piano_roll_scene.view;
  auto& selection_rect_item =
      piano_roll_widget.piano_roll_scene.selection_rect_item;
  const auto& song = song_editor.song_widget.song;

  // start on chord 0 so the drag below has to actually move the
  // selection rather than leave an already-correct one alone
  select_cell(switch_table, 0, 0);

  // the box mirrors whatever's selected, so it's already showing chord
  // 0's own extent before any drag happens
  {
    const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 0, 1);
    QVERIFY(selection_rect_item.isVisible());
    QCOMPARE(selection_rect_item.rect().left(),
             start_ms * PIANO_ROLL_PIXELS_PER_MS);
    QCOMPARE(selection_rect_item.rect().right(),
             end_ms * PIANO_ROLL_PIXELS_PER_MS);
  }

  const auto& chord_start_times =
      piano_roll_widget.piano_roll_scene.chord_start_times;
  QVERIFY(chord_start_times.size() > 3);

  const auto press_scene_x = chord_start_times.at(1) * PIANO_ROLL_PIXELS_PER_MS;
  const auto press_view_pos = view.mapFromScene(QPointF(press_scene_x, 0));
  const auto press_global_pos =
      get_reference(view.viewport()).mapToGlobal(press_view_pos);

  QMouseEvent press_event(QEvent::MouseButtonPress, QPointF(press_view_pos),
                          QPointF(press_global_pos), Qt::LeftButton,
                          Qt::LeftButton, Qt::NoModifier);
  piano_roll_widget.eventFilter(view.viewport(), &press_event);

  QCOMPARE(get_only_range(switch_table).top(), 1);
  QCOMPARE(get_only_range(switch_table).bottom(), 1);

  // the box follows the newly (single-chord) selection
  {
    const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 1, 1);
    QVERIFY(selection_rect_item.isVisible());
    QCOMPARE(selection_rect_item.rect().left(),
             start_ms * PIANO_ROLL_PIXELS_PER_MS);
    QCOMPARE(selection_rect_item.rect().right(),
             end_ms * PIANO_ROLL_PIXELS_PER_MS);
  }

  const auto move_scene_x = chord_start_times.at(3) * PIANO_ROLL_PIXELS_PER_MS;
  const auto move_view_pos = view.mapFromScene(QPointF(move_scene_x, 0));
  const auto move_global_pos =
      get_reference(view.viewport()).mapToGlobal(move_view_pos);

  QMouseEvent move_event(QEvent::MouseMove, QPointF(move_view_pos),
                         QPointF(move_global_pos), Qt::NoButton, Qt::LeftButton,
                         Qt::NoModifier);
  piano_roll_widget.eventFilter(view.viewport(), &move_event);

  // dragging from chord 1 to chord 3 should select the whole range in
  // between, not just reassign the selection to chord 3 alone
  QCOMPARE(get_only_range(switch_table).top(), 1);
  QCOMPARE(get_only_range(switch_table).bottom(), 3);

  // the box now spans the whole selected chord range
  {
    const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 1, 3);
    QVERIFY(selection_rect_item.isVisible());
    QCOMPARE(selection_rect_item.rect().left(),
             start_ms * PIANO_ROLL_PIXELS_PER_MS);
    QCOMPARE(selection_rect_item.rect().right(),
             end_ms * PIANO_ROLL_PIXELS_PER_MS);
  }

  QMouseEvent release_event(QEvent::MouseButtonRelease, QPointF(move_view_pos),
                            QPointF(move_global_pos), Qt::NoButton,
                            Qt::NoButton, Qt::NoModifier);
  piano_roll_widget.eventFilter(view.viewport(), &release_event);

  // the box mirrors the committed table selection rather than being a
  // purely in-drag affordance, so it must still be showing the same
  // range after the mouse is released
  {
    const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 1, 3);
    QVERIFY(selection_rect_item.isVisible());
    QCOMPARE(selection_rect_item.rect().left(),
             start_ms * PIANO_ROLL_PIXELS_PER_MS);
    QCOMPARE(selection_rect_item.rect().right(),
             end_ms * PIANO_ROLL_PIXELS_PER_MS);
  }
}

void Tester::test_piano_roll_playback_selects_chord() {
  auto& piano_roll_widget = song_editor.piano_roll_widget;
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;

  // start on chord 0; starting playback at chord 2's baseline (1200ms,
  // per test_piano_roll_time_bounds() above) shouldn't itself move the
  // selection -- only a timer tick should, so a multi-chord "Play
  // selection" isn't collapsed to a single row the instant Play is
  // pressed
  select_cell(switch_table, 0, 0);

  start_piano_roll_playhead(piano_roll_widget, 1200.0, 1800.0);
  QCOMPARE(get_only_range(switch_table).top(), 0);

  // simulates one playback timer tick without waiting on the real
  // QElapsedTimer -- elapsed() is at least 0, so current_ms is already
  // >= the 1200ms baseline, landing on chord 2
  update_playhead_position(piano_roll_widget.piano_roll_scene,
                           piano_roll_widget.axis_scene, switch_table,
                           piano_roll_widget.selecting_chord_from_playhead);
  QCOMPARE(get_only_range(switch_table).top(), 2);

  stop_piano_roll_playhead(piano_roll_widget);
}

void Tester::test_piano_roll_zoom() {
  auto& piano_roll_widget = song_editor.piano_roll_widget;

  QCOMPARE(piano_roll_widget.piano_roll_scene.view.transform().m11(), 1.0);
  QCOMPARE(piano_roll_widget.piano_roll_scene.view.transform().m22(), 1.0);

  zoom_in_piano_roll(piano_roll_widget);
  // only the time (x) axis scales -- the pitch (y) axis has to stay fixed
  // so it stays aligned with axis_scene, which is never zoomed
  QCOMPARE(piano_roll_widget.piano_roll_scene.view.transform().m11(),
           PIANO_ROLL_TIME_ZOOM_STEP);
  QCOMPARE(piano_roll_widget.piano_roll_scene.view.transform().m22(), 1.0);

  zoom_out_piano_roll(piano_roll_widget);
  QCOMPARE(piano_roll_widget.piano_roll_scene.view.transform().m11(), 1.0);

  // clamped rather than unbounded, so repeated zooming can't shrink/grow
  // the time axis into something unusable
  for (auto zoom_count = 0; zoom_count < 20; zoom_count = zoom_count + 1) {
    zoom_out_piano_roll(piano_roll_widget);
  }
  QCOMPARE(piano_roll_widget.piano_roll_scene.time_zoom_factor,
           PIANO_ROLL_MIN_TIME_ZOOM);

  for (auto zoom_count = 0; zoom_count < 40; zoom_count = zoom_count + 1) {
    zoom_in_piano_roll(piano_roll_widget);
  }
  QCOMPARE(piano_roll_widget.piano_roll_scene.time_zoom_factor,
           PIANO_ROLL_MAX_TIME_ZOOM);

  // restore, so later tests see the default 1x zoom
  set_notes_view_time_zoom(piano_roll_widget.piano_roll_scene, 1.0);
}
