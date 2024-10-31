#pragma once

#include <QComboBox>
#include <QObject>
#include <QStringListModel>
#include <nlohmann/json.hpp>
#include <qabstractitemmodel.h>

#include "named/Named.hpp"

class QWidget;

template <std::derived_from<Named> SubNamed>
struct NamedEditor : public QComboBox {
public:
  const QList<SubNamed> &nameds;
  explicit NamedEditor(const QList<SubNamed> &nameds_input,
                       QWidget *parent_pointer)
      : QComboBox(parent_pointer), nameds(nameds_input) {
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  };

  [[nodiscard]] auto value() const -> const SubNamed * {
    auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &nameds.at(row - 1);
  };

  void setValue(const SubNamed *new_value) {
    setCurrentIndex(
        new_value == nullptr
            ? 0
            : static_cast<int>(std::distance(nameds.data(), new_value)) + 1);
  };
};

template <std::derived_from<Named> SubNamed>
void set_model(NamedEditor<SubNamed>& named_editor, QAbstractItemModel& model) {
  named_editor.setModel(&model);
  named_editor.setMinimumSize(named_editor.minimumSizeHint());
}