#pragma once

#include <libxml/parser.h>
#include <libxml/tree.h>

class XMLDocument {
public:
  xmlDoc *internal_pointer;

  XMLDocument()
      : internal_pointer(xmlNewDoc(nullptr)) {}

  explicit XMLDocument(xmlDoc *internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLDocument() { xmlFreeDoc(internal_pointer); }

  XMLDocument(const XMLDocument &) = delete;
  auto operator=(const XMLDocument &) -> XMLDocument = delete;
  XMLDocument(XMLDocument &&) = delete;
  auto operator=(XMLDocument &&) -> XMLDocument = delete;
};