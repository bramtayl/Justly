#include "named/Named.hpp"

auto get_name_or_empty(const Named *named_pointer) -> QString {
  if (named_pointer == nullptr) {
    return "";
  }
  return named_pointer->name;
}

void add_named_to_json(nlohmann::json &json_row, const Named *named_pointer,
                       const char *column_name) {
  if (named_pointer != nullptr) {
    std::string named = named_pointer->name.toStdString();
    json_row[column_name] = std::move(named);
  }
}