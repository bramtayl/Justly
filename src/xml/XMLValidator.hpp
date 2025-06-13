#pragma once

#include <libxml/xmlschemas.h>

#include "xml/XMLDocument.hpp"
#include "xml/XMLParserContext.hpp"
#include "xml/XMLSchema.hpp"
#include "xml/XMLValidationContext.hpp"

struct XMLValidator {
  XMLSchema xml_schema;
  XMLValidationContext context;
  explicit XMLValidator(const char *filename)
      : xml_schema(
            XMLSchema(XMLParserContext(get_share_file(filename).c_str()))),
        context(XMLValidationContext(xml_schema)) {}
};

[[nodiscard]] static inline auto
validate_against_schema(XMLValidator &validator, XMLDocument &document) {
  return xmlSchemaValidateDoc(validator.context.internal_pointer,
                              document.internal_pointer);
}