#pragma once

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>

#include "cell_types/Interval.hpp"

struct RationalEditor;

static const auto MAX_OCTAVE = 9;

struct IntervalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

 public:
  RationalEditor& rational_editor;

  QWidget& o_text = *(new QLabel("o"));
  QSpinBox& octave_box = *(new QSpinBox);
  QBoxLayout& row_layout = *(new QHBoxLayout(this));

  explicit IntervalEditor(QWidget* parent_pointer);

  [[nodiscard]] auto value() const -> Interval;

  void setValue(const Interval& new_value) const;
};
