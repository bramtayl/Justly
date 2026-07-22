#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <algorithm>

#include "other/PianoRoll.hpp"
#include "widgets/SongWidget.hpp"

static const auto PIANO_ROLL_PIXELS_PER_MS = 0.1;
static const auto PIANO_ROLL_PIXELS_PER_SEMITONE = 6;
static const auto PIANO_ROLL_LANE_HEIGHT = 20;
static const auto PIANO_ROLL_NOTE_BAR_THICKNESS = 3.0;
static const auto PIANO_ROLL_UNPITCHED_LANE_TOP = 20.0;
static const auto PIANO_ROLL_MIN_BAR_WIDTH = 1.0;
static const auto PIANO_ROLL_TIMER_INTERVAL_MS = 33;
static const auto PIANO_ROLL_MAX_HEIGHT = 300;
static const auto PIANO_ROLL_SCENE_MARGIN = 10.0;

struct PianoRollWidget : public QWidget {
  const SongWidget &song_widget;

  QGraphicsScene &scene = *(new QGraphicsScene(this));
  QGraphicsView &view = *(new QGraphicsView(&scene, this));
  QBoxLayout &column_layout = *(new QVBoxLayout(this));
  QGraphicsLineItem &playhead_item = *(new QGraphicsLineItem);

  QTimer &playhead_timer = *(new QTimer(this));
  QElapsedTimer playhead_elapsed_timer;
  double playhead_baseline_ms = 0;
  double playhead_end_ms = 0;
  bool playhead_active = false;

  explicit PianoRollWidget(const SongWidget &song_widget_input)
      : song_widget(song_widget_input) {
    // a bottom dock would otherwise grow to fill all leftover window space;
    // the view's own scrollbars handle content taller than this
    setMaximumHeight(PIANO_ROLL_MAX_HEIGHT);

    column_layout.addWidget(&view);

    playhead_item.setPen(QPen(Qt::red));
    playhead_item.setZValue(1);
    playhead_item.hide();
    scene.addItem(&playhead_item);

    QObject::connect(&playhead_timer, &QTimer::timeout, this,
                     [this]() { update_playhead_position(); });

    rebuild_scene();
  }

  void rebuild_scene() {
    scene.removeItem(&playhead_item);
    const auto saved_line = playhead_item.line();
    const auto was_visible = playhead_item.isVisible();

    scene.clear();

    const auto &song = song_widget.song;
    for (const auto &event : get_piano_roll_events(song)) {
      const auto x = event.start_time_ms * PIANO_ROLL_PIXELS_PER_MS;
      const auto width =
          std::max(PIANO_ROLL_MIN_BAR_WIDTH,
                   event.duration_ms * PIANO_ROLL_PIXELS_PER_MS);

      const auto is_pitched = event.kind == PianoRollNoteKind::pitched_kind;
      const auto lane_y =
          is_pitched
              ? -frequency_to_midi_number(event.frequency) *
                    PIANO_ROLL_PIXELS_PER_SEMITONE
              : PIANO_ROLL_UNPITCHED_LANE_TOP +
                    (event.voice_number * PIANO_ROLL_LANE_HEIGHT);
      const auto bar_y =
          lane_y + ((PIANO_ROLL_LANE_HEIGHT - PIANO_ROLL_NOTE_BAR_THICKNESS) /
                   2);

      scene.addRect(x, bar_y, width, PIANO_ROLL_NOTE_BAR_THICKNESS,
                   QPen(Qt::NoPen),
                   QBrush(is_pitched ? Qt::blue : Qt::darkGray));
    }

    const auto &unpitched_voices = song.unpitched_voices;
    for (auto voice_number = 0; voice_number < unpitched_voices.size();
        voice_number = voice_number + 1) {
      auto &label = get_reference(scene.addSimpleText(
          unpitched_voices.at(voice_number).name));
      label.setPos(0, PIANO_ROLL_UNPITCHED_LANE_TOP +
                          (voice_number * PIANO_ROLL_LANE_HEIGHT));
    }

    scene.addItem(&playhead_item);
    playhead_item.setLine(saved_line);
    playhead_item.setVisible(was_visible);

    scene.setSceneRect(scene.itemsBoundingRect().adjusted(
        -PIANO_ROLL_SCENE_MARGIN, -PIANO_ROLL_SCENE_MARGIN,
        PIANO_ROLL_SCENE_MARGIN, PIANO_ROLL_SCENE_MARGIN));
  }

  void position_playhead(const double time_ms) {
    const auto x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
    const auto &scene_rect = scene.sceneRect();
    playhead_item.setLine(x, scene_rect.top(), x, scene_rect.bottom());
  }

  void start_playhead(const double baseline_ms, const double end_ms) {
    playhead_baseline_ms = baseline_ms;
    playhead_end_ms = end_ms;
    playhead_elapsed_timer.restart();
    playhead_active = true;
    playhead_item.show();
    position_playhead(baseline_ms);
    playhead_timer.start(PIANO_ROLL_TIMER_INTERVAL_MS);
  }

  void stop_playhead() {
    playhead_timer.stop();
    playhead_active = false;
    playhead_item.hide();
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
      position_playhead(playhead_end_ms);
      return;
    }
    position_playhead(current_ms);
  }
};
