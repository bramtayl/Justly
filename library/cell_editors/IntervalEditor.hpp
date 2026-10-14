#pragma once

#include <QtWidgets/QWidget>

#include "cell_types/Interval.hpp"

class QSpinBox;
struct RationalEditor;

struct IntervalEditor : QWidget {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

 public:
  RationalEditor& rational_editor;
  QSpinBox& octave_box;

  explicit IntervalEditor(QWidget* parent_pointer);

  [[nodiscard]] auto value() const -> Interval;

  void setValue(const Interval& new_value) const;
};
