#pragma once

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "other/helpers.hpp"

class XMLDocument {
public:
  xmlDoc *internal_pointer;

  XMLDocument() : internal_pointer(xmlNewDoc(nullptr)) {}

  explicit XMLDocument(xmlDoc *internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLDocument() { xmlFreeDoc(internal_pointer); }

  NO_MOVE_COPY(XMLDocument)
};

[[nodiscard]] static auto get_root(const XMLDocument &document) -> auto & {
  return get_reference(xmlDocGetRootElement(document.internal_pointer));
}

[[nodiscard]] static inline auto make_root(XMLDocument &document,
                                           const char *field_name) -> auto & {
  xmlDocSetRootElement(document.internal_pointer,
                       xmlNewNode(nullptr, c_string_to_xml_string(field_name)));
  return get_root(document);
}
