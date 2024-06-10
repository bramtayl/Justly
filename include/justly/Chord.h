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

template <typename ObjectType>
auto objects_to_json(
    const std::vector<std::unique_ptr<ObjectType>> &object_pointers,
    int first_index, int number) -> nlohmann::json {
  nlohmann::json json_objects;
  std::transform(object_pointers.cbegin() + first_index,
                 object_pointers.cbegin() + first_index + number,
                 std::back_inserter(json_objects),
                 [](const std::unique_ptr<ObjectType> &object_pointer) {
                   return object_pointer->json();
                 });
  return json_objects;
}

template <typename ObjectType>
void objects_from_json(
    std::vector<std::unique_ptr<ObjectType>> *object_pointers, int first_index,
    const nlohmann::json &json_objects) {
  std::transform(
      json_objects.cbegin(),
      json_objects.cbegin() + static_cast<int>(json_objects.size()),
      std::inserter(*object_pointers, object_pointers->begin() + first_index),
      [](const nlohmann::json &json_object) {
        return std::make_unique<ObjectType>(json_object);
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
