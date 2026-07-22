#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <limits>

#include "other/PianoRoll.hpp"
#include "widgets/SongWidget.hpp"

static const auto PIANO_ROLL_PIXELS_PER_MS = 0.1;
static const auto PIANO_ROLL_PIXELS_PER_SEMITONE = 6;
static const auto PIANO_ROLL_LANE_HEIGHT = 20;
static const auto PIANO_ROLL_NOTE_BAR_THICKNESS = 3.0;
static const auto PIANO_ROLL_MIN_BAR_WIDTH = 1.0;
static const auto PIANO_ROLL_TIMER_INTERVAL_MS = 33;
static const auto PIANO_ROLL_MIN_HEIGHT = 300;
static const auto PIANO_ROLL_SCENE_MARGIN = 10.0;
static const auto PIANO_ROLL_AXIS_TICK_LENGTH = 5.0;
static const auto PIANO_ROLL_AXIS_LABEL_GAP = 4.0;
static const auto PIANO_ROLL_AXIS_X = 0.0;
static const auto PIANO_ROLL_DEFAULT_AXIS_Y = 0.0;
static const auto PIANO_ROLL_UNPITCHED_LANE_GAP = 30.0;
static const auto PIANO_ROLL_LEGEND_GAP = 10.0;
static const auto PIANO_ROLL_LEGEND_SWATCH_SIZE = 10.0;
static const auto PIANO_ROLL_TIME_AXIS_STEP_MS = 500.0;
static const auto PIANO_ROLL_MS_PER_SECOND = 1000.0;

// fixed categorical order (never cycled) -- a voice beyond the 8th falls
// back to PIANO_ROLL_OTHER_VOICE_COLOR rather than reusing an earlier hue
static const QList<QColor> PIANO_ROLL_VOICE_COLORS{
    QColor("#2a78d6"), QColor("#eb6834"), QColor("#1baf7a"), QColor("#eda100"),
    QColor("#e87ba4"), QColor("#008300"), QColor("#4a3aa7"), QColor("#e34948"),
};
static const auto PIANO_ROLL_OTHER_VOICE_COLOR = QColor("#898781");

[[nodiscard]] static auto get_voice_color(const int global_voice_index)
    -> QColor {
  if (global_voice_index >= 0 &&
      global_voice_index < PIANO_ROLL_VOICE_COLORS.size()) {
    return PIANO_ROLL_VOICE_COLORS.at(global_voice_index);
  }
  return PIANO_ROLL_OTHER_VOICE_COLOR;
}

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
    // a bottom dock would otherwise default to a cramped sliver; this keeps
    // it usable out of the box while still letting the user drag it taller
    // (or shorter, down to this floor) via the splitter
    setMinimumHeight(PIANO_ROLL_MIN_HEIGHT);

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
    const auto &pitched_voices = song.pitched_voices;
    const auto &unpitched_voices = song.unpitched_voices;
    const auto number_of_pitched_voices =
        static_cast<int>(pitched_voices.size());

    const auto events = get_piano_roll_events(song);

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
    const auto number_of_unpitched_lanes =
        static_cast<int>(lane_end_times.size());

    // the horizontal axis sits at the same y as the lowest pitch tick, and
    // both axes sit at x/y == PIANO_ROLL_AXIS_X, so the axes' first ticks
    // (the lowest pitch tick, and the t=0 time tick) meet at one corner
    const auto axis_y = draw_pitch_axis(min_midi, max_midi);
    draw_time_axis(max_time_ms, axis_y);
    const auto unpitched_lane_top = axis_y + PIANO_ROLL_UNPITCHED_LANE_GAP;

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
      scene.addRect(bar_x, bar_y, width, PIANO_ROLL_NOTE_BAR_THICKNESS,
                   QPen(Qt::NoPen), QBrush(get_voice_color(global_voice_index)));
    }

    draw_legend(pitched_voices, unpitched_voices,
               unpitched_lane_top +
                   (number_of_unpitched_lanes * PIANO_ROLL_LANE_HEIGHT) +
                   PIANO_ROLL_LEGEND_GAP);

    scene.addItem(&playhead_item);
    playhead_item.setLine(saved_line);
    playhead_item.setVisible(was_visible);

    scene.setSceneRect(scene.itemsBoundingRect().adjusted(
        -PIANO_ROLL_SCENE_MARGIN, -PIANO_ROLL_SCENE_MARGIN,
        PIANO_ROLL_SCENE_MARGIN, PIANO_ROLL_SCENE_MARGIN));
  }

  // lists every voice (pitched first, then unpitched) as a colored swatch +
  // name, in the same order used to assign global_voice_index for coloring
  void draw_legend(const QList<PitchedVoice> &pitched_voices,
                   const QList<UnpitchedVoice> &unpitched_voices,
                   const double top_y) {
    auto row_y = top_y;
    auto global_voice_index = 0;
    for (const auto &voice : pitched_voices) {
      draw_legend_row(voice.name, global_voice_index, row_y);
      row_y = row_y + PIANO_ROLL_LANE_HEIGHT;
      global_voice_index = global_voice_index + 1;
    }
    for (const auto &voice : unpitched_voices) {
      draw_legend_row(voice.name, global_voice_index, row_y);
      row_y = row_y + PIANO_ROLL_LANE_HEIGHT;
      global_voice_index = global_voice_index + 1;
    }
  }

  void draw_legend_row(const QString &name, const int global_voice_index,
                       const double row_y) {
    scene.addRect(0, row_y, PIANO_ROLL_LEGEND_SWATCH_SIZE,
                 PIANO_ROLL_LEGEND_SWATCH_SIZE, QPen(Qt::NoPen),
                 QBrush(get_voice_color(global_voice_index)));
    auto &label = get_reference(scene.addSimpleText(name));
    label.setPos(PIANO_ROLL_LEGEND_SWATCH_SIZE + PIANO_ROLL_AXIS_LABEL_GAP,
                row_y - ((label.boundingRect().height() -
                         PIANO_ROLL_LEGEND_SWATCH_SIZE) /
                        2));
  }

  // draws a tick + note-name label at every octave (C) spanning the range of
  // pitched notes actually present, with no padding beyond that range;
  // returns the y position of the lowest (bottommost) pitch tick, which is
  // where the horizontal time axis should sit
  [[nodiscard]] auto draw_pitch_axis(const double min_midi,
                                     const double max_midi) -> double {
    if (min_midi > max_midi) {
      return PIANO_ROLL_DEFAULT_AXIS_Y;
    }
    const auto first_octave =
        C_0_MIDI + to_int(std::floor((min_midi - C_0_MIDI) /
                                     HALFSTEPS_PER_OCTAVE)) *
                       HALFSTEPS_PER_OCTAVE;
    const auto last_octave =
        C_0_MIDI + to_int(std::ceil((max_midi - C_0_MIDI) /
                                    HALFSTEPS_PER_OCTAVE)) *
                       HALFSTEPS_PER_OCTAVE;
    const auto axis_y = -first_octave * PIANO_ROLL_PIXELS_PER_SEMITONE;

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

    scene.addLine(PIANO_ROLL_AXIS_X,
                 -last_octave * PIANO_ROLL_PIXELS_PER_SEMITONE,
                 PIANO_ROLL_AXIS_X, axis_y);
    return axis_y;
  }

  // draws a tick + seconds label every PIANO_ROLL_TIME_AXIS_STEP_MS along a
  // horizontal axis line placed between the pitched notes above and the
  // unpitched lanes below
  void draw_time_axis(const double max_time_ms, const double axis_y) {
    scene.addLine(PIANO_ROLL_AXIS_X, axis_y,
                 max_time_ms * PIANO_ROLL_PIXELS_PER_MS, axis_y);

    for (auto step_number = 0;
        step_number * PIANO_ROLL_TIME_AXIS_STEP_MS <= max_time_ms;
        step_number = step_number + 1) {
      const auto time_ms = step_number * PIANO_ROLL_TIME_AXIS_STEP_MS;
      const auto tick_x = time_ms * PIANO_ROLL_PIXELS_PER_MS;
      scene.addLine(tick_x, axis_y, tick_x,
                   axis_y + PIANO_ROLL_AXIS_TICK_LENGTH);

      auto &label = get_reference(scene.addSimpleText(
          QString::number(time_ms / PIANO_ROLL_MS_PER_SECOND, 'f', 1) + "s"));
      label.setPos(tick_x - (label.boundingRect().width() / 2),
                  axis_y + PIANO_ROLL_AXIS_TICK_LENGTH +
                      PIANO_ROLL_AXIS_LABEL_GAP);
    }
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
