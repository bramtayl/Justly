#include "xml/ZipArchive.hpp"

ZipArchive::~ZipArchive() {
  if (internal_pointer != nullptr) {
    zip_close(internal_pointer);
  }
}

auto zip_entry_size_is_safe(const zip_stat_t& entry_stat) -> bool {
  return (entry_stat.valid & ZIP_STAT_SIZE) != 0 &&
         entry_stat.size <=
             static_cast<zip_uint64_t>(std::numeric_limits<int>::max());
}

auto read_zip_entry(const ZipArchive& archive, const std::string& entry_name)
    -> QByteArray {
  if (archive.internal_pointer == nullptr) {
    return {};
  }

  zip_stat_t entry_stat;
  if (zip_stat(archive.internal_pointer, entry_name.c_str(), 0, &entry_stat) !=
      0) {
    return {};
  }

  if (!zip_entry_size_is_safe(entry_stat)) {
    return {};
  }

  auto* file_pointer =
      zip_fopen(archive.internal_pointer, entry_name.c_str(), 0);
  if (file_pointer == nullptr) {
    return {};
  }

  QByteArray bytes(static_cast<int>(entry_stat.size), '\0');
  const auto bytes_read =
      zip_fread(file_pointer, bytes.data(), entry_stat.size);
  zip_fclose(file_pointer);

  if (std::cmp_not_equal(bytes_read, entry_stat.size)) {
    return {};
  }
  return bytes;
}
