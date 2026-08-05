#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtAssert>
#include <string>
#include <zip.h>
#include <zipconf.h>

#include "other/helpers.hpp"

class ZipArchive {
public:
  zip_t *const internal_pointer;

  explicit ZipArchive(const QString &filename)
      : internal_pointer(zip_open(filename.toStdString().c_str(),
                                  ZIP_RDONLY, nullptr)) {}

  ~ZipArchive() {
    if (internal_pointer != nullptr) {
      zip_close(internal_pointer);
    }
  }

  NO_MOVE_COPY(ZipArchive)
};

// returns an empty QByteArray if the entry is missing or can't be read
[[nodiscard]] static inline auto
read_zip_entry(const ZipArchive &archive,
               const std::string &entry_name) -> QByteArray {
  Q_ASSERT(archive.internal_pointer != nullptr);

  zip_stat_t entry_stat;
  if (zip_stat(archive.internal_pointer, entry_name.c_str(), 0,
              &entry_stat) != 0) {
    return {};
  }

  auto *file_pointer =
      zip_fopen(archive.internal_pointer, entry_name.c_str(), 0);
  if (file_pointer == nullptr) {
    return {};
  }

  QByteArray bytes(static_cast<int>(entry_stat.size), '\0');
  const auto bytes_read = zip_fread(file_pointer, bytes.data(), entry_stat.size);
  zip_fclose(file_pointer);

  if (bytes_read != static_cast<zip_int64_t>(entry_stat.size)) {
    return {};
  }
  return bytes;
}
