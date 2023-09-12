#include "JsonErrorHandler.h"

#include <qmessagebox.h>  // for QMessageBox
#include <qobject.h>      // for QObject
#include <qstring.h>

#include <nlohmann/json-schema.hpp>
#include <ostream>  // for operator<<, stringstream, basic_...

void JsonErrorHandler::error(
    const nlohmann::json::json_pointer &pointer_to_json,
    const nlohmann::json &json_instance, const std::string &message) {
  nlohmann::json_schema::basic_error_handler::error(pointer_to_json,
                                                    json_instance, message);
  std::stringstream error_io;
  error_io << "ERROR: " << message;
  QMessageBox::warning(nullptr, QObject::tr("Parsing error"),
                       QString::fromStdString(error_io.str()));
}