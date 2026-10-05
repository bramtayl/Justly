#pragma once

#include <libxml/xmlschemas.h>

#include "other/helpers.hpp"

struct XMLParserContext
    : CHandle<_xmlSchemaParserCtxt, xmlSchemaFreeParserCtxt> {
  explicit XMLParserContext(const char* filename)
      : CHandle(xmlSchemaNewParserCtxt(filename)) {}
};
