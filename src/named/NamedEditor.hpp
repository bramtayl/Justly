#pragma once

#include <QComboBox>
#include <QObject>
#include <QStringListModel>
#include <nlohmann/json.hpp>

#include "named/Named.hpp"

class QWidget;

// named could be any type that has a name field
template <std::derived_from<Named> SubNamed> struct NamedEditor : public QComboBox {
public:
  const QList<SubNamed> &nameds;
  explicit NamedEditor(const QList<SubNamed> &nameds_input,
                          QWidget *parent_pointer_input = nullptr)
      : QComboBox(parent_pointer_input), nameds(nameds_input) {
    QList<QString> names({""});
    std::transform(nameds.cbegin(), nameds.cend(),
                   std::back_inserter(names),
                   [](const SubNamed &item) { return item.name; });
    setModel(new QStringListModel( // NOLINT(cppcoreguidelines-owning-memory)
        names, parent_pointer_input));
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");

    setMinimumSize(sizeHint());
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
