#pragma once

#include <QtCore/Qt>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QWidget>
#include <cmath>

#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/PitchedNote.hpp"
#include "widgets/piano_roll/piano_roll_helpers.hpp"

// draws a tick + note-name label at every octave (C) at or above the
// lowest pitched note present, up through the highest octave that still
// fits within a fixed margin above the highest note -- ticks beyond either
// margin would just sit off the visible graph, so they're skipped rather
// than drawn there; returns the y position of the horizontal time axis, a
// fixed few semitones below the lowest note (not snapped to any tick), so
// the lowest note's bar never reads as glued to the axis line
[[nodiscard]] static auto draw_pitch_axis(QGraphicsScene &scene,
                                          const double min_midi,
                                          const double max_midi) -> double {
  if (min_midi > max_midi) {
    return PIANO_ROLL_DEFAULT_AXIS_Y;
  }
  const auto axis_pitch =
      min_midi - PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES;
  const auto top_pitch =
      max_midi + PIANO_ROLL_AXIS_PITCH_MARGIN_SEMITONES;
  const auto axis_y = -axis_pitch * PIANO_ROLL_PIXELS_PER_SEMITONE;
  const auto first_octave =
      C_0_MIDI + to_int(std::ceil((axis_pitch - C_0_MIDI) /
                                  HALFSTEPS_PER_OCTAVE)) *
                     HALFSTEPS_PER_OCTAVE;
  const auto last_octave =
      C_0_MIDI + to_int(std::floor((top_pitch - C_0_MIDI) /
                                   HALFSTEPS_PER_OCTAVE)) *
                     HALFSTEPS_PER_OCTAVE;

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

  return axis_y;
}

// a second, fixed-width view onto the main PianoRollNotesView's scene, locked so
// it only ever shows the pitch axis' column (x <= PIANO_ROLL_AXIS_X); its
// vertical scroll is kept in lockstep with the main view's (wired up by
// PianoRollWidget), so the pitch labels stay pinned to the left edge -- and
// lined up with their notes -- no matter how far the main view is scrolled
// horizontally
struct PianoRollAxisView {
  QGraphicsView &view;

  PianoRollAxisView(QWidget &parent_widget, QGraphicsScene &scene)
      : view(*(new QGraphicsView(&scene, &parent_widget))) {
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFocusPolicy(Qt::NoFocus);
    // each QGraphicsView draws its own sunken frame by default, which shows
    // up as a gray seam between this view and the main view even with the
    // layout's spacing at 0 -- dropping both frames removes that seam while
    // leaving the two views' contents flush against each other
    view.setFrameShape(QFrame::NoFrame);
  }

  NO_MOVE_COPY(PianoRollAxisView)

  void set_scrolling_enabled(const bool enabled) {
    view.verticalScrollBar()->setEnabled(enabled);
  }
};
