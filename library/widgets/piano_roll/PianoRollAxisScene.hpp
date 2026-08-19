#pragma once

#include <QtWidgets/QGraphicsScene>

#include "other/helpers.hpp"

class QGraphicsView;
class QWidget;

// a second, fixed-width view pinned to the left edge, showing just the pitch
// axis' ticks/labels (drawn into this scene by
// PianoRollWidget::rebuild_scene()); its vertical scroll is kept in lockstep
// with the main PianoRollNotesScene's (wired up by PianoRollWidget), and both
// scenes place items using the same y = -midi * PIANO_ROLL_PIXELS_PER_SEMITONE
// formula, so the pitch labels stay lined up with their notes no matter how
// far either view is scrolled vertically
//
// is-a QGraphicsScene (rather than holding one by reference) so it can be
// heap-allocated and parented to parent_widget directly, the same way
// PianoRollWidget itself is heap-allocated and referenced from SongEditor --
// a QGraphicsScene isn't a widget, so unlike view below it can't pick up
// ownership by being added to a layout; it has to be parented explicitly
struct PianoRollAxisScene : public QGraphicsScene {
  QGraphicsView& view;

  explicit PianoRollAxisScene(QWidget& parent_widget);

  ~PianoRollAxisScene() override = default;

  NO_MOVE_COPY(PianoRollAxisScene)
};
