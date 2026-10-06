#pragma once

#include <libxml/xmlschemas.h>

#include "xml/XMLDocument.hpp"

struct XMLParserContext
    : CHandle<_xmlSchemaParserCtxt, xmlSchemaFreeParserCtxt> {
  explicit XMLParserContext(const char* filename)
      : CHandle(xmlSchemaNewParserCtxt(filename)) {}
};

struct XMLSchema : CHandle<_xmlSchema, xmlSchemaFree> {
  explicit XMLSchema(const XMLParserContext& context)
      : CHandle(xmlSchemaParse(context.internal_pointer)) {}
};

struct XMLValidationContext
    : CHandle<_xmlSchemaValidCtxt, xmlSchemaFreeValidCtxt> {
  explicit XMLValidationContext(XMLSchema& schema)
      : CHandle(xmlSchemaNewValidCtxt(schema.internal_pointer)) {}
};

struct XMLValidator {
  XMLSchema xml_schema;
  XMLValidationContext context;
  explicit XMLValidator(const char* filename)
      : xml_schema(XMLParserContext(get_share_file(filename).c_str())),
        context(xml_schema) {}
};

// 0 if document matches the validator's schema
[[nodiscard]] inline auto validate_against_schema(XMLValidator& validator,
                                                  XMLDocument& document)
    -> int {
  return xmlSchemaValidateDoc(validator.context.internal_pointer,
                              document.internal_pointer);
}
