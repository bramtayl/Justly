#pragma once

#include <QtGlobal>
#include <nlohmann/json.hpp>

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

auto to_int(double value) -> int;

[[nodiscard]] auto get_number_schema(const char *type, int minimum,
                                     int maximum) -> nlohmann::json;