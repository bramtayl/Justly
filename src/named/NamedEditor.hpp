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
  explicit NamedEditor(QWidget *parent_pointer)
      : QComboBox(parent_pointer) {
    
    static auto names_model = get_list_model(SubNamed::get_all_nameds());
    setModel(&names_model);
    setMinimumSize(minimumSizeHint());
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  };

  [[nodiscard]] auto value() const -> const SubNamed * {
    auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &SubNamed::get_all_nameds().at(row - 1);
  };

  void setValue(const SubNamed *new_value) {
    setCurrentIndex(
        new_value == nullptr
            ? 0
            : static_cast<int>(std::distance(SubNamed::get_all_nameds().data(), new_value)) + 1);
  };
};