#pragma once

#include <libxml/xmlschemas.h>

#include "other/helpers.hpp"

class XMLParserContext {
public:
  _xmlSchemaParserCtxt * const internal_pointer;

  explicit XMLParserContext(const char* filename)
      : internal_pointer(xmlSchemaNewParserCtxt(filename)) {}

  ~XMLParserContext() { xmlSchemaFreeParserCtxt(internal_pointer); }

  NO_MOVE_COPY(XMLParserContext)
};
