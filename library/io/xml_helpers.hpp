#pragma once

#include <libxml/parser.h>

#include <optional>
#include <string>

#include "common/helpers.hpp"

struct XMLString {
  xmlChar* internal_pointer = nullptr;

  XMLString() = default;

  explicit XMLString(xmlChar* internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLString();

  NO_MOVE_COPY(XMLString)
};

[[nodiscard]] auto c_string_to_xml_string(const char* text) -> const xmlChar*;

[[nodiscard]] auto xml_string_to_c_string(const xmlChar* text) -> const char*;

[[nodiscard]] auto xml_string_to_string(const xmlChar* text) -> std::string;

[[nodiscard]] auto get_xml_name(const xmlNode& node) -> std::string;

[[nodiscard]] auto get_content(const xmlNode& node) -> std::string;

[[nodiscard]] auto get_qstring_content(const xmlNode& node) -> QString;

[[nodiscard]] auto get_property(xmlNode& node, const char* name) -> std::string;

[[nodiscard]] auto maybe_get_property(xmlNode& node, const char* name)
    -> std::optional<std::string>;

[[nodiscard]] auto node_is(const xmlNode& node, const char* name) -> bool;

[[nodiscard]] auto maybe_get_xml_child(xmlNode& node, const char* name)
    -> xmlNode*;

[[nodiscard]] auto get_xml_child(xmlNode& node, const char* name) -> xmlNode&;

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

void maybe_add_int_to_xml(xmlNode& node, const char* field_name, int value,
                          int default_value);
