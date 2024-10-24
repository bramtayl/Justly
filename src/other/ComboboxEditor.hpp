
#pragma once

#include <QComboBox>
#include <QObject>
#include <QStringListModel>

#include "other/other.hpp"

class QWidget;

template <typename Item> struct ComboboxEditor : public QComboBox {
public:
  const QList<Item> &all_items;
  explicit ComboboxEditor(const QList<Item> &all_items_input,
                          QWidget *parent_pointer_input = nullptr)
      : QComboBox(parent_pointer_input), all_items(all_items_input) {
    QList<QString> item_names({""});
    std::transform(all_items.cbegin(), all_items.cend(),
                   std::back_inserter(item_names),
                   [](const Item &item) { return item.name; });
    setModel(new QStringListModel( // NOLINT(cppcoreguidelines-owning-memory)
        item_names, parent_pointer_input));
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");

    setMinimumSize(sizeHint());
  };

  [[nodiscard]] auto value() const -> const Item * {
    auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &all_items.at(row - 1);
  };

  void setValue(const Item *new_value) {
    setCurrentIndex(
        new_value == nullptr
            ? 0
            : static_cast<int>(std::distance(all_items.data(), new_value)) + 1);
  };
};
