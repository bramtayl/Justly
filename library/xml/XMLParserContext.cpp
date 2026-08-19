#include "xml/XMLParserContext.hpp"

XMLParserContext::~XMLParserContext() { xmlSchemaFreeParserCtxt(internal_pointer); }
