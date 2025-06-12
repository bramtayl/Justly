#pragma once

#include <QtCore/QDir>
#include <QtGui/QGuiApplication>
#include <libxml/tree.h>

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> auto & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     const int first_row_number,
                                     const int number_of_rows) {
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

[[nodiscard]] static auto get_c_string_content(const xmlNode &node) {
  return xml_string_to_c_string(xmlNodeGetContent(&node));
}

[[nodiscard]] static auto get_content(const xmlNode &node) {
  return std::string(get_c_string_content(node));
}

[[nodiscard]] static inline auto xml_to_int(const xmlNode &element) {
  return std::stoi(get_content(element));
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

[[nodiscard]] static inline auto get_share_file(const char *file_name) {
  static const auto share_folder = []() {
    QDir folder(QCoreApplication::applicationDirPath());
    folder.cdUp();
    folder.cd("share");
    return folder;
  }();
  const auto result_file = share_folder.filePath(file_name);
  Q_ASSERT(QFile::exists(result_file));
  return result_file.toStdString();
}
