#pragma once

#include <QVariant>
#include <QtGlobal>

class QWidget;

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Thing>
[[nodiscard]] static auto
get_const_reference(const Thing *thing_pointer) -> const Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename SubType>
auto variant_to(const QVariant &variant) -> SubType {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}

void prevent_compression(QWidget &widget);
