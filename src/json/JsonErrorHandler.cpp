#include "json/JsonErrorHandler.hpp"

#include <qmessagebox.h>  // for QMessageBox
#include <qobject.h>      // for QObject
#include <qstring.h>      // for QString

#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for operator<<
#include <nlohmann/json-schema.hpp>          // for basic_error_handler
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <sstream>                           // for operator<<, basic_ostream

class QWidget;

JsonErrorHandler::JsonErrorHandler(QWidget *parent_pointer_input)
    : parent_pointer(parent_pointer_input) {}

void JsonErrorHandler::error(
    const nlohmann::json::json_pointer &pointer_to_json,
    const nlohmann::json &json_instance, const std::string &message) {
  nlohmann::json_schema::basic_error_handler::error(pointer_to_json,
                                                    json_instance, message);
  std::stringstream error_message;
  error_message << "\"" << pointer_to_json << "\" - \"" << json_instance
                << "\": " << message << "\n";
  show_parse_error(parent_pointer, error_message.str());
}

void show_parse_error(QWidget *parent_pointer, const std::string &message) {
  QMessageBox::warning(parent_pointer, QObject::tr("Parsing error"),
                       QString::fromStdString(message));
}
