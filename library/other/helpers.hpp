#pragma once

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QMessageBox>
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlstring.h>
#include <limits>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)
#define NO_COPY(classname)                                                     \
  classname(const classname &) = delete;                                       \
  auto operator=(const classname &)->classname = delete;

#define NO_MOVE_COPY(classname)                                                \
  NO_COPY(classname)                                                           \
  classname(classname &&) = delete;                                            \
  auto operator=(classname &&)->classname = delete;
// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

struct XMLString {
  xmlChar *internal_pointer = nullptr;

  XMLString() = default;

  explicit XMLString(xmlChar *internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLString() { xmlFree(internal_pointer); }

  NO_MOVE_COPY(XMLString)
};

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> auto & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

[[nodiscard]] static inline auto get_clipboard() -> auto & {
  return get_reference(QGuiApplication::clipboard());
}

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     const int first_row_number,
                                     const int number_of_rows) {
  Q_ASSERT(first_row_number >= 0);
  Q_ASSERT(number_of_rows >= 0);
  Q_ASSERT(first_row_number + number_of_rows <= items.size());

  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

template <typename SubType>
[[nodiscard]] static auto variant_to(const QVariant &variant) {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}

[[nodiscard]] static auto c_string_to_xml_string(const char *text) {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const xmlChar *>(text);
}

[[nodiscard]] static auto xml_string_to_c_string(const xmlChar *text) {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const char *>(text);
}

[[nodiscard]] static auto xml_string_to_string(const xmlChar *text) {
  return std::string(xml_string_to_c_string(text));
}

[[nodiscard]] static inline auto get_xml_name(const xmlNode &node) {
  return xml_string_to_string(node.name);
}

[[nodiscard]] static auto get_content(const xmlNode &node) {
  const XMLString content{xmlNodeGetContent(&node)};
  return xml_string_to_string(content.internal_pointer);
}

[[nodiscard]] static inline auto string_to_int(const std::string &content) {
  try {
    return std::stoi(content);
  } catch (const std::out_of_range &) {
    // some musicxml fields (e.g. fifths, octave-change, repeat times) are
    // unbounded xs:integer with no schema-enforced range, so a malformed or
    // hostile file can contain a magnitude that overflows int; clamp instead
    // of letting std::stoi's exception propagate uncaught and crash the app
    // -- whatever consumes the clamped value then fails through its own
    // existing bounds checks (e.g. the frequency-range warning) instead
    return content.starts_with('-') ? std::numeric_limits<int>::min()
                                    : std::numeric_limits<int>::max();
  }
}

[[nodiscard]] static inline auto xml_content_is_integer(const xmlNode &element)
    -> bool {
  // some musicxml fields (e.g. divisions, duration, transpose chromatic,
  // pitch alter) are xs:decimal rather than xs:integer, to allow fractional
  // divisions or microtones -- xml_to_int would silently truncate those
  // instead of parsing them, so callers that don't support fractional
  // values must check this first and reject the file instead
  return get_content(element).find('.') == std::string::npos;
}

[[nodiscard]] static inline auto xml_to_int(const xmlNode &element) {
  return string_to_int(get_content(element));
}

[[nodiscard]] static auto
get_new_child_pointer(xmlNode &node, const char *const field_name,
                      const xmlChar *contents = nullptr) {
  return xmlNewChild(&node, nullptr, c_string_to_xml_string(field_name),
                     contents);
}

[[nodiscard]] static auto inline get_new_child(
    xmlNode &node, const char *const field_name) -> auto & {
  return get_reference(get_new_child_pointer(node, field_name));
}

static void set_xml_string(xmlNode &node, const char *const field_name,
                           const std::string &contents) {
  auto *result = get_new_child_pointer(
      node, field_name, c_string_to_xml_string(contents.c_str()));
  Q_ASSERT(result != nullptr);
}

static inline void set_xml_int(xmlNode &node, const char *const field_name,
                               int value) {
  set_xml_string(node, field_name, std::to_string(value));
}

// installed layout is <prefix>/share next to the binary's folder, except
// inside a macOS app bundle, where resources live in Contents/Resources
// rather than alongside Contents/MacOS
[[nodiscard]] static inline auto get_share_folder() {
  QDir folder(QCoreApplication::applicationDirPath());
  folder.cdUp();
  if (folder.dirName() == "Contents") {
    folder.cd("Resources");
  } else {
    folder.cd("share");
  }
  return folder;
}

[[nodiscard]] static inline auto get_share_file(const char *file_name)
    -> std::string {
  static const auto share_folder = get_share_folder();
  const auto result_file = share_folder.filePath(file_name);
  if (!QFile::exists(result_file)) {
    // Q_ASSERT compiles out in release builds, so a broken/incomplete
    // installation missing a bundled resource (xsd schema, icon,
    // soundfont) would otherwise silently hand the caller a nonexistent
    // path instead of failing here; there's no valid path to hand back,
    // so report it to the user and exit rather than throwing
    QMessageBox::critical(
        nullptr, QObject::tr("Missing resource file"),
        QObject::tr("Missing bundled resource file: %1")
            .arg(result_file));
    std::exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)
  }
  return result_file.toStdString();
}
