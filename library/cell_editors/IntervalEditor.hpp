#pragma once

#include <QtWidgets/QFrame>

#include "cell_types/Interval.hpp"

class QBoxLayout;
class QSpinBox;
struct RationalEditor;

static const auto MAX_OCTAVE = 9;

struct IntervalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

 public:
  RationalEditor& rational_editor;

  QWidget& o_text;
  QSpinBox& octave_box;
  QBoxLayout& row_layout;

  explicit IntervalEditor(QWidget* parent_pointer);

  [[nodiscard]] auto value() const -> Interval;

  void setValue(const Interval& new_value) const;
};
