#include "other/helpers.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/qassert.h>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QMessageBox>
#include <cstdlib>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlstring.h>
#include <optional>
#include <stdexcept>
#include <string>

class QClipboard;

XMLString::~XMLString() { xmlFree(internal_pointer); }

auto get_clipboard() -> QClipboard & {
  return get_reference(QGuiApplication::clipboard());
}

auto c_string_to_xml_string(const char *text) -> const xmlChar * {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const xmlChar *>(text);
}

auto xml_string_to_c_string(const xmlChar *text) -> const char * {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const char *>(text);
}

auto xml_string_to_string(const xmlChar *text) -> std::string {
  return std::string(xml_string_to_c_string(text));
}

auto get_xml_name(const xmlNode &node) -> std::string {
  return xml_string_to_string(node.name);
}

auto get_content(const xmlNode &node) -> std::string {
  const XMLString content{xmlNodeGetContent(&node)};
  return xml_string_to_string(content.internal_pointer);
}

auto string_to_maybe_int(const std::string &content) -> std::optional<int> {
  try {
    return std::stoi(content);
  } catch (const std::out_of_range &) {
    return std::nullopt;
  }
}

auto string_to_int(const std::string &content) -> int {
  const auto maybe_int = string_to_maybe_int(content);
  // song.xsd and the clipboard xsds bound every int field they define to
  // fit in a 32-bit int, so schema-validated callers can't reach this;
  // genuinely unbounded musicxml fields must call string_to_maybe_int
  // directly and reject the file instead of ever calling this on an
  // out-of-range value
  Q_ASSERT(maybe_int.has_value());
  return maybe_int.value_or(0);
}

auto xml_content_is_integer(const xmlNode &element) -> bool {
  // some musicxml fields (e.g. divisions, duration, transpose chromatic,
  // pitch alter) are xs:decimal rather than xs:integer, to allow fractional
  // divisions or microtones -- xml_to_int would silently truncate those
  // instead of parsing them, so callers that don't support fractional
  // values must check this first and reject the file instead
  return get_content(element).find('.') == std::string::npos;
}

auto xml_to_int(const xmlNode &element) -> int {
  return string_to_int(get_content(element));
}

namespace {

auto get_new_child_pointer(xmlNode &node, const char *const field_name,
                           const xmlChar *contents = nullptr) -> xmlNode * {
  return xmlNewChild(&node, nullptr, c_string_to_xml_string(field_name),
                     contents);
}

}  // namespace

auto get_new_child(xmlNode &node, const char *const field_name) -> xmlNode & {
  return get_reference(get_new_child_pointer(node, field_name));
}

void set_xml_string(xmlNode &node, const char *const field_name,
                     const std::string &contents) {
  auto *result = get_new_child_pointer(
      node, field_name, c_string_to_xml_string(contents.c_str()));
  Q_ASSERT(result != nullptr);
}

void set_xml_int(xmlNode &node, const char *const field_name, int value) {
  set_xml_string(node, field_name, std::to_string(value));
}

auto get_share_folder() -> QDir {
  QDir folder(QCoreApplication::applicationDirPath());
  folder.cdUp();
  if (folder.dirName() == "Contents") {
    folder.cd("Resources");
  } else {
    folder.cd("share");
  }
  return folder;
}

auto get_share_file(const char *file_name) -> std::string {
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
