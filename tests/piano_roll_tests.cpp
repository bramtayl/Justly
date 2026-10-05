#include <QDockWidget>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsView>

#include "Tester.hpp"
#include "piano_roll/PianoRollNotesScene.hpp"
#include "piano_roll/PianoRollWidget.hpp"

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

// sends a mouse event at a scene position through the production event
// filter, the same way the click/drag tests below do, and returns whether
// the filter consumed it
auto send_piano_roll_mouse_event(PianoRollWidget& piano_roll_widget,
                                 const QEvent::Type event_type,
                                 const QPointF& scene_pos,
                                 const Qt::MouseButton button) -> bool {
  auto& view = piano_roll_widget.piano_roll_scene.view;
  const auto view_pos = view.mapFromScene(scene_pos);
  const auto global_pos = get_reference(view.viewport()).mapToGlobal(view_pos);
  QMouseEvent mouse_event(event_type, QPointF(view_pos), QPointF(global_pos),
                          button, button, Qt::NoModifier);
  return piano_roll_widget.eventFilter(view.viewport(), &mouse_event);
}

// the scene-space center of the bar drawn for the given note
auto get_note_bar_center(const PianoRollWidget& piano_roll_widget,
                         const int chord_number, const int note_number,
                         const bool is_pitched) -> std::optional<QPointF> {
  const auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  const auto& events = piano_roll_scene.events;
  const auto event_iterator = std::ranges::find_if(
      events, [=](const PianoRollNoteEvent& event) -> auto {
        return event.chord_number == chord_number &&
               event.note_number == note_number &&
               event.is_pitched == is_pitched;
      });
  if (event_iterator == events.cend()) {
    return std::nullopt;
  }
  const auto event_index = static_cast<int>(event_iterator - events.cbegin());
  for (auto* const item_pointer : piano_roll_scene.items()) {
    const auto item_data = get_reference(item_pointer).data(0);
    if (item_data.isValid() && item_data.toInt() == event_index) {
      return item_pointer->sceneBoundingRect().center();
    }
  }
  return std::nullopt;
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

  const auto events = get_piano_roll_events(main_window.window_body.song);
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
  QCOMPARE(get_piano_roll_events(main_window.window_body.song).size(), 12);
}

void Tester::test_piano_roll_time_bounds() const {
  const auto [baseline_ms, end_ms] =
      get_piano_roll_time_bounds(main_window.window_body.song, 1, 1);
  QCOMPARE(baseline_ms, 600.0);
  QCOMPARE(end_ms, 1200.0);

  // a note range with no pitched/unpitched filter spans both kinds of note
  const auto& song = main_window.window_body.song;
  const auto pitched_end_ms =
      get_piano_roll_time_bounds(song, 1, 1, 0, 1, true).second;
  const auto unpitched_end_ms =
      get_piano_roll_time_bounds(song, 1, 1, 0, 1, false).second;
  QCOMPARE(get_piano_roll_time_bounds(song, 1, 1, 0, 1).second,
           std::max(pitched_end_ms, unpitched_end_ms));
}

void Tester::test_piano_roll_dock_toggle() {
  auto& piano_roll_dock = main_window.piano_roll_dock;
  auto& show_piano_roll_action =
      main_window.song_menu_bar.view_menu.show_piano_roll_action;

  // main_window is never shown() in these headless tests, so isVisible()
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
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  auto& scene = main_window.piano_roll_widget.piano_roll_scene;

  switch_to(main_window, RowType::pitched_note_type, 1);
  const auto old_item_count = scene.items().size();

  select_cell(switch_table, 0, 0);
  main_window.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
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

  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;
  auto& undo_stack = main_window.window_body.undo_stack;

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

  // drives the actual production event filter with real QMouseEvents,
  // rather than calling add_replace_table directly, so this exercises the
  // full click-to-scene-item-to-callback path. Qt delivers a double-click
  // as press, release, double-click -- and the press moves the playhead
  // line (drawn in front of the bars) right under the cursor, so the
  // double-click has to see past it to the bar
  const auto view_pos = piano_roll_widget.piano_roll_scene.view.mapFromScene(
      note_item_pointer->sceneBoundingRect().center());
  const auto global_pos =
      get_reference(piano_roll_widget.piano_roll_scene.view.viewport())
          .mapToGlobal(view_pos);
  auto* const viewport_pointer =
      piano_roll_widget.piano_roll_scene.view.viewport();
  QMouseEvent press_event(QEvent::MouseButtonPress, QPointF(view_pos),
                          QPointF(global_pos), Qt::LeftButton, Qt::LeftButton,
                          Qt::NoModifier);
  piano_roll_widget.eventFilter(viewport_pointer, &press_event);
  QMouseEvent release_event(QEvent::MouseButtonRelease, QPointF(view_pos),
                            QPointF(global_pos), Qt::LeftButton, Qt::NoButton,
                            Qt::NoModifier);
  piano_roll_widget.eventFilter(viewport_pointer, &release_event);
  QMouseEvent double_click_event(QEvent::MouseButtonDblClick, QPointF(view_pos),
                                 QPointF(global_pos), Qt::LeftButton,
                                 Qt::LeftButton, Qt::NoModifier);
  piano_roll_widget.eventFilter(viewport_pointer, &double_click_event);
  piano_roll_widget.eventFilter(viewport_pointer, &release_event);

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

  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;
  auto& undo_stack = main_window.window_body.undo_stack;

  // enters note mode for chord 1, matching the fixture used by
  // test_piano_roll_double_click_selects_note above, then starts on
  // a different row so the click below has to actually move the
  // selection rather than leave an already-correct one alone
  switch_to(main_window, row_type, 1);
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

  // clicking the already-selected note again leaves the selection alone
  piano_roll_widget.eventFilter(
      piano_roll_widget.piano_roll_scene.view.viewport(), &press_event);
  QCOMPARE(get_only_range(switch_table).top(), note_number);
  piano_roll_widget.eventFilter(
      piano_roll_widget.piano_roll_scene.view.viewport(), &release_event);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_piano_roll_notes_mode_shows_only_chord_notes() {
  auto& window_body = main_window.window_body;
  auto& piano_roll_widget = main_window.piano_roll_widget;

  // two chords, each with a single note of its own, so entering notes
  // mode for one chord can be checked to hide the other chord's note
  // rather than keep showing every chord's notes on the timeline
  static const QString text =
      make_voice_song_xml({"A"}, {"D"}, {{{0}, {}}, {{0}, {}}});
  open_text(main_window, text);

  QCOMPARE(get_piano_roll_events(window_body.song).size(), 2);

  switch_to(main_window, RowType::pitched_note_type, 0);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.size(), 1);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.at(0).chord_number, 0);
  maybe_switch_back_to_chords(window_body.undo_stack,
                              RowType::pitched_note_type);

  switch_to(main_window, RowType::pitched_note_type, 1);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.size(), 1);
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.at(0).chord_number, 1);
  maybe_switch_back_to_chords(window_body.undo_stack,
                              RowType::pitched_note_type);

  // back in chord mode, both chords' notes are shown again
  QCOMPARE(piano_roll_widget.piano_roll_scene.events.size(), 2);

  // restore the fixture used by the other tests
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_piano_roll_notes_mode_axis_starts_at_chord_start() {
  auto& window_body = main_window.window_body;
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  auto& undo_stack = window_body.undo_stack;

  // outside notes mode the axis spans the whole song, starting at time 0
  QCOMPARE(piano_roll_scene.time_axis_baseline_ms, 0.0);

  // chord 1 starts at 600ms and its notes run through 1200ms (see
  // test_piano_roll_time_bounds() above) -- in notes mode the axis should
  // be rebased to that chord's own start, so it only spans the 600ms
  // during which chord 1's notes actually play rather than dragging along
  // the silent 600ms before them
  switch_to(main_window, RowType::pitched_note_type, 1);
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
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

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

  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;
  auto& undo_stack = main_window.window_body.undo_stack;

  switch_to(main_window, row_type, 1);
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
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;
  auto& undo_stack = main_window.window_body.undo_stack;

  // put a highlight/cursor up first, so switching to a voice table (which
  // has no timeline position) has to actually clear it rather than just
  // never having set it
  select_cell(switch_table, 1, 0);
  QVERIFY(piano_roll_widget.piano_roll_scene.playhead_item.isVisible());

  switch_to(main_window, RowType::pitched_voice_type, -1);
  select_cell(switch_table, 0, 0);

  QVERIFY(!piano_roll_widget.piano_roll_scene.playhead_item.isVisible());
  for (auto* const note_item_pointer :
       piano_roll_widget.piano_roll_scene.note_items) {
    QCOMPARE(get_reference(note_item_pointer).pen().style(), Qt::NoPen);
  }

  maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);
}

void Tester::test_piano_roll_selection_preserves_multi_row_range() {
  auto& switch_table = main_window.window_body.switch_column.switch_table;

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
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

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
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;
  auto& view = piano_roll_widget.piano_roll_scene.view;
  auto& selection_rect_item =
      piano_roll_widget.piano_roll_scene.selection_rect_item;
  const auto& song = main_window.window_body.song;

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
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

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
  auto& piano_roll_widget = main_window.piano_roll_widget;

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

// starting playback far to the right of the view's current center should
// make the playhead catch up to the center rather than wait for it
void Tester::test_piano_roll_playhead_starts_past_center() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;

  start_piano_roll_playhead(piano_roll_widget, 1000000.0, 1001000.0);
  QCOMPARE(piano_roll_scene.playhead_transition,
           PlayheadTransition::catching_up);

  stop_piano_roll_playhead(piano_roll_widget);
}

void Tester::test_piano_roll_zoom_actions() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& view_menu = main_window.song_menu_bar.view_menu;

  view_menu.zoom_in_action.trigger();
  QCOMPARE(piano_roll_widget.piano_roll_scene.view.transform().m11(),
           PIANO_ROLL_TIME_ZOOM_STEP);
  view_menu.zoom_out_action.trigger();
  QCOMPARE(piano_roll_widget.piano_roll_scene.view.transform().m11(), 1.0);
}

// a playhead starting left of center holds the view still until it reaches
// the center, and one starting right of center eases the view over for the
// catch-up window -- either way it then just follows the playhead
void Tester::test_piano_roll_playhead_transitions() {
  static const auto FAR_RIGHT_MS = 1000000.0;
  static const auto PAST_CATCHUP_WAIT_MS = 500;

  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;

  start_piano_roll_playhead(piano_roll_widget, 0.0, FAR_RIGHT_MS);
  QCOMPARE(piano_roll_scene.playhead_transition,
           PlayheadTransition::waiting_to_reach_center);
  position_playhead(piano_roll_scene, FAR_RIGHT_MS);
  QCOMPARE(piano_roll_scene.playhead_transition, PlayheadTransition::none);
  stop_piano_roll_playhead(piano_roll_widget);

  start_piano_roll_playhead(piano_roll_widget, FAR_RIGHT_MS,
                            FAR_RIGHT_MS + PAST_CATCHUP_WAIT_MS * 2);
  QCOMPARE(piano_roll_scene.playhead_transition,
           PlayheadTransition::catching_up);
  QTest::qWait(PAST_CATCHUP_WAIT_MS);
  position_playhead(piano_roll_scene, FAR_RIGHT_MS + PAST_CATCHUP_WAIT_MS);
  QCOMPARE(piano_roll_scene.playhead_transition, PlayheadTransition::none);
  stop_piano_roll_playhead(piano_roll_widget);
}

void Tester::test_piano_roll_ctrl_wheel_zoom() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  auto* const viewport_pointer = piano_roll_scene.view.viewport();

  const auto send_wheel = [&piano_roll_widget](
                              QObject* const watched_pointer,
                              const int angle_delta_y,
                              const Qt::KeyboardModifiers modifiers) -> bool {
    QWheelEvent wheel_event(QPointF(), QPointF(), QPoint(),
                            QPoint(0, angle_delta_y), Qt::NoButton, modifiers,
                            Qt::NoScrollPhase, false);
    return piano_roll_widget.eventFilter(watched_pointer, &wheel_event);
  };

  QVERIFY(send_wheel(viewport_pointer, 1, Qt::ControlModifier));
  QCOMPARE(piano_roll_scene.time_zoom_factor, PIANO_ROLL_TIME_ZOOM_STEP);
  QVERIFY(send_wheel(viewport_pointer, -1, Qt::ControlModifier));
  QCOMPARE(piano_roll_scene.time_zoom_factor, 1.0);
  // a purely horizontal ctrl+scroll is still swallowed, but doesn't zoom
  QVERIFY(send_wheel(viewport_pointer, 0, Qt::ControlModifier));
  QCOMPARE(piano_roll_scene.time_zoom_factor, 1.0);

  // without ctrl, or outside the notes viewport, the wheel scrolls as usual
  QVERIFY(!send_wheel(viewport_pointer, 1, Qt::NoModifier));
  QVERIFY(!send_wheel(&piano_roll_widget, 1, Qt::ControlModifier));
  QCOMPARE(piano_roll_scene.time_zoom_factor, 1.0);
}

// clicking takes the playhead over from playback, the same as dragging it
void Tester::test_piano_roll_click_stops_playhead() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  select_cell(switch_table, 0, 0);
  start_piano_roll_playhead(piano_roll_widget, 0.0, 1800.0);
  QVERIFY(piano_roll_scene.playhead_active);

  // chord 1 starts at 600ms, per test_piano_roll_time_bounds() above
  const QPointF chord_1_pos(600.0 * PIANO_ROLL_PIXELS_PER_MS, 0);
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonPress, chord_1_pos,
                                      Qt::LeftButton));
  QVERIFY(!piano_roll_scene.playhead_active);
  QCOMPARE(get_only_range(switch_table).top(), 1);
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonRelease, chord_1_pos,
                                      Qt::NoButton));
}

void Tester::test_piano_roll_right_click_ignored() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  select_cell(switch_table, 0, 0);
  QVERIFY(!send_piano_roll_mouse_event(
      piano_roll_widget, QEvent::MouseButtonPress,
      QPointF(1200.0 * PIANO_ROLL_PIXELS_PER_MS, 0), Qt::RightButton));
  QVERIFY(!piano_roll_widget.piano_roll_scene.playhead_dragging);
  QCOMPARE(get_only_range(switch_table).top(), 0);
}

void Tester::test_piano_roll_click_bar_in_chords_mode_data() {
  QTest::addColumn<bool>("is_pitched");
  QTest::addColumn<int>("note_number");

  QTest::newRow("pitched") << true << 2;
  QTest::newRow("unpitched") << false << 1;
}

// while the table shows chords, clicking a note bar selects its chord
// rather than switching the table over to that note's row
void Tester::test_piano_roll_click_bar_in_chords_mode() {
  QFETCH(const bool, is_pitched);
  QFETCH(const int, note_number);

  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  // already on the bar's chord, so the chord selection is left alone too
  select_cell(switch_table, 1, 0);
  const auto maybe_bar_center =
      get_note_bar_center(piano_roll_widget, 1, note_number, is_pitched);
  if (!maybe_bar_center.has_value()) {
    QFAIL("no note bar to click");
  }
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonPress,
                                      *maybe_bar_center, Qt::LeftButton));
  QCOMPARE(switch_table.delegate.current_row_type, RowType::chord_type);
  QCOMPARE(get_only_range(switch_table).top(), 1);
  QCOMPARE(get_only_range(switch_table).bottom(), 1);
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonRelease,
                                      *maybe_bar_center, Qt::NoButton));
}

// once playback runs past the end, the playhead stops there and selects the
// chord it ended on -- without that selection yanking the playhead back to
// the chord's start
void Tester::test_piano_roll_playhead_reaches_end() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  select_cell(switch_table, 0, 0);
  // chord 2 starts at 1200ms, per test_piano_roll_time_bounds() above; an
  // end time already reached means the first tick finishes playback
  start_piano_roll_playhead(piano_roll_widget, 1200.0, 1200.0);
  update_playhead_position(piano_roll_scene, piano_roll_widget.axis_scene,
                           switch_table,
                           piano_roll_widget.selecting_chord_from_playhead);
  QVERIFY(!piano_roll_scene.playhead_active);
  QCOMPARE(get_only_range(switch_table).top(), 2);
  QCOMPARE(piano_roll_scene.playhead_item.line().x1(),
           1200.0 * PIANO_ROLL_PIXELS_PER_MS);

  // a stray tick after playback stopped is ignored
  select_cell(switch_table, 0, 0);
  update_playhead_position(piano_roll_scene, piano_roll_widget.axis_scene,
                           switch_table,
                           piano_roll_widget.selecting_chord_from_playhead);
  QCOMPARE(get_only_range(switch_table).top(), 0);
}

// a clip shorter than the catch-up window eases the view toward where
// playback actually ends, not past it
void Tester::test_piano_roll_short_clip_catch_up() {
  static const auto FAR_RIGHT_MS = 1000000.0;
  static const auto SHORT_CLIP_MS = 100.0;

  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;

  start_piano_roll_playhead(piano_roll_widget, FAR_RIGHT_MS,
                            FAR_RIGHT_MS + SHORT_CLIP_MS);
  QCOMPARE(piano_roll_scene.playhead_transition,
           PlayheadTransition::catching_up);
  position_playhead(piano_roll_scene, FAR_RIGHT_MS);
  QCOMPARE(piano_roll_scene.playhead_transition,
           PlayheadTransition::catching_up);
  stop_piano_roll_playhead(piano_roll_widget);
}

// events on anything other than the notes viewport pass through untouched,
// even mid-drag
void Tester::test_piano_roll_ignores_other_widgets() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  select_cell(switch_table, 0, 0);
  const QPointF chord_1_pos(600.0 * PIANO_ROLL_PIXELS_PER_MS, 0);

  const auto send_to_widget =
      [&piano_roll_widget](const QEvent::Type event_type) -> bool {
    QMouseEvent mouse_event(event_type, QPointF(), QPointF(), Qt::LeftButton,
                            Qt::LeftButton, Qt::NoModifier);
    return piano_roll_widget.eventFilter(&piano_roll_widget, &mouse_event);
  };

  QVERIFY(!send_to_widget(QEvent::MouseButtonDblClick));
  QVERIFY(!send_to_widget(QEvent::MouseButtonPress));
  QVERIFY(!piano_roll_scene.playhead_dragging);
  // hovering the notes viewport without a drag in progress does nothing
  QVERIFY(!send_piano_roll_mouse_event(piano_roll_widget, QEvent::MouseMove,
                                       chord_1_pos, Qt::NoButton));
  QCOMPARE(get_only_range(switch_table).top(), 0);

  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonPress, chord_1_pos,
                                      Qt::LeftButton));
  QVERIFY(piano_roll_scene.playhead_dragging);
  QVERIFY(!send_to_widget(QEvent::MouseMove));
  QVERIFY(!send_to_widget(QEvent::MouseButtonRelease));
  QVERIFY(piano_roll_scene.playhead_dragging);
  QCOMPARE(get_only_range(switch_table).top(), 1);

  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonRelease, chord_1_pos,
                                      Qt::NoButton));
  QVERIFY(!piano_roll_scene.playhead_dragging);
}

// double-clicking empty space, away from every note bar, opens nothing
void Tester::test_piano_roll_double_click_empty_space() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  // far below every bar, in the first chord
  const QPointF empty_pos(
      1.0, piano_roll_widget.piano_roll_scene.sceneRect().bottom() - 1.0);
  QVERIFY(!send_piano_roll_mouse_event(piano_roll_widget,
                                       QEvent::MouseButtonDblClick, empty_pos,
                                       Qt::LeftButton));
  QCOMPARE(switch_table.delegate.current_row_type, RowType::chord_type);
}

// dragging past the start of the song clamps to the first chord
void Tester::test_piano_roll_drag_before_start() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& piano_roll_scene = piano_roll_widget.piano_roll_scene;
  auto& view = piano_roll_scene.view;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  const QPointF chord_1_pos(600.0 * PIANO_ROLL_PIXELS_PER_MS, 0);
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonPress, chord_1_pos,
                                      Qt::LeftButton));
  QCOMPARE(get_only_range(switch_table).top(), 1);

  // far enough left of the viewport to be before time 0 in the scene
  const QPoint before_start_view_pos(
      view.mapFromScene(QPointF(0, 0)).x() - 1000, 0);
  QMouseEvent move_event(
      QEvent::MouseMove, QPointF(before_start_view_pos),
      QPointF(
          get_reference(view.viewport()).mapToGlobal(before_start_view_pos)),
      Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
  QVERIFY(piano_roll_widget.eventFilter(view.viewport(), &move_event));
  QCOMPARE(get_only_range(switch_table).top(), 0);
  QCOMPARE(get_only_range(switch_table).bottom(), 1);
  QCOMPARE(piano_roll_scene.playhead_item.line().x1(), 0.0);

  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonRelease, chord_1_pos,
                                      Qt::NoButton));
}

// a press with nothing selected yet still selects the chord under it
void Tester::test_piano_roll_click_without_selection() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  get_selection_model(switch_table).clear();
  const QPointF chord_1_pos(600.0 * PIANO_ROLL_PIXELS_PER_MS, 0);
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonPress, chord_1_pos,
                                      Qt::LeftButton));
  QCOMPARE(get_only_range(switch_table).top(), 1);
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonRelease, chord_1_pos,
                                      Qt::NoButton));
}

// clicking a note bar replaces a different whole-row note selection
void Tester::test_piano_roll_click_replaces_note_row() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;
  auto& undo_stack = main_window.window_body.undo_stack;

  switch_to(main_window, RowType::pitched_note_type, 1);
  get_selection_model(switch_table)
      .select(switch_table.pitched_notes_model.index(0, 0),
              QItemSelectionModel::Select | QItemSelectionModel::Clear |
                  QItemSelectionModel::Rows);

  const auto maybe_bar_center =
      get_note_bar_center(piano_roll_widget, 1, 2, true);
  if (!maybe_bar_center.has_value()) {
    QFAIL("no note bar to click");
  }
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonPress,
                                      *maybe_bar_center, Qt::LeftButton));
  QCOMPARE(get_only_range(switch_table).top(), 2);
  QVERIFY(send_piano_roll_mouse_event(piano_roll_widget,
                                      QEvent::MouseButtonRelease,
                                      *maybe_bar_center, Qt::NoButton));

  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
}

// a song without chords has nothing for a click to select
void Tester::test_piano_roll_click_empty_song() {
  auto& piano_roll_widget = main_window.piano_roll_widget;
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  open_text(main_window, make_voice_song_xml({"A"}, {"D"}));
  QVERIFY(send_piano_roll_mouse_event(
      piano_roll_widget, QEvent::MouseButtonPress, QPointF(), Qt::LeftButton));
  QVERIFY(get_selection_model(switch_table).selection().empty());
  QVERIFY(send_piano_roll_mouse_event(
      piano_roll_widget, QEvent::MouseButtonRelease, QPointF(), Qt::NoButton));

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// a note too short to see at the current zoom still gets a visible bar
void Tester::test_piano_roll_minimum_bar_width() {
  auto& piano_roll_widget = main_window.piano_roll_widget;

  // 1/100 of a beat at 100 bpm is 6ms, well under a pixel wide
  open_text(main_window,
            make_voice_song_xml({"A"}, {"D"}, {{{0}, {}}})
                .replace("</voice_name></pitched_note>",
                         "</voice_name><beats><numerator>1</numerator>"
                         "<denominator>100</denominator></beats>"
                         "</pitched_note>"));
  const auto& note_items = piano_roll_widget.piano_roll_scene.note_items;
  QCOMPARE(note_items.size(), 1);
  QCOMPARE(get_reference(note_items.at(0)).rect().width(), 1.0);

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}
