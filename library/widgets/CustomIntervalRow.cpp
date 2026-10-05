#include "widgets/CustomIntervalRow.hpp"

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "widgets/IntervalRow.hpp"

namespace {

// show which preset, if any, the user typed; otherwise, show "Custom"
void select_matching_preset(QComboBox& presets_box,
                            const IntervalEditor& interval_editor) {
  // don't loop back into interval_editor
  const QSignalBlocker blocker(presets_box);
  const auto& just_scale = get_just_scale();
  const auto value = interval_editor.value();
  for (auto index = 0; index < presets_box.count(); index++) {
    if (Interval(just_scale[presets_box.itemData(index).toInt()].ratio, 0) ==
        value) {
      presets_box.setCurrentIndex(index);
      return;
    }
  }
  presets_box.setCurrentIndex(-1);
}

}  // namespace

CustomIntervalRow::CustomIntervalRow(QUndoStack& undo_stack_input,
                                     SwitchTable& switch_table_input)
    : undo_stack(undo_stack_input),
      switch_table(switch_table_input),
      minus_button(*(new QPushButton("−", this))),
      interval_editor(*(new IntervalEditor(this))),
      plus_button(*(new QPushButton("+", this))),
      presets_box(*(new QComboBox(this))) {
  static const auto MINOR_THIRD_HALFSTEPS = 3;

  make_square(minus_button);
  make_square(plus_button);
  presets_box.setPlaceholderText(QObject::tr("Custom"));

  auto& row_layout =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QGridLayout(this));
  row_layout.addWidget(&minus_button, 0, 0);
  row_layout.addWidget(&interval_editor, 0, 1);
  row_layout.addWidget(&plus_button, 0, 2);
  row_layout.addWidget(&presets_box, 1, 1);

  const auto& just_scale = get_just_scale();
  // skip unison, which wouldn't change anything, and the major third and
  // perfect fifth, which already have their own rows
  for (auto halfsteps = 1; halfsteps < just_scale.size(); halfsteps++) {
    if (halfsteps == MAJOR_THIRD_HALFSTEPS ||
        halfsteps == PERFECT_FIFTH_HALFSTEPS) {
      continue;
    }
    const auto& named_ratio = just_scale[halfsteps];
    const auto ratio = named_ratio.ratio;
    QString text;
    QTextStream stream(&text);
    stream << named_ratio.name << " (" << ratio.numerator << "/"
           << ratio.denominator << ")";
    presets_box.addItem(text, halfsteps);
  }

  QObject::connect(
      &presets_box, &QComboBox::currentIndexChanged, this,
      [this, &just_scale](int index) -> auto {
        interval_editor.setValue(
            Interval(just_scale[presets_box.itemData(index).toInt()].ratio, 0));
      });

  for (auto* const spin_box_pointer :
       {&interval_editor.rational_editor.numerator_box,
        &interval_editor.rational_editor.denominator_box,
        &interval_editor.octave_box}) {
    QObject::connect(spin_box_pointer, &QSpinBox::valueChanged, this,
                     [this]() -> auto {
                       select_matching_preset(presets_box, interval_editor);
                     });
  }

  interval_editor.setValue(
      Interval(just_scale[MINOR_THIRD_HALFSTEPS].ratio, 0));

  QObject::connect(&minus_button, &QPushButton::released, this,
                   [this]() -> auto {
                     update_interval(undo_stack, switch_table,
                                     Interval() / interval_editor.value());
                   });

  QObject::connect(
      &plus_button, &QPushButton::released, this, [this]() -> auto {
        update_interval(undo_stack, switch_table, interval_editor.value());
      });
}
