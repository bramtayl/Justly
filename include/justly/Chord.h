#pragma once

#include <iterator>                                 // for back_inserter
#include <memory>                                   // for unique_ptr
#include <nlohmann/detail/iterators/iter_impl.hpp>  // for iter_impl
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>
#include <vector>

#include "justly/Note.h"
#include "justly/NoteChord.h"  // for NoteChord

template <typename ChildType>
auto children_to_json(
    const std::vector<std::unique_ptr<ChildType>> &child_pointers,
    int first_child_number, int number_of_children) -> nlohmann::json {
  nlohmann::json json_children;
  std::transform(
      child_pointers.cbegin() + first_child_number,
      child_pointers.cbegin() + first_child_number + number_of_children,
      std::back_inserter(json_children),
      [](const std::unique_ptr<ChildType> &child_pointer) {
        return child_pointer->json();
      });
  return json_children;
}

template <typename ChildType>
inline void insert_children(
    std::vector<std::unique_ptr<ChildType>> *child_pointers_pointer,
    int first_child_number, const nlohmann::json &json_children) {
  std::transform(
      json_children.cbegin(),
      json_children.cbegin() + static_cast<int>(json_children.size()),
      std::inserter(*child_pointers_pointer,
                    child_pointers_pointer->begin() + first_child_number),
      [](const nlohmann::json &child) {
        return std::make_unique<ChildType>(child);
      });
}

struct Chord : NoteChord {
  std::vector<std::unique_ptr<Note>> note_pointers;

  Chord() = default;
  explicit Chord(const nlohmann::json &);
  ~Chord() override = default;

  [[nodiscard]] auto symbol() const -> std::string override;
  [[nodiscard]] static auto json_schema() -> const nlohmann::json &;
  [[nodiscard]] auto json() const -> nlohmann::json override;
};
