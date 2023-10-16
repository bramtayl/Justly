#pragma once

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string

class JsonErrorHandler : public nlohmann::json_schema::basic_error_handler {
 public:
  void error(const nlohmann::json::json_pointer & pointer_to_json,
             const nlohmann::json & json_instance,
             const std::string & message) override;
  static void show_parse_error(const std::string &);
};
