#pragma once

#include <QComboBox>
#include <QObject>
#include <QStringListModel>
#include <nlohmann/json.hpp>

class QWidget;

// named could be any type that has a name field
template <typename Named> struct NamedEditor : public QComboBox {
public:
  const QList<Named> &nameds;
  explicit NamedEditor(const QList<Named> &nameds_input,
                          QWidget *parent_pointer_input = nullptr)
      : QComboBox(parent_pointer_input), nameds(nameds_input) {
    QList<QString> names({""});
    std::transform(nameds.cbegin(), nameds.cend(),
                   std::back_inserter(names),
                   [](const Named &item) { return item.name; });
    setModel(new QStringListModel( // NOLINT(cppcoreguidelines-owning-memory)
        names, parent_pointer_input));
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");

    setMinimumSize(sizeHint());
  };

  [[nodiscard]] auto value() const -> const Named * {
    auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &nameds.at(row - 1);
  };

  void setValue(const Named *new_value) {
    setCurrentIndex(
        new_value == nullptr
            ? 0
            : static_cast<int>(std::distance(nameds.data(), new_value)) + 1);
  };
};

template <typename Named>
auto get_by_name(const QList<Named> &nameds,
                      const QString &name) -> const Named & {
  const auto named_pointer =
      std::find_if(nameds.cbegin(), nameds.cend(),
                   [name](const Named &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <typename Named> auto get_names(const QList<Named> &nameds) {
  std::vector<std::string> names;
  std::transform(
      nameds.cbegin(), nameds.cend(), std::back_inserter(names),
      [](const Named &item) -> std::string { return item.name.toStdString(); });
  return names;
}

template <typename Named>
auto json_to_named(const QList<Named> &nameds,
                  const nlohmann::json &json_instrument) -> const Named & {
  Q_ASSERT(json_instrument.is_string());
  return get_by_name(
      nameds, QString::fromStdString(json_instrument.get<std::string>()));
}

template <typename Named>
[[nodiscard]] auto named_to_json(const Named &item) -> nlohmann::json {
  return item.name.toStdString();
};
