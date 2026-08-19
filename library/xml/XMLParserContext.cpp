#include "xml/XMLParserContext.hpp"

#include <libxml/xmlschemas.h>

XMLParserContext::~XMLParserContext() {
  xmlSchemaFreeParserCtxt(internal_pointer);
}
