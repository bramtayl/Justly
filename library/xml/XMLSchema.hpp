#pragma once

#include "xml/XMLParserContext.hpp"

struct XMLSchema : CHandle<_xmlSchema, xmlSchemaFree> {
  explicit XMLSchema(const XMLParserContext& context)
      : CHandle(xmlSchemaParse(context.internal_pointer)) {}
};
