#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QWidget>

#include "other/helpers.hpp"

static const auto PIANO_ROLL_AXIS_LABEL_GAP = 4.0;
static const auto PIANO_ROLL_LEGEND_SWATCH_SIZE = 10.0;

// a separate scene/view for the voice legend, laid out in its own
// fixed-width column to the right of the main view -- pinned there just
// like PianoRollAxisView is pinned to the left, so the legend stays visible
// and in the same place no matter how far the main view is scrolled, rather
// than being drawn into the scrollable main scene where it used to
// disappear off-screen
struct PianoRollLegendView {
  QGraphicsScene &scene;
  QGraphicsView &view;

  explicit PianoRollLegendView(QWidget &parent_widget)
      : scene(*(new QGraphicsScene(&parent_widget))),
        view(*(new QGraphicsView(&scene, &parent_widget))) {
    // this view never scrolls horizontally (it's a fixed-width column) and
    // is left free to scroll vertically on its own -- independent of the
    // main view -- so a long voice list stays reachable without the
    // legend's on-screen position ever moving
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFocusPolicy(Qt::NoFocus);
  }

  ~PianoRollLegendView() = default;

  NO_MOVE_COPY(PianoRollLegendView)

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
};

static void draw_legend_row(QGraphicsScene &legend_scene, const QString &name,
                            const int global_voice_index, const double row_y) {
  legend_scene.addRect(0, row_y, PIANO_ROLL_LEGEND_SWATCH_SIZE,
               PIANO_ROLL_LEGEND_SWATCH_SIZE, QPen(Qt::NoPen),
               QBrush(PianoRollLegendView::get_voice_color(global_voice_index)));
  auto &label = get_reference(legend_scene.addSimpleText(name));
  label.setPos(PIANO_ROLL_LEGEND_SWATCH_SIZE + PIANO_ROLL_AXIS_LABEL_GAP,
              row_y - ((label.boundingRect().height() -
                       PIANO_ROLL_LEGEND_SWATCH_SIZE) /
                      2));
}
