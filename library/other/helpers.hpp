#pragma once

#include <libxml/parser.h>

#include <QtCore/QDir>

class QClipboard;
class QAbstractItemModel;
class QItemSelectionRange;

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)
#define NO_COPY(classname)              \
  classname(const classname&) = delete; \
  auto operator=(const classname&)->classname = delete;

#define NO_MOVE_COPY(classname)    \
  NO_COPY(classname)               \
  classname(classname&&) = delete; \
  auto operator=(classname&&)->classname = delete;
// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

struct XMLString {
  xmlChar* internal_pointer = nullptr;

  XMLString() = default;

  explicit XMLString(xmlChar* internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLString();

  NO_MOVE_COPY(XMLString)
};

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing* thing_pointer) -> auto& {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

[[nodiscard]] auto get_clipboard() -> QClipboard&;

[[nodiscard]] auto get_number_of_rows(const QItemSelectionRange& range) -> int;

[[nodiscard]] auto make_range(QAbstractItemModel& model, int first_row_number,
                              int number_of_rows, int left_column,
                              int right_column) -> QItemSelectionRange;

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item>& items,
                                     const int first_row_number,
                                     const int number_of_rows) {
  Q_ASSERT(first_row_number >= 0);
  Q_ASSERT(number_of_rows >= 0);
  Q_ASSERT(first_row_number + number_of_rows <= items.size());

  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

template <typename SubType>
[[nodiscard]] static auto variant_to(const QVariant& variant) {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}

[[nodiscard]] auto c_string_to_xml_string(const char* text) -> const xmlChar*;

[[nodiscard]] auto xml_string_to_c_string(const xmlChar* text) -> const char*;

[[nodiscard]] auto xml_string_to_string(const xmlChar* text) -> std::string;

[[nodiscard]] auto get_xml_name(const xmlNode& node) -> std::string;

[[nodiscard]] auto get_content(const xmlNode& node) -> std::string;

// some musicxml fields (e.g. fifths, octave-change, repeat times) are
// unbounded xs:integer with no schema-enforced range, so a malformed or
// hostile file can contain a magnitude that overflows int; callers that
// read those fields must use this instead of string_to_int and reject the
// file on nullopt rather than letting std::stoi throw
[[nodiscard]] auto string_to_maybe_int(const std::string& content)
    -> std::optional<int>;

[[nodiscard]] auto string_to_int(const std::string& content) -> int;

[[nodiscard]] auto xml_content_is_integer(const xmlNode& element) -> bool;

[[nodiscard]] auto xml_to_int(const xmlNode& element) -> int;

[[nodiscard]] auto get_new_child(xmlNode& node, const char* field_name)
    -> xmlNode&;

void set_xml_string(xmlNode& node, const char* field_name,
                    const std::string& contents);

void set_xml_int(xmlNode& node, const char* field_name, int value);

// installed layout is <prefix>/share next to the binary's folder, except
// inside a macOS app bundle, where resources live in Contents/Resources
// rather than alongside Contents/MacOS
[[nodiscard]] auto get_share_folder() -> QDir;

[[nodiscard]] auto get_share_file(const char* file_name) -> std::string;
