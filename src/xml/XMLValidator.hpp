#pragma once

#include <libxml/xmlschemas.h>

#include "xml/XMLDocument.hpp"
#include "other/helpers.hpp"

struct XMLValidator {
  xmlSchemaPtr schema_pointer;
  xmlSchemaValidCtxtPtr context_pointer;
  explicit XMLValidator(const char *filename) {
    auto &parser_context =
        get_reference(xmlSchemaNewParserCtxt(get_share_file(filename).c_str()));
    schema_pointer = xmlSchemaParse(&parser_context);
    xmlSchemaFreeParserCtxt(&parser_context);
    context_pointer = // NOLINT(cppcoreguidelines-prefer-member-initializer)
        xmlSchemaNewValidCtxt(schema_pointer);
  }
  ~XMLValidator() {
    xmlSchemaFreeValidCtxt(context_pointer);
    xmlSchemaFree(schema_pointer);
  }

  XMLValidator(const XMLValidator &) = delete;
  auto operator=(const XMLValidator &) -> XMLValidator = delete;
  XMLValidator(XMLValidator &&) = delete;
  auto operator=(XMLValidator &&) -> XMLValidator = delete;
};

[[nodiscard]] static inline auto
validate_against_schema(XMLValidator &validator, XMLDocument& document) {
  return xmlSchemaValidateDoc(validator.context_pointer, document.internal_pointer);
}