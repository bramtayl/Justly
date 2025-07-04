#pragma once

#include <QtWidgets/QComboBox>

#include "cell_types/Program.hpp"

[[nodiscard]] static auto get_program_model(const QList<Program> &programs) {
  QList<QString> names({""});
  std::transform(programs.cbegin(), programs.cend(), std::back_inserter(names),
                 [](const Program &item) { return item.translated_name; });
  return QStringListModel(names);
}

struct ProgramEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Program *value READ value WRITE setValue USER true)

public:
  const QList<Program> &programs;
  explicit ProgramEditor(QWidget *const parent_pointer, bool is_pitched = true)
      : QComboBox(parent_pointer),
        programs(is_pitched ? get_some_programs(true)
                            : get_some_programs(false)) {

    static auto instruments_model =
        get_program_model(get_some_programs(true));
    static auto percussion_instruments_model =
        get_program_model(get_some_programs(false));
    // force scrollbar for combo box
    setModel(is_pitched ? &instruments_model : &percussion_instruments_model);
    setStyleSheet("combobox-popup: 0;");
  }

  [[nodiscard]] auto value() const -> const Program * {
    const auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &programs.at(row - 1);
  }

  void setValue(const Program *new_value) {
    setCurrentIndex(
        new_value == nullptr
            ? 0
            : static_cast<int>(std::distance(programs.data(), new_value)) + 1);
  }
};
