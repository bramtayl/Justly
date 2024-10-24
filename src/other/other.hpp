#pragma once

#include <QString>
#include <QVariant>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

[[nodiscard]] auto variant_to_string(const QVariant &variant) -> QString;

[[nodiscard]] auto get_words_schema() -> nlohmann::json;

[[nodiscard]] auto make_validator(const char *title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator;
