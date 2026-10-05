#pragma once

#include "xml/XMLDocument.hpp"
#include "xml/XMLValidationContext.hpp"

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
