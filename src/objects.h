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
void insert_objects(
    std::vector<std::unique_ptr<ObjectType>> *object_pointers,
    int first_object_number, int number_of_objects) {
  for (int child_number = first_object_number;
       child_number < first_object_number + number_of_objects;
       child_number = child_number + 1) {
    object_pointers->insert(object_pointers->begin() + child_number,
                            std::make_unique<ObjectType>());
  }
}

template <typename ObjectType>
void remove_objects(std::vector<std::unique_ptr<ObjectType>> *object_pointers,
                     size_t first_object_number, size_t number_of_objects) {
  object_pointers->erase(
      object_pointers->begin() + first_object_number,
      object_pointers->begin() + first_object_number + number_of_objects);
}

template <typename ObjectType>
auto to_json(
    const std::vector<std::unique_ptr<ObjectType>> &object_pointers,
    size_t first_object_number, size_t number_of_objects) -> nlohmann::json {
  nlohmann::json json_objects;
  std::transform(object_pointers.cbegin() + first_object_number,
                 object_pointers.cbegin() + first_object_number + number_of_objects,
                 std::back_inserter(json_objects),
                 [](const std::unique_ptr<ObjectType> &object_pointer) {
                   return object_pointer->json();
                 });
  return json_objects;
}

template <typename ObjectType>
void from_json(
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
