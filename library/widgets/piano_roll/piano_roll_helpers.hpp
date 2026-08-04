#pragma once

#include <QtCore/QList>
#include <QtGui/QColor>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QScrollBar>

static const auto PIANO_ROLL_PIXELS_PER_MS = 0.1;
static const auto PIANO_ROLL_PIXELS_PER_SEMITONE = 6;
// how far below the lowest note the horizontal axis sits -- enough that the
// lowest note's bar never reads as glued to (or nearly touching) the axis
// line, without wasting a full octave of empty space underneath it
static const auto PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES = 3.0;
static const auto PIANO_ROLL_LANE_HEIGHT = 20;
static const auto PIANO_ROLL_NOTE_BAR_THICKNESS = 3.0;
static const auto PIANO_ROLL_MIN_BAR_WIDTH = 1.0;
static const auto PIANO_ROLL_TIMER_INTERVAL_MS = 33;
// how long the view takes to catch up to the playhead when playback starts
// with the playhead already right of center (see
// PianoRollNotesView::position_playhead()) -- long enough to read as a
// deliberate scroll, short enough not to lag behind what's actually playing
static const auto PIANO_ROLL_PLAYHEAD_CATCHUP_MS = 400.0;
static const auto PIANO_ROLL_MIN_HEIGHT = 300;
static const auto PIANO_ROLL_SCENE_MARGIN = 10.0;
static const auto PIANO_ROLL_AXIS_TICK_LENGTH = 5.0;
static const auto PIANO_ROLL_AXIS_LABEL_GAP = 4.0;
static const auto PIANO_ROLL_AXIS_X = 0.0;
static const auto PIANO_ROLL_DEFAULT_AXIS_Y = 0.0;
static const auto PIANO_ROLL_UNPITCHED_LANE_GAP = 30.0;
static const auto PIANO_ROLL_LEGEND_GAP = 10.0;
static const auto PIANO_ROLL_LEGEND_SWATCH_SIZE = 10.0;
// ticks are re-spaced on every zoom change (see
// PianoRollNotesView::redraw_time_axis_ticks()) to keep roughly this many screen
// pixels between them, rather than a fixed time interval -- otherwise
// zooming in would crowd ticks together and zooming out would spread them so
// far apart that most of the timeline carries no labels at all
static const auto PIANO_ROLL_TARGET_TICK_PIXEL_SPACING = 80.0;
static const auto PIANO_ROLL_MS_PER_SECOND = 1000.0;
static const auto PIANO_ROLL_MS_PER_MINUTE = 60000.0;
static const auto PIANO_ROLL_SECONDS_PER_MINUTE = 60LL;
static const auto PIANO_ROLL_LABEL_DECIMAL_BASE = 10;
static const auto PIANO_ROLL_NICE_STEP_ROLLOVER = 10.0;
static const auto PIANO_ROLL_HIGHLIGHT_PEN_WIDTH = 1.5;
static const auto PIANO_ROLL_MIN_TIME_ZOOM = 0.25;
static const auto PIANO_ROLL_MAX_TIME_ZOOM = 8.0;
static const auto PIANO_ROLL_TIME_ZOOM_STEP = 1.25;

[[nodiscard]] static auto get_voice_color(const int global_voice_index)
    -> QColor {
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

static void set_vertical_scrolling_enabled(QGraphicsView &view,
                                           const bool enabled) {
  view.verticalScrollBar()->setEnabled(enabled);
}
