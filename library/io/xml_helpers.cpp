#include "io/xml_helpers.hpp"

#include "io/XMLChildren.hpp"

XMLString::~XMLString() { xmlFree(internal_pointer); }

auto c_string_to_xml_string(const char* text) -> const xmlChar* {
  return reinterpret_cast<  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const xmlChar*>(text);
}

auto xml_string_to_c_string(const xmlChar* text) -> const char* {
  return reinterpret_cast<  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const char*>(text);
}

auto xml_string_to_string(const xmlChar* text) -> std::string {
  return {xml_string_to_c_string(text)};
}

auto get_xml_name(const xmlNode& node) -> std::string {
  return xml_string_to_string(node.name);
}

auto get_content(const xmlNode& node) -> std::string {
  const XMLString content{xmlNodeGetContent(&node)};
  return xml_string_to_string(content.internal_pointer);
}

auto get_qstring_content(const xmlNode& node) -> QString {
  return QString::fromStdString(get_content(node));
}

auto get_property(xmlNode& node, const char* name) -> std::string {
  // the schemas require every attribute read this way
  const auto maybe_property = maybe_get_property(node, name);
  Q_ASSERT(maybe_property.has_value());
  return maybe_property.value_or("");
}

auto maybe_get_property(xmlNode& node, const char* name)
    -> std::optional<std::string> {
  const XMLString property{xmlGetProp(&node, c_string_to_xml_string(name))};
  if (property.internal_pointer == nullptr) {
    return std::nullopt;
  }
  return xml_string_to_string(property.internal_pointer);
}

auto node_is(const xmlNode& node, const char* name) -> bool {
  return get_xml_name(node) == name;
}

auto maybe_get_xml_child(xmlNode& node, const char* name) -> xmlNode* {
  for (auto& child : get_xml_children(node)) {
    if (node_is(child, name)) {
      return &child;
    }
  }
  return nullptr;
}

auto get_xml_child(xmlNode& node, const char* name) -> xmlNode& {
  return get_reference(maybe_get_xml_child(node, name));
}

auto string_to_maybe_int(const std::string& content) -> std::optional<int> {
  try {
    return std::stoi(content);
  } catch (const std::out_of_range&) {
    return std::nullopt;
  }
}

auto string_to_int(const std::string& content) -> int {
  const auto maybe_int = string_to_maybe_int(content);
  // song.xsd and the clipboard xsds bound every int field they define to
  // fit in a 32-bit int, so schema-validated callers can't reach this;
  // genuinely unbounded musicxml fields must call string_to_maybe_int
  // directly and reject the file instead of ever calling this on an
  // out-of-range value
  Q_ASSERT(maybe_int.has_value());
  return maybe_int.value_or(0);
}

auto xml_content_is_integer(const xmlNode& element) -> bool {
  // some musicxml fields (e.g. divisions, duration, transpose chromatic,
  // pitch alter) are xs:decimal rather than xs:integer, to allow fractional
  // divisions or microtones -- xml_to_int would silently truncate those
  // instead of parsing them, so callers that don't support fractional
  // values must check this first and reject the file instead
  return get_content(element).find('.') == std::string::npos;
}

auto xml_to_int(const xmlNode& element) -> int {
  return string_to_int(get_content(element));
}

namespace {

auto get_new_child_pointer(xmlNode& node, const char* const field_name,
                           const xmlChar* contents = nullptr) -> xmlNode* {
  // unlike xmlNewChild, escapes contents, so text like "R&B" or "<" survives
  return xmlNewTextChild(&node, nullptr, c_string_to_xml_string(field_name),
                         contents);
}

}  // namespace

auto get_new_child(xmlNode& node, const char* const field_name) -> xmlNode& {
  return get_reference(get_new_child_pointer(node, field_name));
}

void set_xml_string(xmlNode& node, const char* const field_name,
                    const std::string& contents) {
  auto* result = get_new_child_pointer(
      node, field_name, c_string_to_xml_string(contents.c_str()));
  Q_ASSERT(result != nullptr);
}

void set_xml_int(xmlNode& node, const char* const field_name, int value) {
  set_xml_string(node, field_name, std::to_string(value));
}

void maybe_add_int_to_xml(xmlNode& node, const char* const field_name,
                          const int value, const int default_value) {
  if (value != default_value) {
    set_xml_int(node, field_name, value);
  }
}
