#pragma once

#include "xml/XMLValidationContext.hpp"

struct XMLValidator {
  XMLSchema xml_schema;
  XMLValidationContext context;
  explicit XMLValidator(const char* filename)
      : xml_schema(XMLParserContext(get_share_file(filename).c_str())),
        context(XMLValidationContext(xml_schema)) {}
};
